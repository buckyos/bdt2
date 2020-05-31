import { BufferWriter } from './writer';
import { BufferReader } from './reader';
import { ErrorCode } from '../error_code';
import { Encoding } from './encoding';
import * as digest from '../crypto/digest';

import { isUndefined, isNull, isNumber, isBuffer, isBoolean, isString, isArray, isObject } from 'util';

export function deepCopy(o: any): any {
    if (isUndefined(o) || isNull(o)) {
        return o;
    } else if (isNumber(o) || isBoolean(o)) {
        return o;
    } else if (isString(o)) {
        return o;
    } else if (isBuffer(o)) {
        return Buffer.from(o);
    } else if (isArray(o) || o instanceof Array) {
        let s = [];
        for (let e of o) {
            s.push(deepCopy(e));
        }
        return s;
    } else if (o instanceof Map) {
        let s = new Map();
        for (let k of o.keys()) {
            s.set(k, deepCopy(o.get(k)));
        }
        return s;
    } else if (isObject(o)) {
        let s = Object.create(null);
        for (let k of Object.keys(o)) {
            s[k] = deepCopy(o[k]);
        }
        return s;
    }  else {
        throw new Error('not JSONable');
    }
}

export function toEvalText(o: any): string {
    if (isUndefined(o) || isNull(o)) {
        return JSON.stringify(o);
    } else if (isNumber(o) || isBoolean(o)) {
        return JSON.stringify(o);
    } else if (isString(o)) {
        return JSON.stringify(o);
    } else if (isBuffer(o)) {
        return `Buffer.from('${o.toString('hex')}', 'hex')`;
    } else if (isArray(o) || o instanceof Array) {
        let s = [];
        for (let e of o) {
            s.push(toEvalText(e));
        }
        return `[${s.join(',')}]`; 
    } else if (o instanceof Map) {
        throw new Error(`use MapToObject before toStringifiable`);
    } else if (o instanceof Set) {
        throw new Error(`use SetToArray before toStringifiable`);
    } else if (isObject(o)) {
        let s = [];
        for (let k of Object.keys(o)) {
            s.push(`'${k}':${toEvalText(o[k])}`);
        }
        return `{${s.join(',')}}`;
    }  else {
        throw new Error('not JSONable');
    }
}

export function toStringifiable(o: any, parsable: boolean = false): any {
    if (isUndefined(o) || isNull(o)) {
        return o;
    } else if (isNumber(o) || isBoolean(o)) {
        return o;
    } else if (isString(o)) {
        return parsable ? 's' + o : o;
    } else if (isBuffer(o)) {
        return parsable ? 'b' + o.toString('hex') : o.toString('hex');
    } else if (isArray(o) || o instanceof Array) {
        let s = [];
        for (let e of o) {
            s.push(toStringifiable(e, parsable));
        }
        return s;
    } else if (o instanceof Map) {
        throw new Error(`use MapToObject before toStringifiable`);
    } else if (o instanceof Set) {
        throw new Error(`use SetToArray before toStringifiable`);
    } else if (isObject(o)) {
        let s = Object.create(null);
        for (let k of Object.keys(o)) {
            s[k] = toStringifiable(o[k], parsable);
        }
        return s;
    }  else {
        throw new Error('not JSONable');
    }
}

export function fromStringifiable(o: any): any {
    // let value = JSON.parse(o);
    function __convertValue(v: any): any {
        if (isString(v)) {
            if (v.charAt(0) === 's') {
                return v.substring(1);
            } else if (v.charAt(0) === 'b') {
                return Buffer.from(v.substring(1), 'hex');
            } else {
                throw new Error(`invalid parsable value ${v}`);
            }
        } else if (isArray(v) || v instanceof Array) {
            for (let i = 0; i < v.length; ++i) {
                v[i] = __convertValue(v[i]);
            }
            return v;
        } else if (isObject(v)) {
            for (let k of Object.keys(v)) {
                v[k] = __convertValue(v[k]);
            }
            return v;
        } else {
            return v;
        }
    }
    return __convertValue(o);
}

// 注意这里只需要支持messagepack就好了，可以包含buffer类型字段;
// JSON也可以转换buffer对象，但是非常低效，相当于hex编码了二进制内容
// 支持这个接口的对象可以直接转换到package body中的字段，或者rpc方案用messagepack序列化输入输出的话也可以
export interface Stringifiable {
    toObject(): any;
    fromObject(o: any): ErrorCode; 
}

// 转换为二进制，相较于转换为可以messagepack编码，可以实现bit级别的压缩会更小，
// 而且用于做mesage digest或者校验签名的话还是直接用二进制序列化比较好，
// 这部分的实际实现可能用更高效的语言实现，js仅做封装转调
export interface Serializable {
    encode(writer: BufferWriter): ErrorCode;
    decode(reader: BufferReader): ErrorCode;
}

export class SerializableWithHash implements Serializable {
    constructor() {
        this.m_hash = Buffer.from(Encoding.NULL_HASH, 'hex');
    }
    get hash(): Buffer {
        return this.m_hash;
    }

    protected m_hash: Buffer;

    protected _encodeHashContent(writer: BufferWriter): ErrorCode {
        return ErrorCode.success;
    }
    protected _decodeHashContent(reader: BufferReader): ErrorCode {
        return ErrorCode.success;
    }

    public encode(writer: BufferWriter): ErrorCode {
        // writer.writeHash(this.hash);
        return this._encodeHashContent(writer);
    }

    public decode(reader: BufferReader): ErrorCode {
        // this.m_hash = reader.readHash('hex');
        let err = this._decodeHashContent(reader);
        this.updateHash();
        return err;
    }

    public updateHash(): void {
        this.m_hash = this._genHash();
    }

    protected _genHash(): Buffer {
        let contentWriter: BufferWriter = new  BufferWriter();
        this._encodeHashContent(contentWriter);
        let content: Buffer = contentWriter.render();
        return digest.hash256(content);
    }

    protected _verifyHash(): boolean {
        return this.hash === this._genHash();
    }
}
