import { ErrorCode, BufferWriter, stringifyErrorCode } from '../base';
import * as Protocol from '../js-c/protocol';
import { PackageCommand, PackageCrypto } from './package_define';
import { PackageEncoder, PackageEncryptOptions } from './encode';
import { magic } from './field_define';

export class UDPPackageEncoder extends PackageEncoder {
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
            if (encrypt.newKey) {
                ecpr.content!.splice(1, 0, encrypt.keyHash);
            }
            content = Buffer.concat(ecpr.content!);
        } else {
            content = Buffer.allocUnsafe(2 + encodedBuffer.length);
            content.writeUInt16LE(magic, 0);
            encodedBuffer.copy(content, 2);
        }
        return {err: ErrorCode.success, content};
    }
}