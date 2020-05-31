import * as assert from 'assert';
import { ErrorCode, Endpoint, LoggerInstance } from '../../../base';
import { TCPNetworkInterface } from '../../interface';
import { SuperNodeTransport } from './transport';
import { createTCPPackageEncoder, TCPPackageEncoder, PackageEncryptOptions } from '../../../protocol';
import { CryptoSession } from '../../crypto_helper';

const NoneCryptoResult: {err: ErrorCode, encrypt?: PackageEncryptOptions } = {
    err: ErrorCode.success,
    encrypt: undefined,
};

export class TCPSuperNodeTransport extends SuperNodeTransport {
    constructor(options: {
        logger: LoggerInstance
        interface: TCPNetworkInterface, 
    }) {
        super(options);
        this.m_interface = options.interface;
        this.m_encoder = createTCPPackageEncoder(this.m_logger);
    }

    get localEndpoint(): Endpoint {
        return this.m_interface.local;
    }

    get remoteEndpoint(): Endpoint {
        return this.m_interface.remote;
    }

    get interface(): TCPNetworkInterface {
        return this.m_interface;
    }

    get reliable(): boolean {
        return true;
    }

    isSame(other: SuperNodeTransport): boolean {
        if (!(other && other instanceof TCPSuperNodeTransport)) {
            return false;
        }
        if (other === this) {
            return true;
        }
        return this.m_interface === other.m_interface;
    }
    
    init(): ErrorCode {
        return ErrorCode.success;
    }

    async _uinit(): Promise<ErrorCode> {
        if (!this.m_interface) {
            return ErrorCode.success;
        }
        let ret = await this.m_interface.close();
        delete this.m_interface;
        return ret;
    }

    async send(pkg: {cmdType: number}&any, logKey: string): Promise<ErrorCode> {
        if (this.m_pending) {
            return ErrorCode.outOfLimit;
        }

        let csRet = NoneCryptoResult;
        if (this.m_cryptoSession) {
            csRet = this.m_cryptoSession.preSendPackage();
        }

        const encodeResult = this.m_encoder.encode([pkg], csRet.encrypt);
        if (encodeResult.err || !encodeResult.content) {
            assert(false, `pkg(${pkg.cmdType}) encoded failed:${encodeResult.err}.`);
            this.m_logger.error(`pkg(${pkg.cmdType}) encoded failed:${encodeResult.err}.`);
            return encodeResult.err;
        }

        let sentSize = 0;
        const doSendLeft = async (): Promise<ErrorCode> => {
            const {err, sent} = this.m_interface.send(encodeResult.content!.slice(sentSize));
            if (err || !sent) {
                return ErrorCode.outOfLimit;
            }
    
            if (sent === encodeResult.content!.length - sentSize) {
                return ErrorCode.success;
            }

            sentSize += sent!;

            return new Promise<ErrorCode>((resolve) => {
                this.m_interface.once('drain', async () => {
                    return resolve(await doSendLeft());
                });
            });
        };
        
        this.m_logger.debug(`send pkg ${logKey}`);
        
        {
            this.m_pending = true;
            const err = await doSendLeft();
            this.m_pending = false;
            return err;
        }
    }

    private m_interface: TCPNetworkInterface;
    private m_pending: boolean = false;
    private m_encoder: TCPPackageEncoder;
}
