import { Endpoint, EndpointList, ErrorCode, LoggerInstance, EndpointProtocol } from '../../base';
import { TCPNetworkInterface } from './tcp_interface';
import { TCPNetworkListener } from './tcp_listener';
import { UDPNetworkInterface } from './udp_interface';
import { LocalPeer } from '../local_peer';
import { PeerHistoryManager } from '../peer_history';

export class NetworkManager {
    constructor(options: {logger: LoggerInstance }) {
        this.m_logger = options.logger;
    }

    async init(options: {
        localPeer: LocalPeer, 
        historyManager: PeerHistoryManager
    }): Promise<{err: ErrorCode, failedEndpoints?: EndpointList}> {
        this.m_localPeer = options.localPeer;
        this.m_historyManager = options.historyManager;
        let initOpts: Array<Promise<ErrorCode> > = [];
        let bindInterfaces: Array<{local: Endpoint}&any> = [];
        
        const eplist = options.localPeer.mutableInfo.endpointList;
        const tcpEndpointList = eplist.list({protocol: EndpointProtocol.tcp});
        if (tcpEndpointList.length > 0) {
            const tcpEndpoint = tcpEndpointList[0];
            let tcpListener = new TCPNetworkListener({
                logger: this.m_logger, 
                localPeer: this.m_localPeer, 
                historyManager: this.m_historyManager
            });
    
            bindInterfaces.push(tcpListener);
            initOpts.push(tcpListener.init({
                endpoint: tcpEndpoint
            }));
        }

        const udpEndpointList = eplist.list({protocol: EndpointProtocol.udp});
        if (udpEndpointList.length > 0) {
            udpEndpointList.forEach((ep: Endpoint) => {
                let udpInterface = new UDPNetworkInterface({
                    logger: this.m_logger, 
                    localPeer: this.m_localPeer!, 
                    historyManager: this.m_historyManager!
                });
                bindInterfaces.push(udpInterface);
                initOpts.push(udpInterface.init({
                    endpoint: ep
                }));
            });
        }

        if (initOpts.length === 0) {
            this.m_logger.error('[networkManager] No invalid endpoint found.');
            return {err: ErrorCode.invalidParam, failedEndpoints: eplist};
        }

        const errors = await Promise.all(initOpts);
        let failedEndpoints: EndpointList = new EndpointList();
        for (let i = 0; i < errors.length; i++) {
            if (errors[i]) {
                this.m_logger.error(`[networkManager] interface(${bindInterfaces[i].local}) init 失败 err: ${errors[i]}`);
                failedEndpoints.addEndpoint(bindInterfaces[i].local);
            } else {
                if (bindInterfaces[i] instanceof TCPNetworkListener) {
                    this.m_tcpListener = bindInterfaces[i];
                } else {
                    this.m_udpInterfaces.push(bindInterfaces[i]);
                }
            }
        }

        if (!this.m_tcpListener && !this.m_udpInterfaces.length) {
            return { err: ErrorCode.localBindFailed, failedEndpoints };
        }

        let boundAddresses: Endpoint[] = [];
        if (this.m_tcpListener) {
            const ep = this.m_tcpListener.local;
            boundAddresses.push(new Endpoint({
                ipVersion: ep.ipVersion,
                address: ep.address
            }));
        }
        for (const ui of this.m_udpInterfaces) {
            const ep = new Endpoint({
                ipVersion: ui.local.ipVersion, 
                address: ui.local.address
            });
            let dup = false;
            for (const _ee of boundAddresses) {
                if (ep.equal(_ee)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                boundAddresses.push(ep);
            }
        }
        this.m_localAddresses = boundAddresses;
        this.m_logger.info(`networkmanager init succ`);
        return { err: ErrorCode.success };
    }

    get localAddresses(): Endpoint[] {
        return this.m_localAddresses;
    }

    createTCPInterface(): {err: ErrorCode, interface?: TCPNetworkInterface} {
        let tcpInterface: TCPNetworkInterface = new TCPNetworkInterface({
            logger: this.m_logger, 
            localPeer: this.m_localPeer!, 
            historyManager: this.m_historyManager!
        });
        return {err: ErrorCode.success, interface: tcpInterface};
    }

    createUDPInterface(): {err: ErrorCode, interface?: UDPNetworkInterface} {
        let udpInterface: UDPNetworkInterface = new UDPNetworkInterface({
            logger: this.m_logger, 
            localPeer: this.m_localPeer!, 
            historyManager: this.m_historyManager!
        });
        return {err: ErrorCode.success, interface: udpInterface};
    }

    // get udpInterface(): UDPNetworkInterface {
    //     return this.m_udpInterface!;
    // }

    get tcpListener(): TCPNetworkListener|undefined {
        return this.m_tcpListener;
    }

    get udpInterfaces(): Array<UDPNetworkInterface> {
        return this.m_udpInterfaces;
    }

    get historyManager(): PeerHistoryManager {
        return this.m_historyManager!;
    }

    get localPeer(): LocalPeer {
        return this.m_localPeer!;
    }
    
    private m_logger: LoggerInstance;
    private m_udpInterfaces: Array<UDPNetworkInterface> = [];
    private m_tcpListener?: TCPNetworkListener;
    private m_localPeer?: LocalPeer;
    private m_historyManager?: PeerHistoryManager;
    private m_localAddresses: Endpoint[] = [];
}
