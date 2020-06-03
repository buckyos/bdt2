import { ErrorCode, LoggerInstance, Endpoint } from '../../../base';
import { NetworkInterface } from '../../../framework/interface';
import { CryptoSession } from '../../crypto_helper';

export abstract class SuperNodeTransport {
    constructor(options: {
        logger: LoggerInstance
    }) {
        this.m_logger = options.logger;
    }

    abstract get localEndpoint(): Endpoint;
    abstract get remoteEndpoint(): Endpoint;
    abstract get interface(): NetworkInterface;
    abstract get reliable(): boolean;
    get cryptoSession(): CryptoSession | undefined {
        return this.m_cryptoSession;
    }
    set cryptoSession(cs: CryptoSession | undefined) {
        this.m_cryptoSession = cs;
    }

    abstract init(): ErrorCode;
    async uinit(): Promise<ErrorCode> {
        const e = await this._uinit();
        if (this.m_cryptoSession) {
            this.m_cryptoSession.unint();
        }
        return e;
    }
    abstract async send(pkg: {cmdType: number}&any, logKey: string): Promise<ErrorCode>;
    abstract isSame(other: SuperNodeTransport): boolean;
    abstract async _uinit(): Promise<ErrorCode>;
        
    protected m_logger: LoggerInstance;
    protected m_cryptoSession?: CryptoSession;
}
