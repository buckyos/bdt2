import { isNullOrUndefined, isUndefined } from 'util';
import { ErrorCode } from './error_code';
import { Serializable, Stringifiable, BufferWriter, BufferReader } from './encode';
import { Endpoint, EndpointOptions, EndpointProtocol, EndpointIPVersion } from './endpoint';

// TODO: 可以支持类似 ipv4127.0.0.1:tcp1000-1010udp1000-1010的合并配置？
export class EndpointList implements Serializable {
    constructor(optionsList?: EndpointOptions[]) {
        if ( optionsList ) {
            for (let options of optionsList ) {
                const endpoint = new Endpoint(options);
                this.addEndpoint(endpoint);
            }
        }
    }

    get length(): number {
        return this.m_tcpList.length + this.m_udpList.length;
    }

    list(options?: {
        protocol?: EndpointProtocol}): Endpoint[] {
        if (!options || isUndefined(options.protocol)) {
            let list = this.m_tcpList.slice(0);
            return list.concat(...this.m_udpList);
        } else if (options.protocol === EndpointProtocol.udp) {
            const list = this.m_udpList;
            return list;
        } else {
            const list = this.m_tcpList;
            return list;
        }
    }
    
    addEndpoint(ep: Endpoint): ErrorCode {
        if ((this.m_tcpList.length + this.m_udpList.length) > 256) {
            return ErrorCode.outOfLimit;
        }

        if (this.exist(ep)) {
            return  ErrorCode.duplicateEndpoint;
        }

        if (ep.protocol === EndpointProtocol.udp) {
            this.m_udpList.push(ep.clone());
        } else {
            this.m_tcpList.push(ep.clone());
        }

        return ErrorCode.success;
    }

    union(other: EndpointList) {
        // 转换格式
        const otherlist: string[] = other.toArray();
        // 以数组形式合并
        const newlist: string[] = this.toArray().concat(otherlist);
        // 数组去重
        const unionEplist: string = [...new Set(newlist)].join(';');
        // 转换回EndpointList对象
        const deduplication = EndpointList.fromString(unionEplist);

        this.m_udpList = deduplication.list!.udpList;
        this.m_tcpList = deduplication.list!.tcpList;
    }

    static fromString(s: string): {err: ErrorCode, list?: EndpointList} {
        const list = new EndpointList();
        const err = list.fromString(s);
        if (err) {
            return {err};
        }
        return {err: ErrorCode.success, list};
    }

    encode(writer: BufferWriter): ErrorCode {
        writer.writeU8(this.m_udpList.length + this.m_tcpList.length);
        for (const ep of this.m_udpList) {
            const err = ep.encode(writer);
            if (err) {
                return err;
            }
        }
        for (const ep of this.m_tcpList) {
            const err = ep.encode(writer);
            if (err) {
                return err;
            }
        }
        return ErrorCode.success;
    }

    decode(reader: BufferReader): ErrorCode {
        const size = reader.readU8();
        let udpList = [];
        let tcpList = [];
        for (let ix = 0; ix < size; ++ix) {
            let ep = new Endpoint();
            const err = ep.decode(reader);
            if (err) {
                return err;
            }
            if (ep.protocol === EndpointProtocol.udp) {
                udpList.push(ep);
            } else {
                tcpList.push(ep);
            }
        }
        this.m_udpList = udpList;
        this.m_tcpList = tcpList;
        return ErrorCode.success;
    }

    fromString(s: string): ErrorCode {
        let udpList = [];
        let tcpList = [];
        if (s.length > 0) {
            const ss = s.split(';');
            for (const es of ss) {
                const rs = Endpoint.fromString(es);
                if (rs.err) {
                    return rs.err;
                }
                if (rs.endpoint!.protocol === EndpointProtocol.udp) {
                    udpList.push(rs.endpoint!);
                } else {
                    tcpList.push(rs.endpoint!);
                }
            }
        }

        this.m_udpList = udpList;
        this.m_tcpList = tcpList;
        return ErrorCode.success;
    } 

    toString(): string {
        const l = this.list();
        if (!l.length) {
            return '';
        }
        let s = l[0].toString();
        for (const es of l.slice(1)) {
            s += `;${es.toString()}`;
        }
        return s;
    }

    toArray(): string[] {
        if (this.length === 0) {
            return [];
        }
        return this.toString().split(';');
    }

    exist(ep: Endpoint): boolean {
        let exist = false;
        let targetList = ep.protocol === EndpointProtocol.udp ? this.m_udpList : this.m_tcpList;

        for ( const value of targetList ) {
            if (value.equal(ep)) {
                exist = true;
            }
        }
        return exist;
    }

    clone(): EndpointList {
        let ins = new EndpointList();
        ins.m_udpList = this.m_udpList.slice(0);
        ins.m_tcpList = this.m_tcpList.slice(0);
        return ins;
    }

    get udpList(): Endpoint[] {
        return this.m_udpList;
    }

    get tcpList(): Endpoint[] {
        return this.m_tcpList;
    }

    private m_udpList: Array<Endpoint> = [];
    private m_tcpList: Array<Endpoint> = [];
}
