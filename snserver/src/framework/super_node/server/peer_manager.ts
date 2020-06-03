import { processUptime, Peerid, PeerConstInfo, PeerMutableInfo, ErrorCode, LoggerInstance } from '../../../base';
import { PeerInfo } from '../../../js-c/protocol';
import { Peer } from './peer';
import { SuperNodeTransport } from '../transport';

export type PeerManagerConfig = {
    timeout: number,
    capacity: number
};

export class PeerManager {
    constructor(options: {
        logger: LoggerInstance, 
        config: PeerManagerConfig;
    }) {
        this.m_logger = options.logger;
        this.m_config = options.config;
        this.m_lastKnockTime = processUptime();
    }

    peerHeartbeat(peerid: Buffer | undefined, peerInfo: PeerInfo): Peer | undefined {
        peerid = peerid || peerInfo ? peerInfo.peerid : undefined;
        if (!peerid) {
            return;
        }
        const peeridStr = peerid.toString('hex');
        let peer: Peer | undefined = this.m_activePeers.get(peeridStr);
        if (!peer) {
            // 从待定区转移到活跃区
            peer = this.m_knockPeers.get(peeridStr);
            if (peer) {
                this.m_knockPeers.delete(peeridStr);
                this.m_activePeers.set(peeridStr, peer);
            }
        }

        if (!peer) {
            if (peerInfo) {
                peer = new Peer({
                    logger: this.m_logger,
                    peerInfo,
                });
                this.m_activePeers.set(peeridStr, peer);
            } else {
                return;
            }
        } else {
            if (peerInfo && peerInfo.endpointArray) {
                peer.updatePeerInfo(peerInfo);
            }
        }

        peer.heartbeat();
        const now = peer.lastActiveTime;

        // 待定区淘汰，活跃区切换为待定区
        if (now - this.m_lastKnockTime > this.m_config.timeout) {
            // <TODO> 如果要支持TCP长连接保持ping，这里需要关闭连接
            this.m_knockPeers = this.m_activePeers;
            this.m_activePeers = new Map();
            this.m_lastKnockTime = now;
        }

        return peer;
    }

    getPeer(peerid: Buffer): Peer | undefined {
        const peeridStr = peerid.toString('hex');
        return this.m_activePeers.get(peeridStr) || this.m_knockPeers.get(peeridStr);
    }

    destroy() {
        // <TODO> 如果要支持TCP长连接保持ping，这里需要关闭连接，ping暂时不支持tcp
        this.m_activePeers = new Map();
        this.m_knockPeers = new Map();
    }

    private m_logger: LoggerInstance;
    private m_knockPeers: Map<string, Peer> = new Map(); // 淘汰待定peer
    private m_activePeers: Map<string, Peer> = new Map(); // 活跃peer
    private m_lastKnockTime: number;
    private m_config: PeerManagerConfig;
}
