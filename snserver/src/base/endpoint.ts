import { isNullOrUndefined, isUndefined } from 'util';
import { ErrorCode } from './error_code';
import { Serializable, Stringifiable, BufferWriter, BufferReader } from './encode';
import * as assert from 'assert';

export enum EndpointIPVersion {
    ipv4 = 0,
    ipv6 = 1
}

export enum EndpointProtocol {
    udp = 0,
    tcp = 1
}

export type EndpointOptions = {
    protocol?: EndpointProtocol,
    ipVersion?: EndpointIPVersion,
    address?: string,
    port?: number,
};

// TODO: 先只支持ipv4了
export class Endpoint implements Serializable, Stringifiable {
    constructor(options?: EndpointOptions) {
        if (options && !isNullOrUndefined(options.protocol)) {
            this.m_protocol = options.protocol;
        } 
        if (options && !isNullOrUndefined(options.ipVersion)) {
            this.m_ipVersion = options.ipVersion;
        } else {
            this.m_ipVersion = EndpointIPVersion.ipv4;
        }
        if (options && !isNullOrUndefined(options.address)) {
            this.m_address = options.address;
        }
        if (options && !isNullOrUndefined(options.port)) {
            this.m_port = options.port;
        }
    }

    get ipVersion(): EndpointIPVersion {
        return this.m_ipVersion;
    }

    get address(): string {
        return this.m_address!;
    }

    get port(): number {
        return this.m_port!;
    }

    get protocol(): EndpointProtocol {
        return this.m_protocol!;
    }

    fromObject(options: EndpointOptions): ErrorCode {
        this.m_protocol = options.protocol; 
        this.m_ipVersion = options.ipVersion!;
        this.m_address = options.address;
        this.m_port = options.port;
        return ErrorCode.success;
    }

    toObject(): EndpointOptions {
        return {
            protocol: this.protocol, 
            ipVersion: this.ipVersion, 
            address: this.address, 
            port: this.port
        };
    }

    encode(writer: BufferWriter): ErrorCode {
        const s = this.m_address!.split('.');
        writer.writeU8(this.m_protocol!);
        for (const p of s) {
            writer.writeU8(parseInt(p));
        }
        writer.writeU16(this.m_port!);
        return ErrorCode.success;
    }

    decode(reader: BufferReader): ErrorCode {
        this.m_protocol = reader.readU8();
        const bytes = [];
        for (let ix = 0; ix < 4; ++ix) {
            bytes.push(reader.readU8());
        }
        bytes.push(reader.readU16());
        this.m_address = `${bytes[0]}.${bytes[1]}.${bytes[2]}.${bytes[3]}`;
        this.m_port = bytes[4];
        return ErrorCode.success;
    }

    // endpoint格式化还要进一步调整
    fromString(s: string): ErrorCode {
        const [fullAddr, port] = s.split(':');
        const version = fullAddr.slice(0, 2) === 'v4' ? EndpointIPVersion.ipv4 : EndpointIPVersion.ipv6;
        const protocol = fullAddr.slice(2, 5) === 'udp' ? EndpointProtocol.udp : EndpointProtocol.tcp;
        let addr: string | undefined;
        
        if (version === EndpointIPVersion.ipv4) {
            addr = fullAddr.slice(5);
        } else {
            addr = s.split(']')[0].slice(6);
        }

        this.m_protocol = protocol;
        this.m_address = addr;
        this.m_port = parseInt(port);
        this.m_ipVersion = version;
        return ErrorCode.success;
    }

    toString(): string {
        const version = this.m_ipVersion === EndpointIPVersion.ipv4 ? 'v4' : 'v6';
        const protocol = this.m_protocol === EndpointProtocol.udp  ? 'udp' : 'tcp';
        if (this.m_ipVersion === EndpointIPVersion.ipv4) {
            return `${version}${protocol}${this.m_address}:${this.m_port}`;
        } else {
            return `${version}${protocol}[${this.m_address}]:${this.m_port}`;
        }
    }

    toBuffer(): Buffer {
        let writer = new BufferWriter();
        this.encode(writer);
        return writer.render();
    }

    clone(): Endpoint {
        let ins = new Endpoint(this);
        return ins;
    }

    equal(other: Endpoint, options?: {ignorePort?: boolean}): boolean {
        let ret = this.m_address === other.m_address &&
            this.m_ipVersion === other.m_ipVersion &&
            this.m_protocol === other.m_protocol;
        if (options && options.ignorePort) {
            return ret;
        }
        return this.m_port === other.m_port && ret;
    }

    static fromString(s: string): {err: ErrorCode, endpoint?: Endpoint} {
        const endpoint = new Endpoint();
        const err = endpoint.fromString(s);
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, endpoint};
    }

    private m_address?: string;
    private m_port?: number;
    private m_ipVersion: EndpointIPVersion;
    private m_protocol?: EndpointProtocol;
}