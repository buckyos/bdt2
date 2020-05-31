import * as net from 'net';
import { LogShim, Endpoint, ErrorCode, LoggerInstance, stringifyErrorCode, Peerid } from '../../base';
import { TCPNetworkInterface } from './tcp_interface';
import { PackageDispatcher, createPackageDispatcher, PackageCommand } from '../../protocol';
import { LocalPeer } from '../local_peer';
import { PeerHistoryManager } from '../peer_history';

export class TCPNetworkListener {
    public static WAIT_TCP_PACKAGE_TIME_OUT: number = 5 * 1000;
    constructor(options: {logger: LoggerInstance, localPeer: LocalPeer, historyManager: PeerHistoryManager, timeout?: number}) {
        this.m_logger = options.logger;
        this.m_localPeer = options.localPeer;
        this.m_historyManager = options.historyManager;
        this.m_timeout = options.timeout!;
        if (!this.m_timeout) {
            this.m_timeout = TCPNetworkListener.WAIT_TCP_PACKAGE_TIME_OUT;
        }
    }

    async init(options: {
        endpoint: Endpoint
    }): Promise<ErrorCode> {
        this.m_local = options.endpoint;
        this.m_logger = new LogShim(this.m_logger).bind(`[TCPListener: ${this.m_local.toString()}]`, true).log;
        this.m_dispatcher = createPackageDispatcher(this.m_logger);
        this.m_logger.debug(`query all existing keys from history manager`);
        const gkr = await this.m_historyManager.getKey({});
        if (gkr.err) {
            this.m_logger.error(`init udp interface failed for get keys failed ${stringifyErrorCode(gkr.err)}!`);
            return gkr.err;
        }
        this.m_keys = gkr.keys!;
        this.m_historyManager.on('keyAdded', (peerid: Peerid, keyHash: Buffer, key: Buffer) => {
            const strHash = keyHash.toString();
            if (!this.m_keys.has(strHash)) {
                this.m_logger.info(`add existing key ${strHash} to interface for key added from other interface`);
                this.m_keys.set(strHash, {key, peerid});
            }
        });
        this.m_historyManager.on('keyExpired', (keyHash: Buffer) => {
            const strHash = keyHash.toString();
            this.m_logger.info(`release key ${strHash} from interface for expired`);
            this.m_keys.delete(strHash);
        });
        return new Promise<ErrorCode>((resolve) => {
            this.m_server = net.createServer();
            this.m_server.on('connection', (socket: net.Socket) => {
                this.m_logger.info(`'${this.m_localPeer.peerid.toString()} get connection`);
                let tcpNetworkInterface = new TCPNetworkInterface({logger: this.m_logger, localPeer: this.m_localPeer, historyManager: this.m_historyManager});
                tcpNetworkInterface.initFromListener({socket, keys: this.m_keys});
                tcpNetworkInterface.setTimeout(this.m_timeout);
                this.m_interfaces.push(tcpNetworkInterface);

                let ret: any;
                let onHandler = () => {
                    tcpNetworkInterface.dispatcher.removeHandler({cookie: ret.cookie!});
                    this._removeInterface(tcpNetworkInterface);

                    tcpNetworkInterface.removeListener('error', onHandler);
                    tcpNetworkInterface.removeListener('close', onHandler);
                };

                ret = tcpNetworkInterface.dispatcher.addHandler({
                    handler: {
                        handler: (pkg: {cmdType: number} & any): {handled?: boolean} => {
                            tcpNetworkInterface.setTimeout(0);

                            pkg.__interface = tcpNetworkInterface;
                            setImmediate (() => {
                                const {handled} = this.m_dispatcher!.dispatch(pkg);
                                if (!handled) {
                                    tcpNetworkInterface.close();
                                } else {
                                    onHandler();
                                }
                            });

                            return {handled: true};
                        }
                    }, 
                    description: `TCPListener on first package from TCPInterface`
                });

                tcpNetworkInterface.on('error', () => tcpNetworkInterface.close());
                tcpNetworkInterface.once('close', onHandler);
                // tcpNetworkInterface.once('end', onHandler);
            });

            // listen错误， 端口冲突会触发
            this.m_server.on('error', (error: Error) => {
                this.m_logger.error(`[tcpListen] server error ${error.message}`);
                resolve(ErrorCode.localBindFailed);
            });

            this.m_server.listen({
                host: options.endpoint.address,
                port: options.endpoint.port,
            });

            this.m_server.once('listening', () => {
                this.m_logger.info(`[tcpListen] socket bind on ${options.endpoint.address} ${options.endpoint.port}`);
                resolve(ErrorCode.success);
            });
        });
    }

    get local(): Endpoint {
        return this.m_local!;
    }

    get dispatcher(): PackageDispatcher {
        return this.m_dispatcher!;
    }

    _removeInterface(_interface: TCPNetworkInterface) {
        for (let i = 0; i < this.m_interfaces.length; i++) {
            if (this.m_interfaces[i] === _interface) {
                this.m_interfaces.splice(i, 1);
            }
        }
    }

    protected m_logger: LoggerInstance;
    private m_local?: Endpoint;
    private m_server?: net.Server;
    private m_dispatcher?: PackageDispatcher;
    private m_interfaces: TCPNetworkInterface[] = [];
    private m_timeout: number;
    private m_localPeer: LocalPeer;
    private m_historyManager: PeerHistoryManager;
    private m_keys: Map<string, {key: Buffer, peerid: Peerid}> = new Map();
}
