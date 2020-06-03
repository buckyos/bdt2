import * as dgram from 'dgram';
import * as assert from 'assert';
import { LogShim, Peerid, Endpoint, EndpointIPVersion, EndpointProtocol, ErrorCode, LoggerInstance, stringifyErrorCode } from '../../base';
import { createUDPPackageDecoder, UDPPackageDecoder, PackageDecryptKeyDelegate, createPackageDispatcher, IndexPackageDispatcher, createIndexPackageDispatcher, PackageCommand, MTU } from '../../protocol';
import { NetworkInterface } from './interface';
import { LocalPeer } from '../local_peer';
import { PeerHistoryManager } from '../peer_history';

export class UDPNetworkInterface extends NetworkInterface implements PackageDecryptKeyDelegate {
    constructor(options: {
        logger: LoggerInstance, 
        localPeer: LocalPeer, 
        historyManager: PeerHistoryManager
    }) {
        super(options);
        this.m_decoder = createUDPPackageDecoder(this.m_logger, this);
        this.m_indexPackageDispatcher = createIndexPackageDispatcher(this.m_logger);
    }

    async init(options: {
        endpoint: Endpoint
    }): Promise<ErrorCode> {
        this.m_local = options.endpoint;
        this.m_logger = new LogShim(this.m_logger).bind(`[UDPInterface: ${options.endpoint.toString()}]`, true).log;
        this.m_dispatcher = createPackageDispatcher(this.m_logger);
        this.m_logger.debug(`begin init udp interface on endpoint ${options.endpoint.toString()}`);
        if (!this.m_local) {
            this.m_logger.error(`udpNetworkInterface init fail for interface initialized`);
            return ErrorCode.invalidState;
        }
        if (options.endpoint.protocol !== EndpointProtocol.udp) {
            this.m_logger.error(`udpNetworkInterface init fail for invalid endpoint`);
            return ErrorCode.invalidParam;
        }
        this.m_logger.debug(`query all existing keys from history manager`);
        const gkr = await this.m_historyManager.getKey({});
        if (gkr.err) {
            this.m_logger.error(`init udp interface failed for get keys failed ${stringifyErrorCode(gkr.err)}!`);
            return gkr.err;
        }

        this.m_logger.debug(`history  key len: ${gkr.keys!.size}`);
        for (const [strHash, {peerid, key}] of gkr.keys!.entries()) {
            this.m_logger.info(`add existing key ${strHash} to interface`);
            let stub = this.m_keys.get(strHash);
            if (stub) {
                ++stub.ref;
            } else {
                stub = {
                    key, 
                    peerid, 
                    ref: 1
                };
                this.m_keys.set(strHash, stub);
            }
        }

        this.m_historyManager.on('keyAdded', (peerid: Peerid, keyHash: Buffer, key: Buffer) => {
            this.m_logger.info(`add newly added key ${keyHash.toString()} to interface for key added from other interface`);
            this.addKey({peerid, keyHash, key});
        });
        this.m_historyManager.on('keyExpired', (keyHash: Buffer) => {
            this.m_logger.info(`release key ${keyHash.toString()} from interface for expired`);
            this.removeKey(keyHash);
        });
        
        return new Promise<ErrorCode>((resolve) => {
            let bindSucc: boolean = false;
            const socket = dgram.createSocket({
                type: this.m_local!.ipVersion === EndpointIPVersion.ipv4 ? 'udp4' : 'udp6', 
                /*reuseAddr: true*/
            });

            socket.once('listening', () => {
                socket.on('message', (msg, rinfo) => {
                    if (msg.length > MTU) {
                        return;
                    }
                    
                    let {err, packages} = this.m_decoder.decode([msg]);
                    if (err) {
                        this.m_logger.debug(`decode package failed for ${stringifyErrorCode(err)} content: ${msg.toString('hex')}`);
                        return ;
                    }
                    if (packages && packages.length > 0) {
                        let epop = Object.create(null);
                        epop.ipVersion = this.m_local!.ipVersion;
                        epop.address = rinfo.address;
                        epop.port = rinfo.port;
                        epop.protocol = EndpointProtocol.udp;
                        let fromep = new Endpoint();
                        const _err = fromep.fromObject(epop);
                        if (_err) {
                            assert(false, `udp rinfo to endpoint failed ${stringifyErrorCode(_err)}`);
                            return ;
                        }

                        let firstPkg = packages[0];
                        if (firstPkg.cmdType === PackageCommand.exchangeKey) {
                            // exchangeKey只要第一时间添加到keyManager即可，不需要处理
                            this.m_historyManager.addKey({
                                peerid: firstPkg.__encrypt.peerid, 
                                key: firstPkg.__encrypt.key, 
                                keyHash: firstPkg.__encrypt.keyHash, 
                            });
                            packages.splice(0, 1);
                        }

                        packages.forEach((pkg) => {
                            if (pkg) {
                                pkg.__fromEndpoint = fromep;
                                pkg.__interface = this;
                                pkg.__recvTime = process.hrtime();
                                let {handled} = this.m_indexPackageDispatcher.dispatch(pkg);
                                if (!handled) {
                                    this.dispatcher.dispatch(pkg);
                                }
                            }
                        });
                    }
                });
                bindSucc = true;
                this.m_logger.info(`[udp_interface]: socket bind udp ${this.m_local!.address}:${this.m_local!.port} success`);

                this.m_socket = socket;
                resolve(ErrorCode.success);
            });
    
            socket.once('close', () => setImmediate(() => {
                    // <TODO>
                }
            ));
    
            socket.on('error', (error) => {
                setImmediate(() => {
                    // <TODO>
                });

                socket.close();
                if (bindSucc) {
                    this.m_logger.warn(`[udp_interface]: socket error ${this.m_local!.address}:${this.m_local!.port}, error:${error}.`);
                    return;
                } else {
                    this.m_logger.warn(`[udp_interface]: socket bind ${this.m_local!.address}:${this.m_local!.port} failed, error:${error}.`);
                    resolve(ErrorCode.localBindFailed);
                }
            });
            socket.bind(this.m_local!.port, this.m_local!.address);
        });
    }

    async close(): Promise<ErrorCode> {
        return new Promise<ErrorCode>((resolve) => {
            if (!this.m_socket) {
                resolve(ErrorCode.notFound);
            }
            this.m_socket!.close(() => {
                this.m_socket = undefined;
                resolve(ErrorCode.success);
            });
        });
    }

    pause(): void {
        
    }
    resume(): void {

    }

    on(event: 'error', listener: (options: {interface: UDPNetworkInterface, error: ErrorCode, to?: Endpoint}) => void): this;
    on(event: string, listener: (...args: any[]) => void): this {
        super.on(event, listener);
        return this;
    }

    send(content: Buffer, to: Endpoint): {err: ErrorCode} {
        if (!this.m_socket) {
            this.m_logger.error(`send message failed: this.m_socket undefined`);
            return {err: ErrorCode.invalidState};
        }

        if (!to || !to.port || !to.address) {
            this.m_logger.error(`send message failed invalid endpoint`);
            return {err: ErrorCode.invalidEndpoint};
        }

        try {
            this.m_socket.send(content, to.port, to.address);
            return {err: ErrorCode.success};
        } catch (error) {
            this.m_logger.warn(`send message to ${to.toString()} failed: ${error.name}|${error.message}`);
            return {err: ErrorCode.exception};
        }
    }

    get local(): Endpoint {
        return this.m_local!;
    }

    addKey(options: {
        peerid: Peerid, 
        keyHash: Buffer, 
        key: Buffer
    }) {
        const strHash = options.keyHash.toString();
        let stub = this.m_keys.get(strHash);
        if (stub) {
            ++stub.ref;
            return;
        }
        stub = {
            key: options.key, 
            peerid: options.peerid, 
            ref: 1
        };
        this.m_keys.set(strHash, stub);
    }   

    removeKey(keyHash: Buffer) {
        const strHash = keyHash.toString();
        let stub = this.m_keys.get(strHash);
        if (!stub) {
            assert(false, `remove key not exists`);
            return ;
        }
        --stub.ref;
        if (!stub.ref) {
            this.m_keys.delete(strHash);
        }
    }

    queryKey(keyHash: Buffer): {err: ErrorCode, key?: Buffer, peerid?: Peerid} {
        const stub = this.m_keys.get(keyHash.toString());
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

    get indexPackageDispatcher(): IndexPackageDispatcher {
        return this.m_indexPackageDispatcher;
    }

    private m_decoder: UDPPackageDecoder;
    private m_local?: Endpoint;
    private m_socket?: dgram.Socket;
    private m_keys: Map<string, {key: Buffer, peerid: Peerid, ref: number}> = new Map();
    private m_indexPackageDispatcher: IndexPackageDispatcher;
}
