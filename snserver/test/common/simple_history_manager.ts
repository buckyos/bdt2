import { ErrorCode, Peerid, Peer } from '../../src/base';
import { PeerHistoryManager, PeerEstablishLog } from '../../src/framework/peer_history';

export class NoImplPeerHistoryManager extends PeerHistoryManager {
    async createKey(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, key?: Buffer, newKey?: boolean}> {
        return {err: ErrorCode.noImpl};
    }

    async getKey(options: {
        peerid?: Peerid, 
        keyHash?: Buffer, 
    }): Promise<{err: ErrorCode, keys?: Map<string, {peerid: Peerid, key: Buffer}>}> {
        return {err: ErrorCode.success, keys: new Map()};
    }

    async addKey(options: {
        peerid: Peerid,
        keyHash: Buffer, 
        key: Buffer, 
        expire: number
    }): Promise<ErrorCode> {
        return ErrorCode.noImpl;
    }

    async logEstablish(options: {
        log: PeerEstablishLog, 
        expire: number
    }): Promise<ErrorCode> {
        return ErrorCode.noImpl;
    }

    async getEstablishLog(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, logs?: PeerEstablishLog[]}> {
        return {err: ErrorCode.success, logs: []};
    }

    async cachePeerInfo(options: {
        peer: Peer;
        expire: number;
    }): Promise<ErrorCode> {
        return ErrorCode.success;
    }

    async getPeerInfo(options: {
        peerid: Peerid
    }): Promise<{err: ErrorCode, peer?: Peer}> {
        return {err: ErrorCode.notFound};
    }
}