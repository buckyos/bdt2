import * as assert from 'assert';
import { processUptime, Peerid, PeerConstInfo, PeerMutableInfo, ErrorCode, LoggerInstance, Endpoint, EndpointList, EndpointOptions } from '../../../base';
import { NetworkManager, LocalPeer, TCPNetworkInterface, UDPNetworkInterface, PeerHistoryManager } from '../../../framework';
import { PeerManager, PeerManagerConfig } from './peer_manager';
import { Peer } from './peer';
import { PackageCommand, stringifyPackageCommand, PackageHandlerPriority, PackageDecryptOptions } from '../../../protocol';
import { SuperNodeTransport, TCPSuperNodeTransport, UDPSuperNodeTransport } from '../transport';
import { ResponseSender } from './response_sender';
import { ResendQueue, ResendQueueConfig } from './resend_queue';
import { SequenceCreator } from '../../sequence';
import { CryptoSession, SingleKeyCryptoSession } from '../../crypto_helper';
import { PeerInfo, signLocalPeerInfo } from '../../../js-c/protocol';

export type SuperNodeServerConfig = {
    tcpFreeInterval: number, 
    peerManager: PeerManagerConfig, 
    resendQueue: ResendQueueConfig
};

export class SuperNodeServer {
    constructor(options: {
        logger: LoggerInstance, 
        config: SuperNodeServerConfig
    }) {
        this.m_config = options.config;

        this.m_logger = options.logger;

        this.m_responseSender = new ResponseSender({
            logger: this.m_logger,
        });

        this.m_resendQueue = new ResendQueue({
            logger: this.m_logger, 
            config: this.m_config.resendQueue
        });

        this.m_peerManager = new PeerManager({
            logger: this.m_logger, 
            config: this.m_config.peerManager
        });
    }

    get localPeer(): PeerInfo {
        return this.m_signedLocalPeerInfo!;
    }

    get historyManager(): PeerHistoryManager {
        return this.m_networkManager!.historyManager;
    }

    init(options: {
        networkManager: NetworkManager,
        encrypted?: boolean,
    }): ErrorCode {
        if (this.m_timer) {
            return ErrorCode.invalidState;
        }
        this.m_networkManager = options.networkManager;
        this.m_encrypted = !!options.encrypted;

        let localPeerUnSigned: PeerInfo = {
            peerid: this.m_networkManager.localPeer.peerid.toObject(),
            constInfo: this.m_networkManager.localPeer.constInfo,
            endpointArray: this.m_networkManager.localPeer.mutableInfo.endpointList.list().map<[string, number]>((ep) => [ep.toString(), 0]), // [[endpoint, flag]]
			createTime: { h : 0, l : 0},
        };

        this.m_signedLocalPeerInfo = signLocalPeerInfo(localPeerUnSigned, this.m_networkManager.localPeer.secret);

        const handlers = {
            [PackageCommand.snPingReq]: (pkg: {cmdType: number}&any, transport: SuperNodeTransport, fromPeer: Peer | undefined) => this._onPingReq(pkg, transport, fromPeer),
            [PackageCommand.snCallReq]: (pkg: {cmdType: number}&any, transport: SuperNodeTransport, fromPeer: Peer | undefined) => this._onCallReq(pkg, transport, fromPeer),
            [PackageCommand.snCalledResp]: (pkg: {cmdType: number}&any, transport: SuperNodeTransport, fromPeer: Peer | undefined) => this._onCalledResp(pkg, transport, fromPeer),
        };

        const onRecvEncryptPackage = (pkg: {__encrypt: PackageDecryptOptions}&any, fromPeer: Peer | undefined, transport: SuperNodeTransport) => {
            if (pkg.__encrypt) {
                let cryptoSession: CryptoSession | undefined;
                if (fromPeer) {
                    cryptoSession = fromPeer.cryptoSession;
                }

                if (!cryptoSession) {
                    let peerid = pkg.fromPeerid;
                    let peerInfo = pkg.peerInfo;
                    if (!peerid || !peerInfo) {
                        if (fromPeer) {
                            peerid = fromPeer.peerid;
                            peerInfo = fromPeer.peerInfo;
                        }
                        if (!peerid || !peerInfo) {
                            return;
                        }
                    }
                    cryptoSession = new SingleKeyCryptoSession({
                        logger: this.m_logger,
                        historyManager: this.historyManager,
                        localPeer: this.m_networkManager!.localPeer,
                        remote: {
                            peerid,
                            publicKey: peerInfo.constInfo.publicKey,
                        },
                    });
                }

                if (!transport.cryptoSession) {
                    transport.cryptoSession = cryptoSession.clone().session!;
                    transport.cryptoSession.setInterface(pkg.__interface);
                }
                transport.cryptoSession.onRecvPackage(pkg);
            }
        };

        const listenUDP = () => {
            this.m_networkManager!.udpInterfaces.forEach((_udpInterface) => {
                const ret = _udpInterface.dispatcher.addHandler({
                    handler: {
                        index: {}, 
                        handler: (pkg: {cmdType: number}&any): {handled?: boolean} => {
                            const handler = handlers[pkg.cmdType];
                            if (!handler) {
                                return {handled: false};
                            }

                            // 加密模式下收到未加密包，直接丢弃
                            if (this.m_encrypted && !pkg.__encrypt) {
                                return {handled: true};
                            }

                            const remoteEndpoint = pkg.__fromEndpoint;
                            if (!pkg.fromPeerid) {
                                if (pkg.peerInfo) {
                                    pkg.fromPeerid = pkg.peerInfo.peerid;
                                }
                                if (pkg.cmdType !== PackageCommand.snCalledResp) {
                                    // 没错，这个包就是我的，但是缺少关键字段
                                    this.m_logger.warn(`Receive package(${pkg.sequence}|${pkg.cmdType}) from ${remoteEndpoint.toString()} without the field('fromPeerid')`);
                                    return {handled: true};
                                }
                            }

                            this.m_logger.debug(`[snServer] Receive package(${pkg.sequence}|${pkg.cmdType}) from ${remoteEndpoint.toString()}`);

                            let transport: SuperNodeTransport | undefined;
                            let fromPeer: Peer | undefined;

                            if (pkg.fromPeerid) {
                                fromPeer = this.m_peerManager.getPeer(pkg.fromPeerid);
                                if (fromPeer) {
                                    transport = fromPeer.findTransport(_udpInterface.local, remoteEndpoint);
                                }
                            }
                            transport = transport || new UDPSuperNodeTransport({interface: _udpInterface, remote: remoteEndpoint, logger: this.m_logger});

                            // 更新密钥
                            if (pkg.__encrypt) {
                                onRecvEncryptPackage(pkg, fromPeer, transport);
                            } else {
                                transport.cryptoSession = undefined;
                            }
                            handler(pkg, transport!, fromPeer);
                            return {handled: true};
                        }
                    }, 
                    description: `SuperNodeServer on all packages from UDPInterface`,
                    priority: PackageHandlerPriority.low,
                });

                this.m_udpHandles.push({
                    udpInterface: _udpInterface, cookie: ret.cookie!
                });
            });
        };

        const listenTCP = () => {
            if (!this.m_networkManager!.tcpListener) {
                return;
            }

            const listenerRet = this.m_networkManager!.tcpListener!.dispatcher.addHandler({
                handler: {
                    index: {cmdType: PackageCommand.snCallReq},
                    handler: (firstPkg: {cmdType: number}&any): {handled?: boolean} => {
                        let _interface: TCPNetworkInterface = firstPkg.__interface;
                        let transport: TCPSuperNodeTransport = new TCPSuperNodeTransport({
                            interface: _interface,
                            logger: this.m_logger
                        });
            
                        const remote = _interface.remote;
                        let peerid: Buffer;
                        let peeridStr: string;
                        let peerInfo: PeerInfo;
                        let sourcePeer: Peer | undefined;
            
                        this.m_tcpInterfaces.add(_interface);
        
                        const packageHandle = (pkg: {cmdType: number}&any): {handled?: boolean} => {
                            /*
                            const handler = handlers[pkg.cmdType];
                            if (!handler) {
                                return {handled: false};
                            }
                            */
        
                            // 加密模式下收到未加密包，直接丢弃
                            if (this.m_encrypted && !pkg.__encrypt) {
                                return {handled: true};
                            }

                            if (this.m_freeTCPInterfaces.delete(_interface)) {
                                this.m_tcpInterfaces.add(_interface);
                            }

                            peerInfo = pkg.peerInfo || peerInfo;
                            if (pkg.fromPeerid) {
                                let thisFromPeeridStr = pkg.fromPeerid.toString('hex');
                                assert(!peeridStr || peeridStr === thisFromPeeridStr);
                                peerid = peerid || pkg.fromPeerid;
                                peeridStr = thisFromPeeridStr;
                            } else {
                                assert(peerid);
                                if (!peerid) {
                                    this.m_logger.warn(`Receive package(${pkg.sequence}|${pkg.cmdType}) from ${remote.toString()} without the field('fromPeerid')`);
                                    return {handled: true}; // 虽然没处理，但也不会有其他地方处理
                                }

                                pkg.fromPeerid = peerid;
                            }
        
                            this.m_logger.debug(`Receive package(${pkg.sequence}|${pkg.cmdType}) from peer(${peeridStr}|${remote.toString()})`);
                            // 暂时TCP只被允许用于CALL，不被用于维持PEER和SN之间的长连接通信
                            // this.m_peerManager.peerHeartbeat(pkg.fromPeerid, pkg.constInfo, pkg.mutableInfo, transport);
                            // handler(pkg, transport);

                            // 更新密钥
                            if (pkg.__encrypt) {
                                sourcePeer = sourcePeer || this.m_peerManager.getPeer(peerid);
                                onRecvEncryptPackage(pkg, sourcePeer, transport);
                            } else {
                                // 指定空密钥
                                transport.cryptoSession = undefined;
                            }

                            this._onCallReq(pkg, transport, sourcePeer);
                            return {handled: true};
                        };

                        packageHandle(firstPkg);

                        let {cookie, err} = _interface.dispatcher.addHandler({
                            handler: {
                                index: {cmdType: PackageCommand.snCallReq}, 
                                handler: packageHandle,
                            },
                            description: `SuperNodeServer on ${stringifyPackageCommand(PackageCommand.snCallReq)} from TCPListener`
                        });
        
                        const removeHandler = () => {
                            if (cookie) {
                                transport.uinit();
                                _interface.dispatcher.removeHandler({cookie});
                                cookie = undefined;
                                if (!this.m_tcpInterfaces.delete(_interface)) {
                                    this.m_freeTCPInterfaces.delete(_interface);
                                }
                            }
                        };
                        _interface.once('close', () => {
                            removeHandler();
                            _interface.removeListener('error', removeHandler);
                            _interface.removeListener('end', removeHandler);
                        });
                        _interface.on('error', removeHandler);
                        _interface.once('end', removeHandler);
                        return {handled: true};
                    }
                }, 
                description: `SuperNodeServer on ${stringifyPackageCommand(PackageCommand.snCallReq)} from UDPInterface`,
                priority: PackageHandlerPriority.low,
            });
            this.m_tcpListenerCookie = listenerRet.cookie;
        };

        listenUDP();
        listenTCP();

        this.m_timer = setInterval(() => {
            const now = processUptime();
            this.m_resendQueue.onTimer();
            // 把活跃的TCP连接翻一下
            if (now - this.m_tcpLastClearTime >= this.m_config.tcpFreeInterval) {
                let willFree = this.m_freeTCPInterfaces;
                this.m_freeTCPInterfaces = this.m_tcpInterfaces;
                this.m_tcpInterfaces = new Set();
                willFree.forEach((tcpInterface) => {
                    tcpInterface.close();
                });
            }
        }, 200);

        return ErrorCode.success;
    }

    destroy() {
        if (this.m_timer) {
            clearInterval(this.m_timer);
            this.m_timer = undefined;
        }
        if (this.m_tcpListenerCookie) {
            this.m_networkManager!.tcpListener!.dispatcher.removeHandler({cookie: this.m_tcpListenerCookie});
            this.m_tcpListenerCookie = undefined;
        }
        this.m_udpHandles.forEach((handler) => {
            handler.udpInterface.dispatcher.removeHandler({cookie: handler.cookie!});
        });
        this.m_tcpInterfaces.forEach((tcpInterface) => {
            tcpInterface.close();
        });
        this.m_freeTCPInterfaces.forEach((tcpInterface) => {
            tcpInterface.close();
        });
        this.m_udpHandles = [];
        this.m_peerManager.destroy();
        this.m_resendQueue.clear();
        this.m_responseSender.destroy();
    }

    _onPingReq(pkg: { cmdType: number } & any, transport: SuperNodeTransport, fromPeer: Peer | undefined) {
        this.m_logger.info(`ping from peer(${pkg.fromPeerid.toString('hex')}|${transport.remoteEndpoint.toString()}),endpoints:${JSON.stringify(pkg.peerInfo.endpointArray)}`);
        if (!pkg.fromPeerid) {
            return;
        }
        
        if (fromPeer && pkg.sendTime < fromPeer.lastPkgSendTime) {
            return;
        }

        // 1.返回响应包
        const resp = {
            cmdType: PackageCommand.snPingResp,
            sequence: pkg.sequence,
            result: ErrorCode.success,
            peerInfo: this.m_signedLocalPeerInfo!,
            endpointArray: [transport.remoteEndpoint.toString()],
        };

        const logKey = `pingResp:${pkg.fromPeerid.toString('hex')}|${transport.remoteEndpoint.toString()}|seq:${resp.sequence}`;
        this.m_responseSender.send({id: pkg.fromPeerid, transport}, resp, logKey, true);

        // 2.更新节点信息
        // 优先采信加密信息，其次seq大的peerInfo
        if (fromPeer) {
            if (!pkg.peerInfo.signature) {
                if (fromPeer.peerInfo.signature || pkg.sendTime <= fromPeer.lastPkgSendTime) {
                    return;
                }
            } else {
                if (fromPeer.peerInfo.signature && pkg.sendTime <= fromPeer.lastPkgSendTime) {
                    return;
                }
            }
        }

        fromPeer = this.m_peerManager.peerHeartbeat(pkg.fromPeerid, pkg.peerInfo);
        fromPeer!.updateTransport(transport);
        fromPeer!.lastPkgSendTime = pkg.sendTime || fromPeer!.lastPkgSendTime;

        if (pkg.__encrypt && !fromPeer!.cryptoSession) {
            fromPeer!.cryptoSession = new SingleKeyCryptoSession({
                logger: this.m_logger,
                historyManager: this.historyManager,
                localPeer: this.m_networkManager!.localPeer,
                remote: {
                    peerid: pkg.fromPeerid,
                    publicKey: fromPeer!.peerInfo.constInfo.publicKey,
                },
            });
        }
    }

    _onCallReq(pkg: {cmdType: number}&any, transport: SuperNodeTransport, fromPeer: Peer | undefined) {
        if (!pkg.toPeerid) {
            this.m_logger.warn(`Receive package(${pkg.sequence}|${pkg.cmdType}) from peer(${pkg.fromPeerid.toString('hex')}|${transport.remoteEndpoint.toString()}) without the field('targetPeer')`);
            return;
        }

        let resp: any = {
            cmdType: PackageCommand.snCallResp,
            snPeerid: this.m_signedLocalPeerInfo!.peerid,
            sequence: pkg.sequence,
            result: ErrorCode.success,
        };

        const callRespLogKey = `callResp:${pkg.fromPeerid.toString('hex')}|from(${pkg.toPeerid.toString('hex')})|seq:${pkg.sequence}`;
        
        let fromPeerInfo = pkg.peerInfo;
        // 尽量用签名信息，防止有节点伪装加密节点连接其他节点
        if (!fromPeerInfo.signature && fromPeer && fromPeer.peerInfo.signature) {
             fromPeerInfo = fromPeer.peerInfo;
        }

        const targetPeer = this.m_peerManager.getPeer(pkg.toPeerid);
        if (targetPeer) {
            this.m_logger.info(`call from peer(${pkg.fromPeerid.toString('hex')}|${transport.remoteEndpoint.toString()}) to peer(${pkg.toPeerid.toString('hex')}),found endpoints:${JSON.stringify(targetPeer.peerInfo.endpointArray)}, alwaysCall:${pkg.alwaysCall}`);

            // 1.callResp
            resp.toPeerInfo = targetPeer.peerInfo;

            if (true || pkg.alwaysCall || !targetPeer!.isWan) {
                // 2.calledReq
                let calledReq: any = {
                    cmdType: PackageCommand.snCalledReq,
                    sequence: this.m_seqCreator.newSequence(),
                    toPeerid: pkg.toPeerid,
                    fromPeerid: pkg.fromPeerid,
                    snPeerid: this.m_signedLocalPeerInfo!.peerid,
                    peerInfo: pkg.peerInfo,
                };
                if (pkg.payload) {
                    calledReq.payload = pkg.payload;
                }
                if (pkg.reverseEndpointArray) {
                    calledReq.reverseEndpointArray = pkg.reverseEndpointArray;
                }

                const calledLogKey = `calledReq:${calledReq.fromPeerid.toString('hex')}|from(${pkg.toPeerid.toString('hex')})|seq:${pkg.sequence}|newSeq:${calledReq.sequence}`;
                this.m_resendQueue.send(targetPeer, calledReq, calledLogKey).then((errorCode: ErrorCode) => {
                    resp.result = errorCode;
                    this.m_responseSender.send({id: pkg.fromPeerid, transport}, resp, callRespLogKey);
                });                
            } else {
                resp.result = ErrorCode.success;
                this.m_responseSender.send({id: pkg.fromPeerid, transport}, resp, callRespLogKey);
            }
        } else {
            this.m_logger.info(`call from peer(${pkg.fromPeerid.toString('hex')}|${transport.remoteEndpoint.toString()}) to peer(${pkg.toPeerid.toString('hex')}), not found.`);
            // callResp(failed)
            resp.result = ErrorCode.notFound;
            this.m_responseSender.send({id: pkg.fromPeerid, transport}, resp, callRespLogKey);
        }
    }

    _onCalledResp(pkg: {cmdType: number}&any, transport: SuperNodeTransport, fromPeer: Peer | undefined) {
        this.m_logger.info(`called resp seq:${pkg.sequence}.`);
        this.m_resendQueue.confirm(pkg);
    }

    private m_peerManager: PeerManager;
    private m_networkManager?: NetworkManager;
    private m_responseSender: ResponseSender;
    private m_resendQueue: ResendQueue;
    private m_udpHandles: {udpInterface: UDPNetworkInterface, cookie: string}[] = [];
    private m_tcpInterfaces: Set<TCPNetworkInterface> = new Set();
    private m_freeTCPInterfaces: Set<TCPNetworkInterface> = new Set();
    private m_tcpLastClearTime: number = processUptime();
    private m_tcpListenerCookie?: string;
    private m_timer?: NodeJS.Timer;
    private m_seqCreator: SequenceCreator = new SequenceCreator();
    private m_encrypted: boolean = false;
    private m_signedLocalPeerInfo?: PeerInfo;
    private m_logger: LoggerInstance;
    private m_config: SuperNodeServerConfig;
}

function endpointArrayToStringArray(epArray: Endpoint[]) {
    let stringArray: string[] = [];
    for (let ep of epArray) {
        stringArray.push(ep.toString());
    }
    return stringArray;
}