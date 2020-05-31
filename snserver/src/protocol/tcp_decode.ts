import { ErrorCode, BufferReader, LoggerInstance, BiMap } from '../base';
import { PackageDecoder, PackageDecryptKeyDelegate, PackageDecryptOptions, DecodeResult } from './decode';
import { isNullOrUndefined } from 'util';
import { buffer } from 'bitwise';
import * as Protocol from '../js-c/protocol';
import { PackageCommand, PackageCrypto, MTU } from './package_define';
import { PackageFieldDefination, magic } from './field_define';
import { PackageIndependVerifier } from './independ_verifier';

const HeaderSize = {
    magic: 2,
    boxHeader: 2,
    boxHeaderCrypto: 2,
    mixHash: PackageCrypto.keyHashLength,
    encryptAesKey: PackageCrypto.encryptKeyLength,
};

enum DecoderStatus {
    magic = 0,  // got(sizeof(magic)) = magic -> firstBoxHeaderPlain
                // got(sizeof(magic)) != magic -> mixHash
    firstBoxHeaderPlain = 1,    // got(sizeof(boxHeader)) -> firstContentPlain
                                // tryDecode(all-cache, mixHash) -> boxHeader
                                // tryDecode(all-cache, exchangeKey) -> boxHeader
    firstContentPlain = 2,  // got(boxHeader.size) -> decode -> boxHeader
                            //                     -> !decode -> mixHash
                            // tryDecode(all-cache, mixHash) -> boxHeader
                            // tryDecode(all-cache, exchangeKey) -> boxHeader
    mixHash = 3,    // got(sizeof(mixHash)) -> find(mixHash) -> firstBoxHeaderMixHash
                    //                         !find(mixHash) -> exchangeKey
                    // tryDecode(all-cache, exchangeKey) -> boxHeader
    firstBoxHeaderMixHash = 4,   // got(sizeof(boxHeader)) -> firstContentMixHash
                                 // tryDecode(all-cache, exchangeKey) -> boxHeader
    firstContentMixHash = 5,    // got(boxHeader.size) -> decode(decrypt()) -> boxHeader
                            //                     -> !decode(decrypt()) -> exchangeKey
                            // tryDecode(all-cache, exchangeKey) -> boxHeader
    exchangeKey = 6,    // got(sizeof(cryptoAesKey)) -> decrypt(cryptoAesKey) -> firstBoxHeaderExchangeKey
                        //                              !decrypt(cryptoAesKey) -> FAILED
    firstBoxHeaderExchangeKey = 7, // got(sizeof(boxHeader)) -> firstContentExchangeKey
    firstContentExchangeKey = 8,    // got(boxHeader.size) -> decode(decrypt()) -> boxHeader
                                //                        !decode(decrypt()) -> FAILED
    boxHeader = 9,  // got(sizeof(boxHeader)) -> content
    content = 10,    // got(boxHeader.size) -> decode -> boxHeader
}

const EmptyBuffer = Buffer.allocUnsafe(0);

export class TCPPackageDecoder extends PackageDecoder {
    constructor(
        logger: LoggerInstance, 
        packageDefinations: Map<number, any>, 
        fieldDictionary?: BiMap<string, string>, 
        keyDelegate?: PackageDecryptKeyDelegate, 
        independVerifiers?: Map<number, (pkg: any) => ErrorCode>) {
            super(logger, packageDefinations, fieldDictionary, keyDelegate, independVerifiers);

            this.m_decryptoOption = undefined;
            this.m_status = DecoderStatus.magic;
            this.m_waitSize = HeaderSize.magic;
            this.m_cacheSize = 0;
            this.m_cacheBuffers = [];
            this.m_readOffset = 0;
            this.m_concatedSize = 0;
            this.m_concatBuffer = Buffer.allocUnsafe(MTU);
    }

    decode(data: Buffer, limit?: number): DecodeResult {
        const concatCachedBuffers = () => {
            for (let i = 0; i < this.m_cacheBuffers.length; i++) {
                const buf = this.m_cacheBuffers[i];
                if (this.m_concatedSize + buf.length > this.m_concatBuffer.length) {
                    const copySize = this.m_concatBuffer.length - this.m_concatedSize;
                    buf.copy(this.m_concatBuffer, this.m_concatedSize, 0, copySize);
                    this.m_concatedSize = this.m_concatBuffer.length;
                    this.m_cacheBuffers.splice(0, i + 1, buf.slice(copySize));
                    break;
                } else {
                    buf.copy(this.m_concatBuffer, this.m_concatedSize);
                    this.m_concatedSize += buf.length;
                }
            }

            if (this.m_cacheSize <= this.m_concatBuffer.length) {
                this.m_cacheBuffers = [];
            }
        };

        if (data && data.length > 0) {
            this.m_cacheBuffers.push(data);
            this.m_cacheSize += data.length;
    
            concatCachedBuffers();
        }

        if (!limit) {
            limit = 0x7FFFFFFF;
        }

        let result: DecodeResult | undefined;
        do {
            let singleResult = this._decodeSingleNextStep();
            concatCachedBuffers();

            if (singleResult.err !== ErrorCode.success) {
                if (result && result.packages!.length > 0) {
                    return result;
                }
                if (singleResult.err === ErrorCode.pending && this.m_cacheSize >= MTU) {
                    singleResult.err = ErrorCode.invalidPkg;
                }
                return singleResult;
            } else {
                let sendTime = 0;
                for (let pkg of singleResult.packages!) {
                    if (pkg.sendTime) {
                        sendTime = pkg.sendTime;
                    } else if (sendTime > 0) {
                        pkg.sendTime = sendTime;
                    }    
                }

                if (!result) {
                    result = singleResult;
                } else {
                    result.packages!.splice(result.packages!.length, 0, ...singleResult.packages!);
                }
            }
        } while (result.packages!.length < limit);

        if (this.m_decryptoOption) {
            result.encrypt = this.m_decryptoOption as PackageDecryptOptions;
        }
        return result;
    }

    detachCacheData(): Array<Buffer> {
        let cachedBuffers = this.m_cacheBuffers;
        if (this.m_concatedSize > 0) {
            cachedBuffers.splice(0, 0, this.m_concatBuffer.slice(0, this.m_concatedSize));
        }
        this.m_cacheBuffers = [];
        this.m_concatBuffer = EmptyBuffer;
        this.m_concatedSize = 0;
        this.m_cacheSize = 0;
        this.m_readOffset = 0;
        this.m_status = DecoderStatus.boxHeader;
        this.m_waitSize = HeaderSize.boxHeader;
        return cachedBuffers;
    }

    _decodeSingleNextStep(): DecodeResult {
        if (this.m_cacheSize < this.m_waitSize) {
            return {err: ErrorCode.pending};
        }

        let result: DecodeResult | undefined;
    
        const onPlainPending = () => {
            let tryResult = this._tryDecodeMixHash();
            if (tryResult.err === ErrorCode.success) {
                return tryResult;
            }

            tryResult = this._tryDecodeExchangeKey();
            if (tryResult.err === ErrorCode.success) {
                return tryResult;
            }
            return result;
        };

        const onPlainFailed = () => {
            this.m_status = DecoderStatus.mixHash;
            this.m_waitSize = HeaderSize.mixHash;
            this.m_readOffset = 0;
            return this._decodeSingleNextStep();
        };

        const onMixHashPending = () => {
            let tryResult = this._tryDecodeExchangeKey();
            if (tryResult.err === ErrorCode.success) {
                return tryResult;
            }
            return result;
        };

        const onMixHashFailed = () => {
            this.m_status = DecoderStatus.exchangeKey;
            this.m_waitSize = HeaderSize.encryptAesKey + HeaderSize.mixHash;
            this.m_readOffset = 0;
            return this._decodeSingleNextStep();
        };

        switch (this.m_status) {
            case DecoderStatus.magic: {
                if (this.m_concatBuffer.readUInt16LE(this.m_readOffset) === magic) {
                    this.m_status = DecoderStatus.firstBoxHeaderPlain;
                    this.m_waitSize += HeaderSize.boxHeader;
                    this.m_readOffset += HeaderSize.magic;
                    result = this._decodeSingleNextStep();
                    if (result.err === ErrorCode.success) {
                        return result;
                    } else if (result.err !== ErrorCode.pending) {
                        return onPlainFailed();
                    }
                } else {
                    return onPlainFailed();
                }

                return onPlainPending()!;
            }

            case DecoderStatus.firstBoxHeaderPlain: {
                const packageSize = this.m_concatBuffer.readUInt16LE(this.m_readOffset);
                this.m_status = DecoderStatus.firstContentPlain;
                this.m_readOffset += HeaderSize.boxHeader;
                this.m_waitSize += packageSize;
                if (this.m_waitSize > MTU) {
                    return onPlainFailed();
                }
                result = this._decodeSingleNextStep();
                if (result.err === ErrorCode.success) {
                    return result;
                } else if (result.err !== ErrorCode.pending) {
                    return onPlainFailed();
                }
                return onPlainPending()!;
            }

            case DecoderStatus.firstContentPlain: {
                let packages = Protocol.decode(this.m_concatBuffer.slice(this.m_readOffset, this.m_waitSize), false);
                if (packages.length === 0) {
                    return onPlainFailed();
                } else {
                    this.m_decryptoOption = false;
                    this._prepareNextPackage(this.m_waitSize);
                    return {err: ErrorCode.success, packages};
                }
            }

            case DecoderStatus.mixHash: {
                this.m_decryptoOption = {
                    key: EmptyBuffer, 
                    keyHash: this.m_concatBuffer.slice(0, HeaderSize.mixHash), 
                    peerid: undefined, 
                    newKey: false, 
                };
                this.m_status = DecoderStatus.firstBoxHeaderMixHash;
                this.m_waitSize += HeaderSize.boxHeaderCrypto;
                this.m_readOffset += HeaderSize.mixHash;
                result = this._decodeSingleNextStep();
                if (result.err === ErrorCode.success) {
                    return result;
                } else if (result.err !== ErrorCode.pending) {
                    return onMixHashFailed();
                }
                return onMixHashPending()!;
            }

            case DecoderStatus.firstBoxHeaderMixHash: {
                const packageSize = this._decryptoBoxSize(this.m_concatBuffer.readUInt16LE(this.m_readOffset));
                if (packageSize < Protocol.minPackageSize) {
                    return onMixHashFailed();
                }
                this.m_status = DecoderStatus.firstContentMixHash;
                this.m_readOffset += HeaderSize.boxHeaderCrypto;
                this.m_waitSize += packageSize;
                if (this.m_waitSize > MTU) {
                    return onMixHashFailed();
                }
                result = this._decodeSingleNextStep();
                if (result.err === ErrorCode.success) {
                    return result;
                } else if (result.err !== ErrorCode.pending) {
                    return onMixHashFailed();
                }
                return onMixHashPending()!;
            }

            case DecoderStatus.firstContentMixHash: {
                result = this._tryDecryptWithKeyHash(
                    this.m_concatBuffer.slice(this.m_readOffset, this.m_waitSize),
                    (this.m_decryptoOption as PackageDecryptOptions).keyHash
                );
                if (result.err !== ErrorCode.success) {
                    return onMixHashFailed();
                } else {
                    this.m_decryptoOption = result.encrypt!;
                    this._prepareNextPackage(this.m_waitSize);
                    for (let pkg of result.packages!) {
                        pkg.__encrypt = this.m_decryptoOption;
                    }
                    return result;
                }
            }

            case DecoderStatus.exchangeKey: {
                this.m_decryptoOption = {
                    key: this.m_concatBuffer.slice(0, HeaderSize.encryptAesKey), // 暂时存个加密key，后面解包成功会置成原始key
                    keyHash: this.m_concatBuffer.slice(HeaderSize.encryptAesKey, HeaderSize.mixHash), 
                    peerid: undefined, 
                    newKey: false, 
                };
                this.m_status = DecoderStatus.firstBoxHeaderExchangeKey;
                this.m_waitSize += HeaderSize.boxHeaderCrypto;
                this.m_readOffset += HeaderSize.encryptAesKey + HeaderSize.mixHash;
                return this._decodeSingleNextStep();
            }

            case DecoderStatus.firstBoxHeaderExchangeKey: {
                const packageSize = this._decryptoBoxSize(this.m_concatBuffer.readUInt16LE(this.m_readOffset));
                if (packageSize < Protocol.minPackageSize) {
                    return {err: ErrorCode.invalidPkg};
                }
                this.m_status = DecoderStatus.firstContentExchangeKey;
                this.m_readOffset += HeaderSize.boxHeaderCrypto;
                this.m_waitSize += packageSize;
                if (this.m_waitSize > MTU) {
                    return {err: ErrorCode.invalidPkg};
                }
                return this._decodeSingleNextStep();
            }

            case DecoderStatus.firstContentExchangeKey: {
                result = this._tryDecryptWithKey(
                    this.m_concatBuffer.slice(this.m_readOffset, this.m_waitSize),
                    (this.m_decryptoOption as PackageDecryptOptions).key
                );
                if (result.err !== ErrorCode.success) {
                    this.m_decryptoOption = undefined;
                    return result;
                } else {
                    this.m_decryptoOption = result.encrypt!;
                    this._prepareNextPackage(this.m_waitSize);
                    for (let pkg of result.packages!) {
                        pkg.__encrypt = this.m_decryptoOption;
                    }
                    return result;
                }
            }

            case DecoderStatus.boxHeader: {
                let packageSize = 0;
                if (this.m_decryptoOption) {
                    packageSize = this._decryptoBoxSize(this.m_concatBuffer.readUInt16LE(this.m_readOffset));
                    this.m_readOffset += HeaderSize.boxHeaderCrypto;
                } else {
                    packageSize = this.m_concatBuffer.readUInt16LE(this.m_readOffset);
                    this.m_readOffset += HeaderSize.boxHeader;
                }
                if (packageSize < Protocol.minPackageSize) {
                    return {err: ErrorCode.invalidPkg};
                }
                this.m_status = DecoderStatus.content;
                this.m_waitSize += packageSize;
                if (this.m_waitSize > MTU) {
                    return {err: ErrorCode.invalidPkg};
                }
                return this._decodeSingleNextStep();
            }

            case DecoderStatus.content: {
                if (this.m_decryptoOption) {
                    result = this._tryDecryptWithKeyHash(
                        this.m_concatBuffer.slice(this.m_readOffset, this.m_waitSize),
                        (this.m_decryptoOption as PackageDecryptOptions).keyHash,
                        (this.m_decryptoOption as PackageDecryptOptions).key,
                        (this.m_decryptoOption as PackageDecryptOptions).peerid,
                    );
                    if (result.err === ErrorCode.success) {
                        for (let pkg of result.packages!) {
                            pkg.__encrypt = this.m_decryptoOption;
                        }
                    }
                } else {
                    let packages = Protocol.decode(this.m_concatBuffer.slice(this.m_readOffset, this.m_waitSize), false);
                    if (packages.length === 0) {
                        return {err: ErrorCode.invalidPkg};
                    }
                    result = {err: ErrorCode.success, packages};
                }
                this._prepareNextPackage(this.m_waitSize);
                return result;
            }
        }
        return {err: ErrorCode.noImpl};
    }

    protected _decryptoBoxSize(cryptoSize: number): number {
        return cryptoSize;
    }

    protected _prepareNextPackage(boxSize: number) {
        this.m_status = DecoderStatus.boxHeader;
        this.m_cacheSize -= boxSize;
        this.m_readOffset = 0;
        this.m_concatedSize -= boxSize;
        this.m_waitSize = HeaderSize.boxHeader;
        this.m_concatBuffer.copy(this.m_concatBuffer, 0, boxSize, this.m_concatedSize);
    }

    // try-decode失败不改状态，成功才更新状态
    protected _tryDecodeMixHash(): DecodeResult {
        if (this.m_concatedSize < HeaderSize.mixHash + HeaderSize.boxHeaderCrypto + Protocol.minPackageSize) {
            return {err: ErrorCode.pending};
        }

        const keyHash = this.m_concatBuffer.slice(0, HeaderSize.mixHash);
        const packageSize = this._decryptoBoxSize(this.m_concatBuffer.readUInt16LE(HeaderSize.mixHash));
        const headerLength = HeaderSize.mixHash + HeaderSize.boxHeaderCrypto;
        const boxSize = packageSize + headerLength;

        if (packageSize < Protocol.minPackageSize ||
            boxSize > MTU) {
            return {err: ErrorCode.invalidPkg};
        }

        if (boxSize > this.m_concatedSize) {
            return {err: ErrorCode.pending};
        }

        let result = this._tryDecryptWithKeyHash(this.m_concatBuffer.slice(headerLength, boxSize), keyHash);
        if (result.err === ErrorCode.success) {
            this.m_decryptoOption = result.encrypt;
            this._prepareNextPackage(boxSize);

            for (let pkg of result.packages!) {
                pkg.__encrypt = this.m_decryptoOption;
            }
        }
        return result;
    }

    protected _tryDecodeExchangeKey(): DecodeResult {
        const keyLength = HeaderSize.encryptAesKey + HeaderSize.mixHash;
        const headerLength = keyLength + HeaderSize.boxHeaderCrypto;
        if (this.m_concatedSize < headerLength + Protocol.minPackageSize) {
            return {err: ErrorCode.pending};
        }

        const encryptKey = this.m_concatBuffer.slice(0, HeaderSize.encryptAesKey);
        // const keyHash = this.m_concatBuffer.slice(HeaderSize.encryptAesKey, keyLength);
        const packageSize = this._decryptoBoxSize(this.m_concatBuffer.readUInt16LE(keyLength));
        const boxSize = packageSize + headerLength;

        if (packageSize < Protocol.minPackageSize ||
            boxSize > MTU) {
            return {err: ErrorCode.invalidPkg};
        }

        if (boxSize > this.m_concatedSize) {
            return {err: ErrorCode.pending};
        }

        let result = this._tryDecryptWithKey(this.m_concatBuffer.slice(headerLength, boxSize), encryptKey);
        if (result.err === ErrorCode.success) {
            this.m_decryptoOption = result.encrypt;
            this._prepareNextPackage(boxSize);

            for (let pkg of result.packages!) {
                pkg.__encrypt = this.m_decryptoOption;
            }
        }
        return result;
    }

    private m_decryptoOption?: boolean | PackageDecryptOptions;
    private m_status: DecoderStatus;
    private m_waitSize: number;
    private m_cacheSize: number;
    private m_cacheBuffers: Array<Buffer>;
    private m_readOffset: number;
    private m_concatedSize: number;
    private m_concatBuffer: Buffer;
}