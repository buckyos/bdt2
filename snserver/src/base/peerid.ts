import { ErrorCode } from './error_code';
import { BufferReader, BufferWriter, Serializable, Stringifiable, SerializableWithHash } from './encode';
import { rsa, digest } from './crypto';
import { EndpointList } from './endpointlist';
import { isNullOrUndefined} from 'util';
import { byte, nibble, buffer } from 'bitwise';
import { UInt8, UInt4 } from 'bitwise/types';
import assert = require('assert');

type AreaCodeObject = {
    continent?: number,
    country?: number,
    carrier?: number,
    city?: number,
    inner?: number
};

const AreaCodeLength = 40 / 8;
const AreaCodeWithoutInnerLength = 32 / 8;
export {AreaCodeLength, AreaCodeWithoutInnerLength};

export class AreaCode implements Serializable, Stringifiable {
    static unknown: number = 0;

    // 6+8+4+14=32
    // 6 bits
    private m_continent: number = 0;
    // 8 bits 
    private m_country: number = 0;
    // 4 bits 
    private m_carrier: number = 0;
    // 14 bits
    private m_city: number = 0;
    // 8 bits
    private m_inner: number = 0;

    constructor() {
    }

    fromObject(options: AreaCodeObject): ErrorCode {
        if (!isNullOrUndefined(options.continent)) {
            this.m_continent = options.continent;
        } else {
            this.m_continent = AreaCode.unknown;
        }
        if (!isNullOrUndefined(options.country)) {
            this.m_country = options.country;
        } else {
            this.m_country = AreaCode.unknown;
        }
        if (!isNullOrUndefined(options.carrier)) {
            this.m_carrier = options.carrier;
        } else {
            this.m_carrier = AreaCode.unknown;
        }
        if (!isNullOrUndefined(options.city)) {
            this.m_city = options.city;
        } else {
            this.m_city = AreaCode.unknown;
        }
        if (!isNullOrUndefined(options.inner)) {
            this.m_inner = options.inner;
        } else {
            this.m_inner = AreaCode.unknown;
        }
        return ErrorCode.success;
    }

    toObject(): AreaCodeObject {
        return {
            continent: this.m_continent,
            country: this.m_country,
            carrier: this.m_carrier,
            city: this.m_city,
            inner: this.m_inner
        };
    }

    clone(): AreaCode {
        let ins = new AreaCode();
        ins.fromObject(this);
        return ins;
    }

    cloneWithoutInner(): AreaCode {
        let ins = new AreaCode();
        ins.m_continent = this.m_continent;
        ins.m_country = this.m_country;
        ins.m_carrier = this.m_carrier;
        ins.m_city = this.m_city;
        return ins;
    }

    encode(writer: BufferWriter): ErrorCode {
        // 按理说这里应该是 protocal层用高效语言实现，这里仅做封装, 先实现了能跑起来
        // 左右移只支持signed， 这里如果用加法的话可能会超界，首部的continent和country的首2bites要按byte处理
        let err = this.encodeWithoutInner(writer);
        if (err) {
            return err;
        }
        writer.writeU8(this.m_inner);
        return ErrorCode.success;
    }

    encodeWithoutInner(writer: BufferWriter): ErrorCode {
        const continentBits = byte.read((this.m_continent & 0x3f) as UInt8);
        const countryBits = byte.read((this.m_country & 0xff) as UInt8);
        const carrierBits = nibble.read((this.m_carrier & 0xf) as UInt4);
        const cityHighBits = byte.read((this.m_city & 0x3f00) >>> 8 as UInt8);
        const cityLowBits = byte.read((this.m_city & 0xff) as UInt8);
        const bits = continentBits.slice(-6).concat(countryBits).concat(carrierBits).concat(cityHighBits.slice(-6)).concat(cityLowBits);
        writer.writeU32(buffer.create(bits).readUInt32BE(0));

        return ErrorCode.success;
    }

    decode(reader: BufferReader): ErrorCode {
        let err = this.decodeWithoutInner(reader);
        if (err) {
            return err;
        }
        this.m_inner = reader.readU8();
        return ErrorCode.success;
    }

    decodeWithoutInner(reader: BufferReader): ErrorCode {
        const buf = Buffer.allocUnsafe(4);
        buf.writeUInt32BE(reader.readU32(), 0);
        const o = Object.create(null);
        o.continent = buffer.readUInt(buf, 0, 6);
        o.country = buffer.readUInt(buf, 6, 8);
        o.carrier = buffer.readUInt(buf, 14, 4);
        o.city = buffer.readUInt(buf, 18, 14);
        return this.fromObject(o);
    }

    compare(code: AreaCodeObject|AreaCode): number {
        if (code instanceof AreaCode) {
            return this.compare(code.toObject());
        }

        if (this.m_continent !== code.continent) {
            return (this.m_continent > code.continent!) ? 1 : -1;
        } else if (this.m_country !== code.country) {
            return (this.m_country > code.country!) ? 2 : -2;
        } else if (this.m_carrier !== code.carrier) {
            return (this.m_carrier > code.carrier!) ? 3 : -3;
        } else if (this.m_city !== code.city) {
            return (this.m_city > code.city!) ? 4 : -4;
        } else if (this.m_inner !== code.inner) {
            return (this.m_inner > code.inner!) ? 5 : -5;
        } else {
            return 0;
        }
    }

    get continent(): number {
        return this.m_continent;
    }

    get country(): number {
        return this.m_country;
    }

    get carrier(): number {
        return this.m_carrier;
    }

    get city(): number {
        return this.m_city;
    }

    get inner(): number {
        return this.m_inner;
    }

}

// 允许不同的PeerConstInfo有不同的公私钥体系
export enum PeerPublicKeyType {
    RSA1024,
    RSA2048,
    RSA3072,
    SECP256K1
}

// 不同种类的公钥长度
const PublicKeyLength = new Map<PeerPublicKeyType, number>([
    [PeerPublicKeyType.RSA1024, 162],
    [PeerPublicKeyType.RSA2048, 294],
    [PeerPublicKeyType.RSA3072, 422],
    [PeerPublicKeyType.SECP256K1, 33],
]);

// PeeridConstInfo不包括难度部分，而是通过难度部分的nonce，nonce+ConstInfo可以计算出一个符合难度的hash，peerid本身是通过ConstInfo计算得到的
type ConstInfoObject = {
    areaCode?: AreaCodeObject;
    deviceId?: Buffer;
    publicKeyType?: PeerPublicKeyType;
    publicKey?: Buffer;
};

export type PeerDifficultyInfo = {
    nonce: [number, number];
    difficulty: number;
};

export class PeerConstInfo extends SerializableWithHash implements Stringifiable {
    fromObject(options: ConstInfoObject): ErrorCode {
        if (options.areaCode) {
            this.m_areaCode.fromObject(options.areaCode);
        } else {
            this.m_areaCode = new AreaCode();
        }
        if (options.deviceId) {
            this.m_deviceId = options.deviceId;
        } else {
            this.m_deviceId = Buffer.alloc(0);
        }
        if (!isNullOrUndefined(options.publicKeyType) && options.publicKey) {
            this.m_publicKeyType = options.publicKeyType;
            this.m_publicKey = options.publicKey;
        } else {
            delete this.m_publicKey;
        }
        return ErrorCode.success;
    }

    toObject(): ConstInfoObject {
        return {
           areaCode: this.m_areaCode.toObject(), 
           deviceId: this.m_deviceId,
           publicKeyType: this.m_publicKeyType,
           publicKey: this.m_publicKey
        };
    }

    encode(writer: BufferWriter): ErrorCode {
        const err = super.encode(writer);
        if (err) {
            return err;
        }
        return ErrorCode.success;
    }

    decode(reader: BufferReader): ErrorCode {
        let err = super.decode(reader);
        if (err) {
            return err;
        }
        return ErrorCode.success;
    }

    protected _encodeHashContent(writer: BufferWriter): ErrorCode {
        let err = this.m_areaCode.encode(writer);
        if (err) {
            return err;
        }
        writer.writeU8(this.m_deviceId.length);
        writer.writeBytes(this.m_deviceId);
        writer.writeU8(this.m_publicKeyType!);
        writer.writeBytes(this.m_publicKey!);
        return ErrorCode.success;
    }

    protected _decodeHashContent(reader: BufferReader): ErrorCode {
        let areaCode = new AreaCode();
        let err = areaCode.decode(reader);
        if (err) {
            return err;
        }

        this.m_areaCode = areaCode;
        const deviceIdLength = reader.readU8();
        this.m_deviceId = reader.readBytes(deviceIdLength);
        this.m_publicKeyType = reader.readU8();
        
        const publicKeyLength = PublicKeyLength.get(this.m_publicKeyType)!;
        this.m_publicKey = reader.readBytes(publicKeyLength);
        
        return ErrorCode.success;
    }
 
    get areaCode(): AreaCode {
        return this.m_areaCode;
    }

    get deviceId(): Buffer {
        return this.m_deviceId;
    }

    get publicKeyType(): PeerPublicKeyType|undefined {
        return this.m_publicKeyType;
    }

    get publicKey(): Buffer {
        return this.m_publicKey!;
    }

    setPublicKey(publicKeyType: PeerPublicKeyType, publicKey: Buffer) {
        this.m_publicKeyType = publicKeyType;
        this.m_publicKey = publicKey;
    }

    clone(): PeerConstInfo {
        let ins = new PeerConstInfo();
        ins.m_areaCode = this.m_areaCode.clone();
        ins.m_deviceId = this.m_deviceId;
        if (this.m_publicKey) {
            ins.m_publicKeyType = this.m_publicKeyType;
            ins.m_publicKey = Buffer.allocUnsafe(this.m_publicKey.length);
            this.m_publicKey.copy(ins.m_publicKey);
        }
        return ins;
    }

    equal(other: PeerConstInfo): boolean {
        return other.m_deviceId === this.m_deviceId &&
            other.m_areaCode.compare(this.m_areaCode) === 0 &&
            other.m_publicKeyType === this.m_publicKeyType &&
            other.m_publicKey!.equals(this.m_publicKey!);
    }

    private m_areaCode: AreaCode = new AreaCode();
    private m_deviceId: Buffer = Buffer.alloc(0);
    private m_publicKey?: Buffer;
    private m_publicKeyType?: PeerPublicKeyType;
}

type MutableInfoObject = {
    endpointList: EndpointList,
};

export class PeerMutableInfo extends SerializableWithHash implements Stringifiable {
    get endpointList(): EndpointList {
        const el = this.m_endpointList;
        return el;
    }

    sign(secret: Buffer) {
        // TODO: 
    }

    verify(publicKey: Buffer): {err: ErrorCode, verified?: ErrorCode} {
        // TODO: 
        return {err: ErrorCode.success, verified: ErrorCode.success};
    }

    clone(): PeerMutableInfo {
        let ins = new PeerMutableInfo();
        ins.m_endpointList = this.m_endpointList.clone();
        if (this.m_signature) {
            ins.m_signature = Buffer.allocUnsafe(this.m_signature.length);
            this.m_signature.copy(ins.m_signature);
        }
        return ins;
    }

    fromObject(options: MutableInfoObject): ErrorCode {
        if (options.endpointList) {
            this.m_endpointList = options.endpointList.clone();
        }
        return ErrorCode.success;
    }

    toObject(): MutableInfoObject {
        return {
           endpointList: this.m_endpointList.clone(),
        };
    }

    protected _encodeHashContent(writer: BufferWriter): ErrorCode {
        return this.m_endpointList.encode(writer);
    }

    protected _decodeHashContent(reader: BufferReader): ErrorCode {
        return this.m_endpointList.decode(reader);
    }

    private m_endpointList: EndpointList = new EndpointList();
    private m_signature?: Buffer;
}

const PeeridLength = 32;
export {PeeridLength};

// peerid中不包括难度信息，也不用计算pow. 只保留hash就可以了。nonce在peer类中计算和验证
export class Peerid implements Serializable, Stringifiable {
    static create(constInfo: PeerConstInfo): {err: ErrorCode, peerid?: Peerid, secret?: Buffer} {
        const [publicKey, secret] = rsa.createKeyPair();
        constInfo.setPublicKey(PeerPublicKeyType.RSA1024, publicKey);
        constInfo.updateHash();
        let peerid = new Peerid();
        let err = peerid.fromConstInfo(constInfo);
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, peerid, secret};
    }

    static getConstHash(hash: Buffer): Buffer {
        return hash.slice(AreaCodeLength);
    }

    fromConstInfo(constInfo: PeerConstInfo): ErrorCode {
        const writer = new BufferWriter();
        // 40 bits
        let err = constInfo.areaCode.encode(writer);
        if (err) {
            return err;
        }
        constInfo.updateHash();
        // 截取低216bits，27bytes
        writer.writeBytes(Peerid.getConstHash(constInfo.hash));
        this.m_buffer = writer.render();
        this.m_areaCode = constInfo.areaCode.clone();
        return ErrorCode.success;
    }

    fromObject(options: Buffer): ErrorCode {
        /*let areaCode = new AreaCode();
        let err = areaCode.decode(new BufferReader(options));
        if (err) {
            return err;
        }*/
        this.m_buffer = options;
        return ErrorCode.success;
    }

    clone(): Peerid {
        let peerid = new Peerid();
        peerid.m_areaCode = this.m_areaCode!.clone();
        peerid.m_buffer = Buffer.from(this.m_buffer!);
        return peerid;
    }

    toObject(): any {
        return this.m_buffer!;
    }

    encode(writer: BufferWriter): ErrorCode {
        assert(this.m_buffer!.length === PeeridLength);
        writer.writeBytes(this.m_buffer!);
        return ErrorCode.success;
    }

    decode(reader: BufferReader): ErrorCode {
        const buf = reader.readBytes(PeeridLength);
        let areaCode = new AreaCode();
        let err = areaCode.decode(new BufferReader(buf));
        if (err) {
            return err;
        }
        this.m_buffer = buf;
        this.m_areaCode = areaCode;
        return ErrorCode.success;
    }
 
    toString(): string {
        return this.m_buffer!.toString('hex');
    }

    get areaCode(): AreaCode {
        return this.m_areaCode!;
    }

    equal(other: Peerid): boolean {
        return this.m_buffer!.equals(other.m_buffer!);
    }

    verifyConstInfo(constInfo: PeerConstInfo): {err: ErrorCode, verified?: ErrorCode} {
        constInfo.updateHash();
        const hash = this.m_buffer!.slice(AreaCodeLength);
        const res = hash.compare(Peerid.getConstHash(constInfo.hash));
        return {err: ErrorCode.success, verified: !res ? ErrorCode.success : ErrorCode.verifyFailed};
    }

    private m_areaCode?: AreaCode;
    private m_buffer?: Buffer;
}

const INT32_MAX = 0xffffffff;
// Peer对象代表Peerid+PeerInfo的完整信息，可以对peerid进行验证，也可以不停机的计算nonce
export class PeerUtil {
    static verify(constInfo: PeerConstInfo, diffcultyInfo: PeerDifficultyInfo): boolean {
        return PeerUtil.verifyDiffculty(constInfo, diffcultyInfo!.difficulty, diffcultyInfo!.nonce[0], diffcultyInfo!.nonce[1])[0];
    }

    static getRealDiffculty(buf: Buffer): number {
        for (let i = 0; i < buf.length; i++) {
            let val = buf.readUInt8(i);
            if (val > 0) {
                let j = 1;
                for (; j < 9; j++) {
                    if (val >> j === 0) {
                        break;
                    }
                }
                return i * 8 + (8 - j);
            }
        }
        return 0;
    }

    static checkPOW(hash: Buffer, diffculty: number): [boolean, number] {
        const buflen = Math.ceil(diffculty / 8);
        const pad = diffculty % 8;
        // 截取头diffcultybits，pad到byte边界
        let checkBuf = hash.slice(0, buflen);
        let targetBuf = Buffer.allocUnsafe(buflen);
        targetBuf.fill(0);
        if (pad) {
            targetBuf.set([0xFF >> pad], buflen - 1);
        }
        if (checkBuf.compare(targetBuf) < 1) {
            console.log(`check ${diffculty} success! hash ${hash.toString('hex')}`);
            return [true, PeerUtil.getRealDiffculty(hash)];
        } else {
            return [false, 0];
        }
    }

    static verifyDiffculty(constInfo: PeerConstInfo, diffculty: number, nonce0: number, nonce1: number) {
        const writer = new BufferWriter();
        constInfo.encode(writer);
        writer.writeU32(nonce0);
        writer.writeU32(nonce1);
        let hash = digest.hash256(writer.render());
        return PeerUtil.checkPOW(hash, diffculty);
    }

    // 先实现一个简单的进程内pow
    // 为对人友好，这里的diffculty定义为hash开始全0的bits数，因为没必要像BTC一样调整难度，可以不引入BTC的target概念
    static calcuteNonce(constInfo: PeerConstInfo, minDiffculty: number, range?: {nonce0?: {begin?: number, end?: number}, nonce1?: {begin?: number, end?: number}}): PeerDifficultyInfo|undefined {
        const nonce0 = {begin: 0, end: INT32_MAX};
        const nonce1 = {begin: 0, end: INT32_MAX};
        if (range && range.nonce0 && range.nonce0.begin) {
            nonce0.begin = range.nonce0.begin;
        }
        if (range && range.nonce0 && range.nonce0.end) {
            nonce0.end = range.nonce0.end;
        }
        if (range && range.nonce1 && range.nonce1.begin) {
            nonce1.begin = range.nonce1.begin;
        }
        if (range && range.nonce1 && range.nonce1.end) {
            nonce1.end = range.nonce1.end;
        }
        for (let i = nonce0.begin; i < nonce0.end; i++) {
            for (let j = nonce1.begin; j < nonce1.end; j++) {
                const ret = PeerUtil.verifyDiffculty(constInfo, minDiffculty, i, j);
                if (ret[0]) {
                    return {difficulty: ret[1], nonce: [i, j]};
                }
            }            
        }
    }
}

export type Peer = {
    peerid: Peerid,
    constInfo: PeerConstInfo,
    mutableInfo: PeerMutableInfo,
};
