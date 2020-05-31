const { protocol } = require('./build/Release/js2c');
import { PeerConstInfo } from '../base';

export type PeerInfo = {
    peerid: Buffer,
    constInfo: PeerConstInfo,
    endpointArray: Array< [string, number] >, // [[endpoint, flag]]
    snList?: Array<Buffer>,
    snInfoList?: Array<PeerInfo>,
    signature?: Buffer,
    createTime: { h: number, l: number },
};

export function encode(pkgs: Array<{cmdType: number}&any>): Buffer | undefined {
    let encodedInfo: {buffer: Buffer, length: number} | undefined = protocol.encode(pkgs);
    if (!encodedInfo || !encodedInfo.buffer) {
        return undefined;
    }
    return encodedInfo.buffer.slice(0, encodedInfo.length);
}

export function decode(buffer: Buffer, isStartWithExchangeKey?: boolean): Array<{cmdType: number}&any> {
    let pkgs: Array<{cmdType: number}> | undefined = protocol.decode(buffer, isStartWithExchangeKey || false);
    return pkgs || [];
}

export function signLocalPeerInfo(peerInfo: PeerInfo, secret: Buffer): PeerInfo {
    return protocol.signLocalPeerInfo(peerInfo, secret);
}

export const minPackageSize = 3; // cmdType/cmdFlags