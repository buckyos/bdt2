import { PeerConstInfo, Peerid, LoggerInstance, PeerMutableInfo } from '../base';
export class LocalPeer {
    constructor(options: {
        logger: LoggerInstance
    }) {
       
    }

    init(options: {
        constInfo: PeerConstInfo, 
        secret: Buffer, 
        mutableInfo: PeerMutableInfo
    }) {
        let peerid = new Peerid();
        let err = peerid.fromConstInfo(options.constInfo);
        if (err) {
            return err;
        }
        const constInfo = options.constInfo.clone();
        const secret = options.secret;
        this.m_peerid = peerid; 
        this.m_constInfo = constInfo;
        this.m_secret = secret;
        this.m_mutableInfo = options.mutableInfo.clone();
    }

    get peerid(): Peerid {
        const p = this.m_peerid!;
        return p;
    }

    get constInfo(): PeerConstInfo {
        const i = this.m_constInfo!;
        return i;
    }

    get mutableInfo(): PeerMutableInfo {
        const i = this.m_mutableInfo!;
        return i;
    }

    get secret(): Buffer {
        const s = this.m_secret!;
        return s;
    }

    private m_peerid?: Peerid;
    private m_constInfo?: PeerConstInfo;
    private m_mutableInfo?: PeerMutableInfo;
    private m_secret?: Buffer;
}