import { PackageDispatcher, createPackageDispatcher } from '../../protocol';
import { ErrorCode, Endpoint } from '../../base';
import { LoggerInstance } from 'winston';
import { EventEmitter } from 'events';
import { LocalPeer } from '../local_peer';
import { PeerHistoryManager } from '../peer_history';

export abstract class NetworkInterface extends EventEmitter {
    constructor(options: {logger: LoggerInstance, localPeer: LocalPeer, historyManager: PeerHistoryManager}) {
        super();
        this.m_logger = options.logger;
        this.m_localPeer = options.localPeer;
        this.m_historyManager = options.historyManager;
    }
    get dispatcher(): PackageDispatcher {
        return this.m_dispatcher!;
    }

    abstract pause(): void;
    abstract resume(): void;
    abstract close(): Promise<ErrorCode>;
    abstract get local(): Endpoint;

    protected m_logger: LoggerInstance;
    protected m_dispatcher?: PackageDispatcher;
    protected m_localPeer: LocalPeer;
    protected m_historyManager: PeerHistoryManager;
}
