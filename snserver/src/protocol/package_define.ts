import { ErrorCode, Peerid as _Peerid, PeerConstInfo as _PeerConstInfo, PeerMutableInfo as _PeerMutableInfo, EndpointList as _EndpointList, BiMap, digest } from '../base';
import { uint, serializable, bytes } from './field_define';
import { PackageIndependVerifyContext } from './independ_verifier';
import { isNullOrUndefined } from 'util';

const Peerid = serializable(_Peerid);
const PeerConstInfo = serializable(_PeerConstInfo);
const PeerMutableInfo = serializable(_PeerMutableInfo);
const EndpointList = serializable(_EndpointList);

export class PackageFlags {
    static tunnel = 1;
    static session = 2;

    // session层包标记
    static sessionSyn = 1;
    static sessionAck = 2;
    static sessionFin = 4;
    static sessionSACK = 8;
    static sessionFinAck = 16;

    static contain(dest: number, f: number): boolean {
        return ((dest & f) === f) ? true : false;
    }
    static isSession(dest: number): boolean {
        return PackageFlags.contain(dest, PackageFlags.session);
    }

    static isTunnel(dest: number): boolean {
        return PackageFlags.contain(dest, PackageFlags.tunnel);
    }

    static isSessionSyn(flag: number): boolean {
        return PackageFlags.contain(flag, PackageFlags.sessionSyn) && !PackageFlags.contain(flag, PackageFlags.sessionAck);
    }

    static isSessionAck(flag: number): boolean {
        return PackageFlags.contain(flag, PackageFlags.sessionAck) && !PackageFlags.contain(flag, PackageFlags.sessionSyn);
    }

    static isSessionAckAck(flag: number): boolean {
        return PackageFlags.contain(flag, PackageFlags.composeFlags(PackageFlags.sessionAck, PackageFlags.sessionSyn));
    }

    static isSessionFin(flag: number): boolean {
        return PackageFlags.contain(flag, PackageFlags.sessionFin);
    }

    static isSessionFinAck(flag: number): boolean {
        return PackageFlags.contain(flag, PackageFlags.sessionFinAck);
    }

    static composeFlags(...flags: number[]): number {
        if (flags.length === 0) {
            return 0;
        }
        let ret: number = flags[0];
        for (let f of flags) {
            ret = ret | f;
        }

        return ret;
    }
}

export enum PackageCommand {
    exchangeKey = 0, 
    tunnelSyn = 1,
    tunnelAck = 2, 
    tunnelAckAck = 3,
    tunnelPing = 4,
    tunnelPong = 5,

    snCmdBegin = 0x20,
    snCallReq = snCmdBegin,
    snCallResp = snCmdBegin + 1,
    snCalledReq = snCmdBegin + 2,
    snCalledResp = snCmdBegin + 3,
    snPingReq = snCmdBegin + 4,
    snPingResp = snCmdBegin + 5,

    datagram = 0x30,

    session = 0x40,
}

export function stringifyPackageCommand(cmdType: PackageCommand): string {
    if (cmdType === PackageCommand.exchangeKey) {
        return 'exchangeKey';
    }
    if (cmdType === PackageCommand.tunnelSyn) {
        return 'tunnelSyn';
    }
    if (cmdType === PackageCommand.tunnelAck) {
        return 'tunnelAck';
    }
    if (cmdType === PackageCommand.tunnelAckAck) {
        return 'tunnelAckAck';
    }
    if (cmdType === PackageCommand.tunnelPing) {
        return 'tunnelPing';
    }
    if (cmdType === PackageCommand.tunnelPong) {
        return 'tunnelPong';
    }
    if (cmdType === PackageCommand.snPingReq) {
        return 'snPingReq';
    }
    if (cmdType === PackageCommand.snPingResp) {
        return 'snPingResp';
    }
    if (cmdType === PackageCommand.snCallReq) {
        return 'snCallReq';
    }
    if (cmdType === PackageCommand.snCallResp) {
        return 'snCallResp';
    }
    if (cmdType === PackageCommand.snCalledReq) {
        return 'snCalledReq';
    }
    if (cmdType === PackageCommand.snCalledResp) {
        return 'snCalledResp';
    }
    if (cmdType === PackageCommand.datagram) {
        return 'datagram';
    }
    if (cmdType === PackageCommand.session) {
        return 'session';
    }
    return 'unknown package type';
}

export const PackageCrypto = {
    keyLength: 32,
    encryptKeyLength: 128,
    keyHashLength: 8,
    keyHash(key: Buffer): Buffer {
        return digest.sha256(key).slice(0, PackageCrypto.keyHashLength);
    }
};

const _packageDefinations = [
    // example 
    // {
    //     cmdType: PackageCommand.exchangeKey, 
    //     sequence: uint(32),
    //     seqAndKeySign: bytes(),
    //     fromPeerid: Peerid,
    //     peerInfo: PeerConstInfo,
    //     sendTime: uint(32),
    //     可以在interface完成的不依赖上下文的独立检查定义在这里
    //     independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
    //           return ErrorCode.success;
    //     }
    // }
    {
        cmdType: PackageCommand.tunnelSyn,
        flags: uint(16),
        fromPeer: Peerid,
        toPeer: Peerid,
        sequence: uint(32), 
        tunnelId: uint(32),
        constInfo: PeerConstInfo, 
        sendTime: uint(32),
        independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
            if (pkg.__encrypt.peerid && !pkg.__encrypt.peerid.euqal(pkg.fromPeer)) {
                return ErrorCode.invalidPkg;
            }
            let verifyHr = pkg.fromPeer.verifyConstInfo(pkg.constInfo);
            if (verifyHr.err || verifyHr.verified) {
                return ErrorCode.invalidPkg;
            }
            if (pkg.sendTime < context.currentTime) {
                return ErrorCode.expired;
            }
            return ErrorCode.success;
        }
    }, 
    {
        cmdType: PackageCommand.tunnelAck,
        flags: uint(16),
        sequence: uint(32), 
        ackSequence: uint(32), 
        tunnelId: uint(32),
        toTunnelId: uint(32),
        fromPeer: Peerid, 
        constInfo: PeerConstInfo, 
        sendTime: uint(32),
        independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
            if (pkg.__encrypt.peerid && !pkg.__encrypt.peerid.euqal(pkg.fromPeer)) {
                return ErrorCode.invalidPkg;
            }
            let verifyHr = pkg.fromPeer.verifyConstInfo(pkg.constInfo);
            if (verifyHr.err || verifyHr.verified) {
                return ErrorCode.invalidPkg;
            }
            if (pkg.sendTime < context.currentTime) {
                return ErrorCode.expired;
            }
            return ErrorCode.success;
        }
    },
    {
        cmdType: PackageCommand.tunnelAckAck, 
        flags: uint(16),
        sequence: uint(32), 
        ackSequence: uint(32),
        tunnelId: uint(32),
        toTunnelId: uint(32),
        independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
            return ErrorCode.success;
        }
    },
    {
        cmdType: PackageCommand.tunnelPing,
        flags: uint(16),
        sequence: uint(32),
        tunnelId: uint(32),
        toTunnelId: uint(32),
    },
    {
        cmdType: PackageCommand.tunnelPong,
        flags: uint(16),
        sequence: uint(32),
        tunnelId: uint(32),
        toTunnelId: uint(32),
    },

    // SN
    {
        cmdType: PackageCommand.snPingReq,
        cmdFlags: uint(16),
        sequence: uint(32),
        fromPeerid: Peerid, // 可选，如果和SN之间是加密通信，则不需要
        peerInfo: PeerConstInfo,
    },
    {
        cmdType: PackageCommand.snPingResp,
        cmdFlags: uint(16),
        sequence: uint(32),
        result: uint(8),
        peerInfo: PeerConstInfo,
        endpointArray: EndpointList,
    },
    {
        cmdType: PackageCommand.snCallReq,
        cmdFlags: uint(16),
        sequence: uint(32),
        toPeerid: Peerid,
        fromPeerid: Peerid,
        peerInfo: PeerConstInfo,
        payload: bytes(),
    },
    {
        cmdType: PackageCommand.snCallResp,
        cmdFlags: uint(16),
        sequence: uint(32),
        result: uint(8),
        toPeerInfo: PeerConstInfo, // 可选，没有对方信息，或者对方信息没有意义，比如：对方在NAT后，且只有TCP地址
        independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
            // if (!pkg.targetConstInfo || !pkg.targetMutableInfo) {
            //     // this.m_logger.warn('singletontunnel receive invalid callResp.');
            //     return ErrorCode.invalidPkg;
            // }

            // let hr = pkg.remote.verifyConstInfo(callResp.targetConstInfo);
            // if (hr.err || hr.verified!) {
            //     // this.m_logger.warn(`singletontunnel receive package verify failed. remote:${this.remote.toString()}, constInfo:${callResp.targetConstInfo}`);
            //     return ErrorCode.verifyFailed;
            // }
            return ErrorCode.success;
        }
    },
    {
        cmdType: PackageCommand.snCalledReq,
        cmdFlags: uint(16),
        sequence: uint(32),
        toPeerid: Peerid,
        fromPeerid: Peerid,
        peerInfo: PeerConstInfo,
        payload: bytes(),
        independVerify(pkg: any, context: PackageIndependVerifyContext): ErrorCode {
            // let verifyHr = pkg.sourcePeer.verifyConstInfo(pkg.peerInfo);
            // if (verifyHr.err || verifyHr.verified) {
            //     return ErrorCode.invalidPkg;
            // }
            return ErrorCode.success;
        }
    },
    {
        cmdType: PackageCommand.snCalledResp,
        cmdFlags: uint(16),
        sequence: uint(32),
        result: uint(8),
    },

    {
        cmdType: PackageCommand.datagram, 
        tunnelId: uint(32),
        toTunnelId: uint(32),
        sequence: uint(32),
        reply: uint(32),
        data: bytes(),
        total: uint(32),
        piece: uint(8),
        index: uint(8),
    },

    {
        cmdType: PackageCommand.session,
        toTunnelId: uint(32),
        flags: uint(16),
        connType: uint(8),
        sessionId: uint(32),
        toSessionId: uint(32),
        packageId: uint(32),
        ackPackageId: uint(32),
        streamPos: uint(48),
        ackStreamPos: uint(48),
        sackStreamRanges: bytes(),
        recvSize: uint(48),
        data: bytes(),
        sendTime: uint(32),
        delay: uint(32),
        fastIndex: (pkg: any): string | undefined => {
            if (isNullOrUndefined(pkg.toTunnelId) || isNullOrUndefined(pkg.toSessionId)) {
                return;
            }
            return pkg.toTunnelId.toString() + pkg.toSessionId.toString();
        }
    }
];

let packageDefinations: Map<number, any> = new Map();
let packageIndependVerifiers: Map<number, (pkg: any, context: PackageIndependVerifyContext) => ErrorCode> = new Map();
let packageFastIndex: Map<number, (pkg: any) => string | undefined> = new Map();
for (const pkgDef of _packageDefinations) {
    const cmdType = pkgDef.cmdType;
    let _pkgDef = Object.create(null);
    for (const [field, fieldDef] of Object.entries(pkgDef)) {
        if (field === 'cmdType') {
            continue;
        }
        if (field === 'independVerify') {
            continue;
        }
        if (field === 'fastIndex') {
            continue;
        }
        _pkgDef[field] = fieldDef;
    }
    _pkgDef.cmdType = uint(16);
    packageDefinations.set(cmdType, _pkgDef);
    packageIndependVerifiers.set(cmdType, pkgDef.independVerify ? pkgDef.independVerify : () => ErrorCode.success);
    if (pkgDef.fastIndex) {
        packageFastIndex.set(cmdType, pkgDef.fastIndex);
    }
}

const fieldDictionary = new BiMap<string, string>([
    ['flags', 'f'],
    ['toTunnelId', 'ti'],
    ['toSessionId', 'si'],
    ['streamPos', 'p'],
    ['ackStreamPos', 'ap'],
    ['fromPeer', 'fp'],
    ['toPeer', 'tp'],
    ['sequence', 's'],
    ['sendTime', 'st'],
    ['ackSequence', 'as'],
    ['tunnelId', 'fti'],
    ['sessionId', 'fsi'],
    ['sourceTunnelId', 'sti'],
    ['sourceTunnelSequence', 'sts'],
    ['connType', 'ct'],
    ['packageId', 'pi'],
    ['ackPackageId', 'api'],
    ['sackStreamRanges', 'ssr'],
    ['constInfo', 'ci'],
    ['mutableInfo', 'mi'],
    ['result', 'r'],
    ['eplist', 'e'],
    ['toPeerid', 'tp'],
    ['sourcePeer', 'sp'],
    ['sourceConstInfo', 'sc'],
    ['sourceMutableInfo', 'sm'],
    ['tunnelSequence', 'ts'],
    ['reply', 're'],
    ['data', 'd'],
    ['recvSize', 'rs'],
]);

const MTU: number = 1464;

export {packageDefinations, packageIndependVerifiers, fieldDictionary, packageFastIndex, MTU};
