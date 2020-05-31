const assert = require('assert');
import { isUndefined } from 'util';
import * as Protocol from '../js-c/protocol';
import { ErrorCode, Peerid, currentTime, stringifyErrorCode } from '../base';
import { PackageDecoder, PackageDecryptOptions, DecodeResult } from './decode';
import { magic} from './field_define';
import { PackageCrypto, PackageCommand } from './package_define';

export class UDPPackageDecoder extends PackageDecoder {
    decode(buffers: Buffer[]): {
        err: ErrorCode,
        packages?: Array<{cmdType: number}&any>
    } {
        let result: {
            err: ErrorCode,
            packages?: Array<{cmdType: number}&any>
        } = {
            err: ErrorCode.unknown,
        };

        for (const buffer of buffers) {
            const decodeResult = this._decodePackage(buffer);
            if (decodeResult.err === ErrorCode.success) {
                if (!result.packages) {
                    result.err = ErrorCode.success;
                    result.packages = decodeResult.packages!;
                } else {
                    result.packages.push(... decodeResult.packages!);
                }
            } else {
                if (result.err === ErrorCode.unknown) {
                    result.err = decodeResult.err;
                }
            }
        }

        return result;
    }

    protected _decodePackage(buffer: Buffer): DecodeResult {
        let msg: any;
        const ct = currentTime();
        let decodeResult: {
            err: ErrorCode,
            packages?: Array<{cmdType: number}&any>, 
            encrypt?: PackageDecryptOptions
        };

        decodeResult = this._tryDecodeWithPlain(buffer);
        if (decodeResult.err === ErrorCode.success) {
            return decodeResult;
        }

        if (buffer.length >= Protocol.minPackageSize + PackageCrypto.keyHashLength) {
            decodeResult = this._tryDecryptWithKeyHash(
                buffer.slice(PackageCrypto.keyHashLength),
                buffer.slice(0, PackageCrypto.keyHashLength)
            );
            if (decodeResult.err !== ErrorCode.success &&
                buffer.length >= Protocol.minPackageSize + PackageCrypto.encryptKeyLength) {

                const keyHash = buffer.slice(PackageCrypto.encryptKeyLength, PackageCrypto.encryptKeyLength + PackageCrypto.keyHashLength);
                decodeResult = this._tryDecryptWithKey(
                    buffer.slice(PackageCrypto.encryptKeyLength + PackageCrypto.keyHashLength),
                    buffer.slice(0, PackageCrypto.encryptKeyLength)
                );

                if (decodeResult.err === ErrorCode.success && decodeResult.packages![0].cmdType === PackageCommand.exchangeKey) {
                    if (decodeResult.encrypt!.keyHash.compare(keyHash) !== 0) {
                        decodeResult = {err: ErrorCode.verifyFailed};
                    }
                }
            }
        }

        if (decodeResult.err === ErrorCode.success) {
            let sendTime = 0;
            for (let pkg of decodeResult.packages!) {
                pkg.__encrypt = decodeResult.encrypt;
                if (pkg.sendTime) {
                    sendTime = pkg.sendTime;
                    // TODO 暂时屏蔽时间比较
                    // if (pkg.sendTime > ct) {
                    //     this.logger.debug(`package decode failed for invalid sendtime ${pkg.sendTime} current ${ct}`);
                    //     return {err: ErrorCode.invalidPkg};
                    // }
                    pkg.__networkDelay = ct - pkg.sendTime;
                } else if (sendTime > 0) {
                    pkg.sendTime = sendTime;
                }
            }
        } else {
            this.logger.debug(`package decode failed err=${decodeResult.err}, buffer:${buffer.slice(0, 32).toString('hex')}`);
            return decodeResult;
        }
        
        if (this.m_independVerifier && this.logger.level === 'debug') {
            for (let pkg of decodeResult.packages!) {
                const err = this.m_independVerifier.verify(pkg);
                if (!err) {
                    this.logger.debug(`package decode failed for independ verify failed ${stringifyErrorCode(err)}`);
                    return {err: ErrorCode.invalidPkg};
                }
            }
        }

        return decodeResult;
    }

    protected _tryDecodeWithPlain(buffer: Buffer): DecodeResult {
        if (!buffer || buffer.length < Protocol.minPackageSize + 2) {
            return {err: ErrorCode.tooSmall};
        }

        const _magic = buffer.readUInt16LE(0);
        if (_magic !== magic) {
            return {err: ErrorCode.invalidPkg};
        }

        let packages = Protocol.decode(buffer.slice(2), false);
        if (packages.length === 0) {
            return {err: ErrorCode.invalidPkg};
        }
        return {err: ErrorCode.success, packages};
    }
}