import { LoggerInstance } from 'winston';
import { NetworkManager, UDPNetworkInterface, LocalPeer, PeerHistoryManager } from '../../src/framework';
import { Endpoint, ErrorCode, EndpointIPVersion, EndpointProtocol } from '../../src/base';
import { PackageHandlerPriority } from '../../src/protocol';
import * as dgram from 'dgram';

type MappingEntry = {
    _interface: UDPNetworkInterface,
    srcPort: number,
    proxyPort: number,
    destPort?: number,
};

export class NatSim {
    protected m_logger: LoggerInstance;
    protected m_srcToProxy: Map<number, MappingEntry[]> = new Map();
    protected m_srcToDest: Map<number, Set<number>> = new Map();
    protected m_destToProxy: Map<number, MappingEntry> = new Map();
    protected m_useablePort: number = 50000;
    protected m_symmetric = false;
    constructor(options: {logger: LoggerInstance}) {
        this.m_logger = options.logger;
    }

    set symmetric(b: boolean) {
        this.m_symmetric = b;
    }

    async _makeMapping(_interface: UDPNetworkInterface): Promise<MappingEntry> {
        let real = _interface.send;

        let newInterface: UDPNetworkInterface = new UDPNetworkInterface({
            logger: this.m_logger,
            localPeer: ((_interface as any).m_localPeer) as LocalPeer,
            historyManager: ((_interface as any).m_historyManager) as PeerHistoryManager,
        });
        
        let port = this.m_useablePort++;
        await newInterface.init({
            endpoint:
                new Endpoint({
                    port,
                    address: '127.0.0.1',
                    ipVersion: EndpointIPVersion.ipv4,
                    protocol: EndpointProtocol.udp
                })
        });
        newInterface.dispatcher.addHandler({
            handler: {
                handler: (pkg: any): { handled: boolean } => {
                    pkg.__fromWall = true;
                    _interface.dispatcher.dispatch(pkg);
                    return { handled: true };
                }
            },
            description: '[test] newInterface ',
        });

        return {
            _interface: newInterface,
            srcPort: _interface.local.port,
            proxyPort: port,
        };
    }

    async _makeMappings(_interface: UDPNetworkInterface, count: number) {
        let entrys: MappingEntry[] = [];
        for (let i = 0; i < count; i++) {
            let entry = await this._makeMapping(_interface);
            entrys.push(entry);
        }
        this.m_srcToProxy.set(_interface.local.port, entrys);
    }

    async wall(manager: NetworkManager) {
        let udpinterfaces: Array<UDPNetworkInterface> = ((manager as any).m_udpInterfaces) as Array<UDPNetworkInterface>;
        for (let _interface of udpinterfaces) {
            await this._makeMappings(_interface, 10);

            _interface.send = (content: Buffer, to: Endpoint): { err: ErrorCode } => {
                let entry1: MappingEntry;
                if (!this.m_symmetric) {
                    entry1 = (this.m_srcToProxy.get(_interface.local.port) as MappingEntry[])[0];
                } else {
                    if (!this.m_srcToDest.has(_interface.local.port)) {
                        this.m_srcToDest.set(_interface.local.port, new Set());
                    }
                    let sendports = this.m_srcToDest.get(_interface.local.port) as Set<number>;
                    if (!sendports.has(to.port)) {
                        sendports.add(to.port);
    
                        for (let entry of (this.m_srcToProxy.get(_interface.local.port) as MappingEntry[])) {
                            if (!entry.destPort) {
                                entry.destPort = to.port;
                                this.m_destToProxy.set(to.port, entry);
                                break;
                            }
                        }
                    }
    
                    entry1 = this.m_destToProxy.get(to.port) as MappingEntry;
                }

                this.m_logger.debug(`=============${_interface.local.port} send to ${to.port}, proxy: ${entry1.proxyPort}`);
                return entry1._interface.send(content, to);
            };

            _interface.dispatcher.addHandler({
                description: '[test]  wall interface ',
                handler: {
                    handler: (pkg: any): { handled: boolean } => {
                        if (!pkg.__fromWall) {
                            this.m_logger.debug(`=============${_interface.local.port} get from ${pkg.__fromEndpoint.port} cmdType=${pkg.cmdType}, 1`);
                            return { handled: true };
                        }
    
                        let sendPort = this.m_srcToDest.get(pkg.__fromEndpoint.port);
                        if (this.m_symmetric && (!sendPort || !sendPort.has(pkg.__fromEndpoint.port))) {
                            // local没有给对方发过数据，那么对方过来的数据，不转发
                            this.m_logger.debug(`=============${_interface.local.port} get from ${pkg.__fromEndpoint.port} cmdType=${pkg.cmdType}, 2`);
                            return { handled: true };
                        }
    
                        this.m_logger.debug(`=============${_interface.local.port} get from ${pkg.__fromEndpoint.port} cmdType=${pkg.cmdType}, 3`);
                        return { handled: false };
                    }
                },
                priority: PackageHandlerPriority.high + 100000
            });
        }
    }
}
