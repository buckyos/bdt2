
declare enum BdtResult {
    success = 0,
    failed = 1,
    invalidState = 11,
    pending = 6,
    timeout = 29,
}

// type Time64 = {
//     l: number;
//     h: number;
// }

type Peerid = Buffer;

type PeerArea = {
    continent: number;
    country: number;
    carrier: number;
    city: number;
    inner: number;
}

declare enum PeerPublicKeyType {rsa1024 = 0, rsa2048 = 1, rsa3072 = 2}

type PeerConstInfo = {
    areaCode: PeerArea;
    deviceId: Buffer;
    publicKeyType: PeerPublicKeyType;
    publicKey: Buffer;
}

type LocalPeerSecret = Buffer;

interface PeerConstInfoStatic {
    // new(areaCode: PeerArea, deviceId: Buffer, publicKeyType: PeerPublicKeyType): {
    //     err: BdtResult,
    //     constInfo?: PeerConstInfo;
    //     secret?: LocalPeerSecret;
    // };
    toPeerid(constInfo: PeerConstInfo): Peerid | null | undefined;
}

declare const PeerConstInfo: PeerConstInfoStatic;

// interface PeeridStatic {
//     new(hex: string): Peerid | null;
//     new(buffer: Buffer): Peerid | null;
//     new(constInfo: PeerConstInfo): {err: BdtResult, peerid?: Peerid};

//     verify(peerid: Peerid, constInfo: PeerConstInfo): boolean;
//     compare(left: Peerid, right: Peerid): number;
// }

// declare const Peerid: PeeridStatic;

// declare enum EndpointProtocol {tcp = 0, udp = 1}
// declare enum EndpointIpVersion {v4 = 4, v6 = 6}

// type Endpoint = {
//     address: string;
//     port: number;
//     protocol: EndpointProtocol;
//     ipVersion: EndpointIpVersion;
//     isStaticWan: boolean;
//     isSigned: boolean;
// }

// interface EndpointStatic {
//     new(str: string): Endpoint | null;
//     new(other: Endpoint): Endpoint | null;

//     toString(endpoint: Endpoint): string | null;
//     compare(left: Endpoint, right: Endpoint): number;
// }

// declare const Endpoint: EndpointStatic;

// type PeerInfoSignature = Buffer;

type Endpoint = string;

type PeerInfo = {
    readonly peerid: Peerid;
    readonly constInfo: PeerConstInfo;
    readonly endpoints: Array<Endpoint>;
    readonly supernodes: Array<Peerid | PeerInfo>;
    // readonly createTime: Time64;
    // readonly signature?: PeerInfoSignature;
}

type LocalPeerInfo = PeerInfo & {
    secret: LocalPeerSecret;
}

// type PeerInfoOptions = {
//     peerid: Peerid;
//     constInfo: PeerConstInfo;
//     endpoints: Array<BdtEndpoint>;
//     supernodes: Array<BdtPeerid | PeerInfo>;
//     createTime: Time64;
//     signature?: PeerInfoSignature;
// }

// interface PeerInfoStatic {
//     new(options: PeerInfoOptions): PeerInfo | null;
//     new(options: PeerInfoOptions, secret: LocalPeerSecret): {err: BdtResult, localPeerInfo?: LocalPeerInfo};

//     equal(left: PeerInfo, right: PeerInfo): boolean;
// }

// declare const PeerInfo: PeerInfoStatic;

type PackageConnectionConfig = {
    resendInterval: number;
    recvTimeout: number;
    msl: number;
    mtu: number;
    mss: number;
    maxRecvBuffer: number;
    maxSendBuffer: number;
    halfOpen: boolean;
}

type StreamConnectionConfig = {
    minRecordSize: number;
    maxRecordSize: number;
    maxNagleDalay: number;
    resendInterval: number;
    recvTimeout: number;
}

type ConnectionConfig = {
    pkg: PackageConnectionConfig;
    stream: StreamConnectionConfig;
    connectTimeout: number;
}

type ListenConnectionConfig = ConnectionConfig & {
    backlog: number;
}

type BuildTunnelOptions = {
    constInfo: PeerConstInfo,
    supernodes: Array<Peerid>,
    // flags: number,
    // aesKey: Buffer,
    // localEndpoint: Endpoint,
    // remoteEndpoint: Endpoint,
}

interface Connection {
    readonly name: string;
    readonly providerName: string;
    
    confirm(options: {
        answer?: Buffer,
        onConnect: (err: BdtResult) => void
    }): BdtResult;

    send(options: {
        data: Buffer,
        onSend: (err: BdtResult, sentBuffer: Buffer, sentSize: number) => void
    }): BdtResult;

    recv(options: {
        buffer: Buffer,
        onRecv?: (recvBuffer: Buffer, recvSize: number) => void
    }): BdtResult;
    
    close(): BdtResult;
    reset(): BdtResult;

    // on(event: 'data', listener: (buffer: Buffer, connection: Connection) => void): this;
    // on(event: 'drain', listener: (connection: Connection) => void): this;
    // on(eventName: string, listener: (...args: any[]) => void): this;

    // once(event: 'connect', listener: (connection: Connection) => void): this;
    // once(event: 'end', listener: (connection: Connection) => void): this;
    // once(event: 'close', listener: (connection: Connection) => void): this;
    // once(event: 'error', listener: (connection: Connection) => void): this;
    // once(eventName: string, listener: (...args: any[]) => void): this;
}

interface NetModifier {
    changeLocalAddress(options: {
        src: String, 
        dst: String
    }): BdtResult;

    apply(): BdtResult;
}

interface Stack {
    // readonly localPeer: LocalPeerInfo;

    // close(): BdtResult;

    connectRemote(options: {
        remote: Peerid,
        constInfo: PeerConstInfo,
        vport: number,
        question?: Buffer,
        supernodes?: Array<Peerid>,
        onConnect: (connection: Connection, err: BdtResult) => void,
        onAnswer?: (firstAnswer: Buffer) => void,
        // isEncrypted?: boolean
    } & BuildTunnelOptions): BdtResult;

    listenConnection(options: {
        vport: number,
        // options?: ListenConnectionConfig
    }): BdtResult;

    acceptConnection(options: {
        vport: number,
        onAccept: (connection: Connection, question?: Buffer) => void
    }): BdtResult;

    // stop-listen?

    // on(event: 'connection', listener: (connection: Connection, firstQuestion?: Buffer) => void): this;
    // on(eventName: string, listener: (...args: any[]) => void): this;

    addStaticPeerInfo(peerInfo: PeerInfo): void;

    createNetModifier(): NetModifier;
}

type StackOptions = {
    local: LocalPeerInfo,
    initialPeers: Array<PeerInfo>,
}

interface StackConstructor {
    new(StackOptions): Stack;
}

declare const Stack: StackConstructor;
