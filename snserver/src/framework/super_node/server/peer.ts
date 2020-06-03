import { processUptime, Peerid, PeerConstInfo, PeerMutableInfo, Endpoint, ErrorCode, LoggerInstance } from '../../../base';
import { SuperNodeTransport } from '../transport';
import { PackageEncryptOptions } from '../../../protocol';
import { PeerInfo } from '../../../js-c/protocol';
import { CryptoSession } from '../../crypto_helper';

type uint64 = {
    h: number,
    l: number,
};

function compareUint64(left: uint64, right: uint64) {
    const hSub = left.h - right.h;
    if (hSub) {
        return hSub;
    } else {
        return left.l - right.l;
    }
}

export class Peer {
    constructor(options: {
        logger: LoggerInstance,
        peerInfo: PeerInfo
    }) {
        this.m_logger = options.logger;
        this.m_peeridStr = options.peerInfo.peerid.toString('hex');
        this.m_peerid = new Peerid();
        this.m_peerid.fromObject(options.peerInfo.peerid);
        this.m_peerInfo = options.peerInfo;
        this.m_lastPkgSendTime = 0;
        this.m_lastActiveTime = processUptime();
    }

    get peerid(): Peerid {
        return this.m_peerid;
    }

    get peeridBuffer(): Buffer {
        return this.m_peerInfo.peerid;
    }

    get peeridStr(): string {
        return this.m_peeridStr;
    }

    get peerInfo(): PeerInfo {
        return this.m_peerInfo;
    }

    get transport(): SuperNodeTransport {
        return this.m_lastTransport!;
    }

    get lastActiveTime(): number {
        return this.m_lastActiveTime;
    }

    get cryptoSession(): CryptoSession | undefined {
        return this.m_cryptoSession;
    }

    set cryptoSession(cs: CryptoSession | undefined) {
        this.m_cryptoSession = cs;
    }

    get lastPkgSendTime(): number {
        return this.m_lastPkgSendTime;
    }

    set lastPkgSendTime(sendTime: number) {
        this.m_lastPkgSendTime = sendTime;
    }

    get isWan(): boolean {
        if (this.m_isWan === undefined) {
            this.m_isWan = false;
            for (const epInfo of this.m_peerInfo.endpointArray) {
                if (epInfo[2]) {
                    this.m_isWan = true;
                    return true;
                }
            }
        }
        return this.m_isWan;
    }

    /**
     * 用更新的transport和该peer通信
     * @param transport 新transport
     */
    updateTransport(transport: SuperNodeTransport) {
        const key = `${transport.localEndpoint.toString()}-${transport.remoteEndpoint.toString()}`;
        let existTransport = this.m_transports.get(key);
        if (!existTransport) {
            existTransport = transport;
            this.m_transports.set(key, transport);
        }
        this.m_lastTransport = existTransport;
    }

    findTransport(localEndpoint: Endpoint, remoteEndpoint: Endpoint): SuperNodeTransport | undefined {
        const key = `${localEndpoint.toString()}-${remoteEndpoint.toString()}`;
        return this.m_transports.get(key);
    }

    /**
     * 全量替换peer固定信息数据
     * @param info 新PeerConstInfo
     */
    updatePeerInfo(info: PeerInfo) {
        if (info.createTime && compareUint64(info.createTime, this.m_peerInfo.createTime) > 0) {
            this.m_peerInfo = info;
            this.m_isWan = undefined;
        }
    }

    /**
     * 增加一个endpoint
     * @param ep 要追加的endpoint
     */
    addEndpoint(ep: string) {
        for (let existEp of this.m_peerInfo.endpointArray) {
            if (existEp[0] === ep) {
                return;
            }
        }
        this.m_peerInfo.endpointArray.push([ep, 0]);
    }

    heartbeat() {
        this.m_lastActiveTime = processUptime();
    }

    destroy() {
        this.m_transports.forEach((t) => t.uinit());
        this.m_transports.clear();
        if (this.m_cryptoSession) {
            this.m_cryptoSession.unint();
        }
    }

    private m_logger: LoggerInstance;
    private m_peeridStr: string;
    private m_peerid: Peerid;
    private m_peerInfo: PeerInfo;
    private m_lastTransport?: SuperNodeTransport;
    private m_transports: Map<string, SuperNodeTransport> = new Map();
    private m_cryptoSession?: CryptoSession;
    private m_lastActiveTime: number;
    private m_lastPkgSendTime: number;
    private m_isWan?: boolean;
}