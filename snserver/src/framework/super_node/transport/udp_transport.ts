import * as assert from 'assert';
import { LogShim, ErrorCode, LoggerInstance, Endpoint } from '../../../base';
import { UDPNetworkInterface } from '../../interface';
import { SuperNodeTransport } from './transport';
import { createUDPPackageEncoder, PackageEncoder, PackageEncryptOptions, stringifyPackageCommand, MTU } from '../../../protocol';

const NoneCryptoResult: {err: ErrorCode, encrypt?: PackageEncryptOptions } = {
    err: ErrorCode.success,
    encrypt: undefined,
};

export class UDPSuperNodeTransport extends SuperNodeTransport {
    constructor(options: {
        logger: LoggerInstance
        interface: UDPNetworkInterface,
        remote: Endpoint, 
    }) {
        super(options);
        this.m_interface = options.interface;
        this.m_remote = options.remote;
        this.m_logger = new LogShim(this.m_logger).bind(`[UDPTransport: ${this.m_interface.local.toString()}to${this.m_remote.toString()}]`, true).log;
        this.m_encoder = createUDPPackageEncoder(this.m_logger);
    }
    
    get localEndpoint(): Endpoint {
        return this.m_interface.local;
    }

    get remoteEndpoint(): Endpoint {
        return this.m_remote;
    }

    get interface(): UDPNetworkInterface {
        return this.m_interface;
    }

    get reliable(): boolean {
        return false;
    }
        
    isSame(other: SuperNodeTransport): boolean {
        if (!(other && other instanceof UDPSuperNodeTransport)) {
            return false;
        }
        if (other === this) {
            return true;
        }
        return this.m_interface === other.m_interface && this.m_remote.equal(other.m_remote);
    }

    init(): ErrorCode {
        return ErrorCode.success;
    }

    async _uinit(): Promise<ErrorCode> {
        return ErrorCode.success;
    }

    async send(pkg: {cmdType: number}&any, logKey: string): Promise<ErrorCode> {
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
        if (encodeResult.content.length > MTU) {
            this.m_logger.warn(`package size(${encodeResult.content.length}) is too large, will drop it.`);
            return ErrorCode.outOfLimit;
        }
        this.m_logger.debug(`send pkg ${logKey}`);
        const {err} = this.m_interface.send(encodeResult.content, this.m_remote);
        return err;
    }

    private m_interface: UDPNetworkInterface;
    private m_remote: Endpoint;
    private m_encoder: PackageEncoder;
}