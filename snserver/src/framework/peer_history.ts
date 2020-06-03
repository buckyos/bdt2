import { Peerid, Endpoint, ErrorCode, LoggerInstance, digest, Peer, currentTime } from '../base';
import { EventEmitter } from 'events';
import * as crypto from 'crypto';
import { PackageCrypto } from '../protocol/package_define';

export enum ConnectType {
    none = 0,
    tcpDirect = 1,
    tcpReverse = 2,
    udp = 3,
}

export type PeerEstablishLog = {
    peerid: Peerid, 
    local: Endpoint, 
    remote: Endpoint,
    connectType: ConnectType,
    time?: number,
};

export abstract class PeerHistoryManager extends EventEmitter {
    on(event: 'keyExpired', listener: (keyHash: Buffer) => void): this;
    on(event: 'keyAdded', listener: (peerid: Peerid, keyHash: Buffer, key: Buffer) => void): this;
    on(event: string, listener: (...args: any[]) => void): this {
        super.on(event, listener);
        return this;
    }

    abstract createKey(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, keyHash?: Buffer, key?: Buffer, newKey?: boolean}>;

    abstract getKey(options: {
        peerid?: Peerid, 
        keyHash?: Buffer, 
    }): Promise<{err: ErrorCode, keys?: Map<string, {peerid: Peerid, key: Buffer}>}>;

    abstract addKey(options: {
        peerid: Peerid,
        keyHash: Buffer, 
        key: Buffer, 
    }): Promise<ErrorCode>;

    abstract logEstablish(options: {
        log: PeerEstablishLog
    }): Promise<ErrorCode>;

    abstract getEstablishLog(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, logs?: PeerEstablishLog[]}>;

    abstract cachePeerInfo(options: {
        peer: Peer
    }): Promise<ErrorCode>;

    abstract getPeerInfo(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, peer?: Peer}>;
}

export type PeerHistoryManagerConfig = {
    keyExpire: number;
    peerExpire: number;
    logExpire: number;
};

// TODO: 管理和remote的历史信息， 比如产生的aes key；成功连接的路径等等;
// TODO: 先确定接口和上层逻辑， 全内存实现， 不持久化
export class PeerHistoryManagerImpl extends PeerHistoryManager {
    constructor(options: {
        logger: LoggerInstance,
        config: PeerHistoryManagerConfig
    }) {
        super();
        this.m_config = options.config;
    }
    private m_config: PeerHistoryManagerConfig;

    init(options: {
    }): ErrorCode {
        return ErrorCode.success;
    }

    // TODO: 记得这里一定要实现成不可重入的， createKey返回之前，重入的其他createKey调用应该返回同一结果
    async createKey(options: {
        peerid: Peerid,
    }): Promise<{err: ErrorCode, keyHash?: Buffer, key?: Buffer, newKey?: boolean}> {
        const gkr = await this.getKey({
            peerid: options.peerid
        });
        if (gkr.err) {
            return {err: gkr.err};
        }
        if (gkr.keys!.size) {
            for (const [_keyHash, {key}] of gkr.keys!.entries()) {
                return {err: ErrorCode.success, keyHash: Buffer.from(_keyHash), key};
            }
        }
        const newKey = crypto.randomBytes(32);
        // hash统一使用sha256
        const keyHash = PackageCrypto.keyHash(newKey);
        const err = await this.addKey({peerid: options.peerid, keyHash, key: newKey});
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, keyHash, key: newKey, newKey: true};
    }

    async getKey(options: {
        peerid?: Peerid, 
        keyHash?: Buffer, 
    }): Promise<{err: ErrorCode, keys?: Map<string, {peerid: Peerid, key: Buffer}>}> {
        if (options.peerid) {
            const strPeerid = options.peerid.toString();
            const peerKeys = this.m_peerKeyHashes.get(strPeerid);
            if (!peerKeys) {
                return {err: ErrorCode.success, keys: new Map()};
            }
            let keyHashes = new Set();
            if (options.keyHash) {
                if (!peerKeys.has(options.keyHash.toString())) {
                    return {err: ErrorCode.success, keys: new Map()};
                }
                keyHashes.add(options.keyHash);
            } else {
                keyHashes = peerKeys;
            }
            let keys = new Map();
            for (const hash of keyHashes) {
                keys.set(hash, {peerid: options.peerid, key: this.m_hash2Keys.get(hash)!.key});
            }
            return {err: ErrorCode.success, keys};
        } 
        if (options.keyHash) {
            let keys = new Map();
            const peerAndKey = this.m_hash2Keys.get(options.keyHash.toString());
            if (peerAndKey) {
                keys.set(options.keyHash, peerAndKey);
            }
            return {err: ErrorCode.success, keys};
        } else {
            let keys = new Map(this.m_hash2Keys.entries());
            return {err: ErrorCode.success, keys};
        }
    }

    async addKey(options: {
        peerid: Peerid,
        keyHash: Buffer, 
        key: Buffer
    }): Promise<ErrorCode> {
        const strHash = options.keyHash.toString();
        if (this.m_hash2Keys.has(strHash)) {
            return ErrorCode.success;
        }
        const strPeerid = options.peerid.toString();
        let peerKeys = this.m_peerKeyHashes.get(strPeerid);
        if (!peerKeys) {
            peerKeys = new Set();
            this.m_peerKeyHashes.set(strPeerid, peerKeys);
        }
        
        peerKeys.add(strHash);
        this.m_hash2Keys.set(strHash, {key: options.key, peerid: options.peerid});
        this.emit('keyAdded', options.peerid, options.keyHash, options.key);
        return ErrorCode.success;
    }
    private m_hash2Keys: Map<string, {peerid: Peerid, key: Buffer}> = new Map();
    private m_peerKeyHashes: Map<string, Set<string>> = new Map();

    async logEstablish(options: {
        log: PeerEstablishLog
    }): Promise<ErrorCode> {
        const strPeerid = options.log.peerid.toString();
        let logs = this.m_establishLogs.get(strPeerid);
        if (!logs) {
            logs = [];
            this.m_establishLogs.set(strPeerid, logs);
        }
        options.log.time = options.log.time || currentTime();
        logs.unshift(options.log);
        return ErrorCode.success;
    }

    async getEstablishLog(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, logs?: PeerEstablishLog[]}> {
        const strPeerid = options.peerid.toString();
        let logs = this.m_establishLogs.get(strPeerid);
        if (!logs) {
            logs = [];
        }
        return {err: ErrorCode.success, logs};
    }
    private m_establishLogs: Map<string,  PeerEstablishLog[]> = new Map();

    async cachePeerInfo(options: {
        peer: Peer;
    }): Promise<ErrorCode> {
        this.m_cachedPeers.set(options.peer.peerid.toString(), options.peer);
        return ErrorCode.success;
    }

    async getPeerInfo(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, peer?: Peer}> {
        const peer = this.m_cachedPeers.get(options.peerid.toString());
        if (!peer) {
            return {err: ErrorCode.notFound};
        }
        return {err: ErrorCode.success, peer};
    }
    private m_cachedPeers: Map<string, Peer> = new Map();
}
