import * as Protocol from '../js-c/protocol';
import { PackageCommand, PackageCrypto } from './package_define';
import { isUndefined } from 'util';
import * as crypto from 'crypto';
import { ErrorCode, BufferReader, LoggerInstance, aes, digest, rsa, BiMap, Peerid, stringifyErrorCode, currentTime } from '../base';
import { PackageFieldDefination } from './field_define';
import { PackageIndependVerifier } from './independ_verifier';

export type PackageDecryptOptions = {
    key: Buffer, 
    keyHash: Buffer, 
    peerid?: Peerid, 
    newKey: boolean,
};

export type DecodeResult = {
    err: ErrorCode, 
    packages?: Array< {cmdType: number}&any >,
    encrypt?: PackageDecryptOptions
};

export interface PackageDecryptKeyDelegate {
    queryKey(hash: Buffer): {err: ErrorCode, key?: Buffer, peerid?: Peerid};
    getLocal(): {err: ErrorCode, secret?: Buffer, public?: Buffer};
}

export abstract class PackageDecoder {
    constructor(
        protected readonly logger: LoggerInstance, 
        private readonly packageDefinations: Map<number, any>, 
        private readonly fieldDictionary?: BiMap<string, string>, 
        private readonly keyDelegate?: PackageDecryptKeyDelegate, 
        independVerifiers?: Map<number, (pkg: any) => ErrorCode>) {
        if (independVerifiers) {
            this.m_independVerifier = new PackageIndependVerifier(this.logger, independVerifiers);
        }
    }

    static decryptKey(key: Buffer, secret: Buffer): Buffer | undefined {
        try {
            return rsa.decrypt(key, secret);
        } catch (error) {
            return undefined;
        }
    }

    protected _tryDecryptWithKeyHash(buffer: Buffer, keyHash: Buffer, key?: Buffer, peerid?: Peerid): DecodeResult {
        if (!this.keyDelegate) {
            this.logger.debug(`no key delegate`);
            return {err: ErrorCode.invalidParam};
        }
        if (!buffer || buffer.length < Protocol.minPackageSize) {
            return {err: ErrorCode.invalidPkg};
        }

        this.logger.debug(`dcrypt with keyhash begin: ${keyHash.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);

        if (!key || !peerid) {
            const qkr = this.keyDelegate.queryKey(keyHash);
            if (qkr.err) {
                this.logger.debug(`query key failed`);
                return {err: ErrorCode.invalidPkg};
            } else {
                key = qkr.key!;
                peerid = qkr.peerid!;
            }
        }

        let decryptedBuffer: Buffer | undefined;
        try {
            decryptedBuffer = aes.decryptData(key, buffer);
        } catch (error) {
        }    
        if (!decryptedBuffer) {
            this.logger.debug(`decrypt content failed`);
            return {err: ErrorCode.invalidParam};
        }

        let packages = Protocol.decode(decryptedBuffer, false);
        if (packages.length === 0) {
            this.logger.debug(`decrypt decode failed. with keyhash: ${keyHash.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
            return {err: ErrorCode.invalidPkg};
        }

        this.logger.debug(`dcrypt with keyhash success: ${keyHash.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
        
        return {
            err: ErrorCode.success,
            packages,
            encrypt: {
                key, 
                keyHash, 
                peerid, 
                newKey: false, 
            }
        };
    }

    protected _tryDecryptWithKey(buffer: Buffer, encryptedKey: Buffer): DecodeResult {
        if (!this.keyDelegate) {
            this.logger.debug(`no key delegate`);
            return {err: ErrorCode.invalidParam};
        }
        if (!buffer || buffer.length < Protocol.minPackageSize) {
            return {err: ErrorCode.invalidPkg};
        }

        this.logger.debug(`decrypt with encrypted key: ${encryptedKey.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
        
        const glr = this.keyDelegate.getLocal();
        if (glr.err) {
            return {err: glr.err};
        }
        const key = PackageDecoder.decryptKey(encryptedKey, glr.secret!);
        if (!key) {
            this.logger.debug(`decrypt key failed, with encrypted key: ${encryptedKey.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
            return {err: ErrorCode.verifyFailed};
        }

        let decryptedBuffer: Buffer | undefined;
        try {
            decryptedBuffer = aes.decryptData(key, buffer);
        } catch (error) {
        }
        if (!decryptedBuffer) {
            this.logger.debug(`decrypt content failed`);
            return {err: ErrorCode.invalidParam};
        }

        let packages = Protocol.decode(decryptedBuffer, true);
        if (packages.length === 0) {
            this.logger.debug(`decrypt key decode failed, with encrypted key: ${encryptedKey.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
            return {err: ErrorCode.invalidPkg};
        }

        this.logger.debug(`decrypt success, with hash: ${PackageCrypto.keyHash(key).toString('hex')}, encrypted key: ${encryptedKey.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
        
        const exchgPkg = packages[0];
        if (exchgPkg.cmdType === PackageCommand.exchangeKey &&
            exchgPkg.seqAndKeySign &&
            exchgPkg.peerInfo &&
            exchgPkg.peerInfo.constInfo &&
            exchgPkg.peerInfo.constInfo.publicKey) {

            let buf4Verify: Buffer = Buffer.allocUnsafe(4 + PackageCrypto.keyLength);
            let offset: number = 0;
            offset = buf4Verify.writeUInt32LE(exchgPkg.sequence || 0, offset);
            key.copy(buf4Verify, offset, 0, PackageCrypto.keyLength);
    
            if (!rsa.verify(buf4Verify, exchgPkg.seqAndKeySign, exchgPkg.peerInfo.constInfo.publicKey)) {
                return { err: ErrorCode.verifyFailed};
            }

            return {
                err: ErrorCode.success,
                packages,
                encrypt: {
                    key,
                    keyHash: PackageCrypto.keyHash(key), // <TODO> hash算法要调整，还需要定时计算hash
                    peerid: exchgPkg.peerInfo.peerid,
                    newKey: true,
                }
            };
        }

        this.logger.debug(`dcrypt verify failed, with encrypted key: ${encryptedKey.toString('hex')}, buffer: ${buffer.slice(0, 32).toString('hex')}`);
        
        return {err: ErrorCode.verifyFailed};
    }

    protected m_independVerifier?: PackageIndependVerifier;
}