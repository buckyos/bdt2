import * as net from 'net';
import { LogShim, Endpoint, EndpointProtocol, EndpointIPVersion, LoggerInstance, ErrorCode, Peerid, stringifyErrorCode } from '../../base';
import { TCPPackageDecoder, createTCPPackageDecoder, PackageDecryptKeyDelegate, createPackageDispatcher, PackageCommand } from '../../protocol';
import { NetworkInterface } from './interface';
import { LocalPeer } from '../local_peer';
import { PeerHistoryManager } from '../peer_history';
import { RecordDecoder, NoneRecordDecoder, EncryptRecordDecoder } from './tcp_record';

enum TCPNetworkInterfaceState {
    none = 0, 
    init = 10, 
    connecting = 20,
    established = 30
}

const EmptyBuffer = Buffer.allocUnsafe(0);

export class TCPNetworkInterface extends NetworkInterface implements PackageDecryptKeyDelegate {
    constructor(options: {logger: LoggerInstance, localPeer: LocalPeer, historyManager: PeerHistoryManager}) {
        super(options);
        this.m_decoder = createTCPPackageDecoder(options.logger, this);
        this.m_streaming = false;
    }

    on(event: 'drain', listener: () => void): this;
    on(event: 'end', listener: () => void): this;
    on(event: 'error', listener: (err: ErrorCode) => void): this;
    on(event: 'close', listener: (err: boolean) => void): this;
    on(event: 'establish', listener: () => void): this;
    on(event: string, listener: (...args: any[]) => void): this {
        super.on(event, listener);
        return this;
    }

    connect(options: {
        local?: Endpoint, 
        remote: Endpoint,
    }): ErrorCode {
        this.m_remote = options.remote;
        this.m_state = TCPNetworkInterfaceState.connecting;
        if (options.local) {
            this.m_socket = net.connect({
                port: options.remote.port,
                host: options.remote.address,
                localAddress: options.local!.address,
                localPort: options.local!.port,
            });
            this.m_local = options.local.clone();
        } else {
            this.m_socket = net.createConnection(options.remote.port, options.remote.address);
            this.m_local = new Endpoint({
                protocol: EndpointProtocol.tcp,
                ipVersion: net.isIPv4(options.remote.address) ? EndpointIPVersion.ipv4 : EndpointIPVersion.ipv6,
                address: this.m_socket.localAddress,
                port: this.m_socket.localPort,
            });
        }

        this.m_logger = new LogShim(this.m_logger).bind(`[TCPInterface: ${this.m_local.toString()}to${this.m_remote.toString()}]`, true).log;
        this.m_dispatcher = createPackageDispatcher(this.m_logger);

        this.m_socket!.once('connect', () => {
            this.m_local = new Endpoint({
                protocol: EndpointProtocol.tcp,
                ipVersion: this.m_local!.ipVersion,
                address: this.m_socket!.localAddress,
                port: this.m_socket!.localPort,
            });
            this.m_socket!.removeAllListeners('error');
            this._initSocket();
            this.emit('establish');
        });
        this.m_socket!.once('error', (error: Error) => {
            this.m_socket!.removeAllListeners('connect');
            this.m_logger.error(`TCPNetworkInterface connect to ${options.remote.toString()} failed,err=${error}`);
            this.emit('error', ErrorCode.exception);
        });

        return ErrorCode.success;
    }

    initFromListener(options: {
        socket: net.Socket, 
        keys: Map<string, {peerid: Peerid, key: Buffer}>
    }): ErrorCode {
        this.m_keys = options.keys;
        this.m_socket = options.socket;
        this.m_local = new Endpoint({
            protocol: EndpointProtocol.tcp,
            ipVersion: net.isIPv4(this.m_socket.localAddress) ? EndpointIPVersion.ipv4 : EndpointIPVersion.ipv6,
            address: this.m_socket.localAddress,
            port: this.m_socket.localPort,
        });
        this.m_remote = new Endpoint({
            protocol: EndpointProtocol.tcp,
            ipVersion: this.m_local!.ipVersion,
            address: options.socket.remoteAddress,
            port: options.socket.remotePort,
        });
        this.m_logger = new LogShim(this.m_logger).bind(`[TCPInterface: ${this.m_local.toString()}to${this.m_remote.toString()}]`, true).log;
        this.m_dispatcher = createPackageDispatcher(this.m_logger);
        this._initSocket();
        return ErrorCode.success;
    }

    pause(): void {
        if (!this.m_bPause) {
            this.m_socket!.pause();
            this.m_bPause = true;
        }
    }

    resume(): void {
        if (this.m_bPause) {
            this.m_socket!.resume();
            this.m_bPause = false;
        }
    }

    private _initSocket() {
        this.m_state = TCPNetworkInterfaceState.established;
        this.m_socket!.on('data', (data: Buffer) => {
            if (this.m_streaming) {
                const cr = this.m_recordDecoder!.push(data);
                if (cr.err) {
                    this.m_logger.debug(`decode record failed ${cr.err}`);
                    this.emit('error', cr.err);
                }
                if (cr.data) {
                    this.m_listener!(cr.data);
                }
            } else {
                do {
                    const ret = this.m_decoder.decode(data, 1);
                    if (ret.err) {
                        if (ret.err !== ErrorCode.pending) {
                            // 不够一个包大小，或者是流式数据包，可能无法解包
                            this.m_logger.debug(`decode package failed for ${stringifyErrorCode(ret.err)}`);
                            this.m_socket!.destroy(ret.err);
                        }
                        break;
                    } else {
                        data = EmptyBuffer;
                        const firstPkg = ret.packages![0]!;
                        if (firstPkg.cmdType === PackageCommand.exchangeKey) {
                            // exchangeKey只要第一时间添加到keyManager即可，不需要处理
                            this.m_historyManager.addKey({
                                peerid: firstPkg.__encrypt.peerid, 
                                key: firstPkg.__encrypt.key, 
                                keyHash: firstPkg.__encrypt.keyHash, 
                            });
                            ret.packages!.splice(0, 1);
                        }
                        
                        for (const pkg of ret.packages!) {
                            this.dispatcher.dispatch(pkg);
                        }
                    }
                } while (true);
            }
        });
        this.m_socket!.once('end', () => {
            this.m_logger.info(`tcpinterface socket end`);
            this.emit('end');
        });
        this.m_socket!.on('drain', () => {
            this.m_bQueue = false;
            this.emit('drain');
        });
        this.m_socket!.once('error', (err: Error) => {
            this.m_logger.error(`tcpinterface socket error,info=${err}`);
            this.emit('error', ErrorCode.unknown);
        });
        this.m_socket!.once('close', (had_error: boolean) => {
            this.m_logger.info(`tcpinterface socket close,had_error=${had_error}`);
            this.emit('close');
        });
        this.m_socket!.once('timeout', () => {
            this.emit('error', ErrorCode.timeout);
        });
    }

    get streaming(): boolean {
        return this.m_streaming;
    }

    setStreaming(options: {keyHash?: Buffer, listener: (data: Buffer[]) => void}) {
        if (!this.m_streaming) {
            if (!options.keyHash) {
                this.m_recordDecoder = new NoneRecordDecoder();
            } else {
                this.m_recordDecoder = new EncryptRecordDecoder({
                    keyHash: options.keyHash, 
                    key: this.m_keys.get(options.keyHash!.toString())!.key
                });
            }
            this.m_streaming = true;
            this.m_listener = options.listener;
            for (const record of this.m_decoder.detachCacheData()) {
                const cr = this.m_recordDecoder!.push(record);
                if (cr.err) {
                    this.m_logger.debug(`decode record failed ${cr.err}`);
                    this.emit('error', cr.err);
                }
                if (cr.data) {
                    this.m_listener(cr.data);
                }
            }
        }
    }

    setTimeout(timeout: number) {
        if (!this.m_streaming && this.m_socket) {
            this.m_socket.setTimeout(timeout);
        }
    }

    send(value: Buffer): {err: ErrorCode, sent?: number} {
        if (this.m_bQueue) {
            return {err: ErrorCode.success, sent: 0};
        } 

        try {
            this.m_bQueue = !this.m_socket!.write(value);
            return {err: ErrorCode.success, sent: value.length};
        } catch (error) {
            this.m_logger.error(`tcpinterface send buffer exception, error=${error}`);
            return {err: ErrorCode.unknown};
        }
    }

    uninit() {
        this.removeAllListeners();
        if (this.m_dispatcher) {
            this.m_dispatcher.removeAllHandlers();
        }
        if (this.m_socket) {
            this.m_socket.destroy();
            this.m_socket.removeAllListeners();
            delete this.m_socket;
        }
    }

    async close(): Promise<ErrorCode> {
        if (this.m_socket) {
            this.m_socket.end();
        }
        return ErrorCode.success;
    }

    get local(): Endpoint {
        return this.m_local!;
    }

    get remote(): Endpoint {
        return this.m_remote!;
    }

    get state(): TCPNetworkInterfaceState {
        return this.m_state;
    }

    addKey(options: {
        peerid: Peerid, 
        keyHash: Buffer, 
        key: Buffer
    }) {
        const strHash = options.keyHash.toString();
        this.m_keys.set(strHash, {key: options.key, peerid: options.peerid});
    }

    queryKey(keyHash: Buffer): {err: ErrorCode, key?: Buffer, peerid?: Peerid} {
        const strHash = keyHash.toString();
        const stub = this.m_keys.get(strHash);
        if (!stub) {
            return {err: ErrorCode.notFound};
        }
        return {err: ErrorCode.success, key: stub.key, peerid: stub.peerid};
    }

    getLocal(): {err: ErrorCode, secret?: Buffer, public?: Buffer} {
        return {
            err: ErrorCode.success, 
            secret: this.m_localPeer.secret, 
            public: this.m_localPeer.constInfo.publicKey};
    }

    private m_state: TCPNetworkInterfaceState = TCPNetworkInterfaceState.none;
    private m_local?: Endpoint;
    private m_remote?: Endpoint;
    private m_decoder: TCPPackageDecoder;
    private m_streaming: boolean;
    private m_socket?: net.Socket;
    private m_bPause: boolean = false;
    private m_bQueue: boolean = false;
    private m_listener?: (data: Buffer[]) => void;
    private m_keys: Map<string, {peerid: Peerid, key: Buffer}> = new Map(); 
    private m_recordDecoder?: RecordDecoder;
}