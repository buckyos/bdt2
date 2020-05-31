import * as assert from 'assert';
import { isUndefined } from 'util';
import * as Protocol from '../js-c/protocol';
import { PackageCommand, PackageCrypto } from './package_define';
import * as crypto from 'crypto';
import { ErrorCode, BiMap, LoggerInstance, digest, rsa, aes } from '../base';
import { PackageIndependVerifier } from './independ_verifier';

export type PackageEncryptOptions = {
    key: Buffer, 
    keyHash: Buffer, 
    newKey?: {
        remotePublic: Buffer, 
    }, 
    sign?: {
        secret: Buffer
    }
};

export abstract class PackageEncoder {
    constructor(
        protected readonly logger: LoggerInstance, 
        private readonly packageDefinations: Map<number, object>, 
        private readonly fieldDictionary?: BiMap<string, string>, 
        independVerifiers?: Map<number, (pkg: any) => ErrorCode>) {
        if (independVerifiers) {
            this.m_independVerifier = new PackageIndependVerifier(this.logger, independVerifiers);
        }
    }

    abstract encode(packages: ({cmdType: number}&any)[], encrypt?: PackageEncryptOptions): {err: ErrorCode, content?: Buffer};

    protected _signExchange(exchgPkg: {cmdType: number} & any, encrypt?: PackageEncryptOptions): void {
        if (exchgPkg.cmdType !== PackageCommand.exchangeKey || !encrypt) {
            return;
        }

        let buf4Sign: Buffer = Buffer.allocUnsafe(4 + PackageCrypto.keyLength);
        let offset: number = 0;
        offset = buf4Sign.writeUInt32LE(exchgPkg.sequence || 0, offset);
        encrypt.key.copy(buf4Sign, offset, 0, PackageCrypto.keyLength);

        const sign = rsa.sign(buf4Sign, encrypt.sign!.secret);
        exchgPkg.seqAndKeySign = sign;
    }

    protected _encrypt(_content: Buffer, encrypt?: PackageEncryptOptions): {err: ErrorCode, content?: Buffer[]} {
        if (!encrypt) {
            return {err: ErrorCode.success, content: [_content]};
        }

        assert(encrypt.keyHash.length === PackageCrypto.keyHashLength);
        let encryptKey = encrypt.keyHash;
        if (encrypt.newKey) {
            encryptKey = PackageEncoder.encryptKey(encrypt.key, encrypt.newKey.remotePublic);
        }
        const encryptContent = aes.encryptData(encrypt.key, _content);
        const content = [encryptKey, encryptContent];
        
        return {err: ErrorCode.success, content};
    }   

    static encryptKey(key: Buffer, remotePublic: Buffer): Buffer {
        return rsa.encrypt(key, remotePublic);
    }

    protected m_independVerifier?: PackageIndependVerifier;
}