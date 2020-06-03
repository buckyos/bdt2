import { ErrorCode, BufferWriter, LoggerInstance, BiMap } from '../base';
import { PackageEncoder, PackageEncryptOptions } from './encode';
import * as Protocol from '../js-c/protocol';
import { PackageCommand, PackageCrypto } from './package_define';
import { magic } from './field_define';

export class TCPPackageEncoder extends PackageEncoder {
    constructor(
        logger: LoggerInstance, 
        packageDefinations: Map<number, object>, 
        fieldDictionary?: BiMap<string, string>, 
        independVerifiers?: Map<number, (pkg: any) => ErrorCode>,
        isActiveFirstPackage?: boolean // 是否主动连接方发出的第一个包
    ) {
        
        super(logger, packageDefinations, fieldDictionary, independVerifiers);
        this.m_isActiveFirstPackage = !!isActiveFirstPackage;
    }

    encode(packages: ({cmdType: number}&any)[], encrypt?: PackageEncryptOptions): {err: ErrorCode, content?: Buffer} {
        if (!packages.length) {
            return {err: ErrorCode.invalidParam};
        }

        if (this.m_independVerifier && this.logger.level === 'debug') {
            for (let pkg of packages) {
                const err = this.m_independVerifier.verify(pkg);
                if (!err) {
                    this.logger.warn(`encoding invalid package ${pkg.cmdType}`);
                }
            }
        }

        // 1.填充exchange
        const firstPkg = packages[0];
        if (firstPkg.cmdType === PackageCommand.exchangeKey) {
            this._signExchange(firstPkg, encrypt);
        }

        // 2.encode(packages)
        const encodedBuffer = Protocol.encode(packages);
        if (!encodedBuffer) {
            this.logger.error('udp encode packages failed.');
            return {err: ErrorCode.exception};
        }

        let content: Buffer | undefined;
        if (encrypt) {
            let ecpr = this._encrypt(encodedBuffer, encrypt);
            if (ecpr.err) {
                this.logger.error(`udp encrypt packages failed`, packages);
                return {err: ecpr.err};
            }
            if (this.m_isActiveFirstPackage) {
                let encryptBufferArray = ecpr.content!;
                if (encrypt.newKey) {
                    encryptBufferArray.splice(1, 0, encrypt.keyHash);
                }
                let boxHeader = Buffer.allocUnsafe(2);
                encryptBufferArray.splice(0, 0, boxHeader);
                content = Buffer.concat(encryptBufferArray);
                content.writeUInt16LE(this.cryptoBoxSize(content.length - 2), 0);
            } else {
                let [encryptKey, encryptContent] = ecpr.content!;
                content = Buffer.allocUnsafe(2 + encryptContent.length);
                content.writeUInt16LE(this.cryptoBoxSize(encryptContent.length), 0);
                encryptContent.copy(content, 2);
            }
        } else {
            if (this.m_isActiveFirstPackage) {
                // 主动连接方第一个包要加magic验证包的合法性
                content = Buffer.allocUnsafe(4 + encodedBuffer.length);
                content.writeUInt16LE(magic, 0);
                content.writeUInt16LE(encodedBuffer.length, 2);
                encodedBuffer.copy(content, 4);
            } else {
                content = Buffer.allocUnsafe(2 + encodedBuffer.length);
                content.writeUInt16LE(encodedBuffer.length, 0);
                encodedBuffer.copy(content, 2);
            }
        }

        if (this.m_isActiveFirstPackage) {
            this.m_isActiveFirstPackage = false;
        }
        return {err: ErrorCode.success, content};
    }

    cryptoBoxSize(size: number): number {
        return size;
    }

    protected m_isActiveFirstPackage: boolean;
}