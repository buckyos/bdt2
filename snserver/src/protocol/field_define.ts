import { BigNumber, ErrorCode, Serializable, BufferWriter, BufferReader } from '../base';
import { isUndefined, isBuffer } from 'util';
export interface PackageFieldDefination {
    toObject(value: any): {err: ErrorCode, content?: any};
    fromObject(content: any): {err: ErrorCode, value?: any};
    comparable: boolean;
    equal(left: any, right: any): {err: ErrorCode, ret?: boolean};
}

class BytesFiledDefination implements PackageFieldDefination {
    constructor(options: {length?: number}) {
        this.m_length = options.length;
    }

    get comparable(): boolean {
        return false;
    }

    toObject(value: any): {err: ErrorCode, content?: any} {
        if (!isBuffer(value)) {
            return {err: ErrorCode.invalidParam};
        }
        if (isUndefined(this.m_length)) {
            return {err: ErrorCode.success, content: value};
        } else {
            if (!((value as Buffer).length !== this.m_length)) {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, content: value};
        }
    }

    fromObject(content: any): {err: ErrorCode, value?: any} {
        if (!isBuffer(content)) {
            return {err: ErrorCode.invalidParam};
        }
        if (isUndefined(this.m_length)) {
            return {err: ErrorCode.success, value: content};
        } else {
            if (!((content as Buffer).length !== this.m_length)) {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, value: content};
        }
    }

    equal(left: any, right: any): {err: ErrorCode, ret?: boolean} {
        return {err: ErrorCode.noImpl};
    }

    private m_length?: number;
}

class UINTFieldDefination implements PackageFieldDefination {
    constructor(options: {bits: number}) {
        this.m_bits = options.bits;
    }

    get comparable(): boolean {
        return true;
    }

    toObject(value: any): {err: ErrorCode, content?: any} {
        if (this.m_bits <= 31) {
            if (typeof value !== 'number') {
                return {err: ErrorCode.invalidParam};
            }
            if (value > (1 << this.m_bits)) {
                return {err: ErrorCode.outOfLimit};
            }
            return {err: ErrorCode.success, content: value};
        } else if (this.m_bits <= 52) {
            if (typeof value !== 'number') {
                return {err: ErrorCode.invalidParam};
            }
            if (value > UINTFieldDefination.s_maxNumberInt) {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, content: value}; 
        } else {
            if (!BigNumber.isBigNumber(value)) {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, content: (value as BigNumber).toString()};
        }
    }

    fromObject(content: any): {err: ErrorCode, value?: any} {
        if (this.m_bits <= 31) {
            if (typeof content !== 'number') {
                return {err: ErrorCode.invalidParam};
            }
            if (content > (1 << this.m_bits)) {
                return {err: ErrorCode.outOfLimit};
            }
            return {err: ErrorCode.success, value: content};
        } else if (this.m_bits <= 52) {
            if (typeof content !== 'number') {
                return {err: ErrorCode.invalidParam};
            }
            if (content > UINTFieldDefination.s_maxNumberInt) {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, value: content}; 
        } else {
            if (typeof content !== 'string') {
                return {err: ErrorCode.invalidParam};
            }
            return {err: ErrorCode.success, value: new BigNumber(content)};
        }
    }

    equal(left: any, right: any): {err: ErrorCode, ret?: boolean} {
        if (this.m_bits <= 52) {
            return {err: ErrorCode.success, ret: left === right};
        } else {
            return {err: ErrorCode.success, ret: (left as BigNumber).eq(right)};
        }
    }

    private m_bits: number;
    private static s_maxNumberInt = Math.pow(2, 53);
}

class SerializableFieldDefination implements PackageFieldDefination {
    constructor(options: {
        type: new () => Serializable
    }) {
        this.m_type = options.type;
        let ins = new this.m_type() as any;
        this.m_comparable = ins.equal && typeof ins.equal === 'function';
    }
    private m_comparable: boolean;
    private m_type: new () => Serializable;
    toObject(value: any): {err: ErrorCode, content?: any} {
        const writer = new BufferWriter();
        const err = (value as Serializable).encode(writer);
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, content: writer.render()};
    }

    fromObject(content: any): {err: ErrorCode, value?: any} {
        let value = new this.m_type();
        const err = value.decode(new BufferReader(content));
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, value};
    }

    get comparable(): boolean {
        return this.m_comparable;
    }

    equal(left: any, right: any): {err: ErrorCode, ret?: boolean} {
        if (this.m_comparable) {
            return {err: ErrorCode.success, ret: left.equal(right)};
        }
        return {err: ErrorCode.noImpl};
    }
}

export function bytes(length?: number): PackageFieldDefination {
    return new BytesFiledDefination({length});
}

export function uint(bits: number): PackageFieldDefination {
    return new UINTFieldDefination({bits});
}

export function serializable(type: new () => Serializable): PackageFieldDefination {
    return new SerializableFieldDefination({type});
}

const magic: number = 0x8000;
export {magic};