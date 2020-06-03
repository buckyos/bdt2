import { Endpoint, EndpointList, EndpointProtocol, EndpointIPVersion, ErrorCode, Peerid, PeerConstInfo, PeerMutableInfo, PeerPublicKeyType, Peer, EndpointOptions } from '../../src/base';
import { SuperNodeServer } from '../../src/framework/super_node';
import { PeerInfo } from '../../src/js-c/protocol';
import { NetworkManager, LocalPeer } from '../../src/framework';
import { PeerHistoryManagerImpl } from '../../src/framework/peer_history';
import { SequenceCreator } from '../../src/framework/sequence';
import { logger, newLocalPeer } from './simple_base';
import { NoImplPeerHistoryManager } from './simple_history_manager';

// 快速启动 测试的sn-server
export async function quickServerInit(options = {
    ip: '127.0.0.1',
    udpPort: 1234,
    tcpPort: 2345,
    encrypted: false,
}): Promise<{err: ErrorCode, server: SimpleServer}> {
    const server = new SimpleServer();
    const localPeer = newLocalPeer(Buffer.from('sn--server'), [
        {
            protocol: EndpointProtocol.udp,
            ipVersion: EndpointIPVersion.ipv4,
            address: options.ip,
            port: options.udpPort,
        },
        {
            protocol: EndpointProtocol.tcp,
            ipVersion: EndpointIPVersion.ipv4,
            address: options.ip,
            port: options.tcpPort,
        }
    ]);
    
    const err = await server.init({
        localPeer,
    });

    return {
        err,
        server,
    };
}

export class SimpleServer {
    constructor() {

    }

    async init(options: {
        localPeer: LocalPeer,
        encrypted?: boolean,
    }): Promise<ErrorCode> {
        let networkManager = new NetworkManager({logger});
        const networkInitRet = await networkManager.init({localPeer: options.localPeer, historyManager: new PeerHistoryManagerImpl({logger, config: {keyExpire: 24 * 3600000, peerExpire: 24 * 3600000, logExpire: 24 * 3600000}})});
        if (networkInitRet.err) {
            return networkInitRet.err;
        }

        this.m_networkManager = networkManager;
        
        this.m_sn = new SuperNodeServer({logger, config: {
            tcpFreeInterval: 5 * 1000, 
            peerManager: {
                timeout: 60809, 
                capacity: 160809
            }, 
            resendQueue: {
                defaultInterval: 489, 
                maxTimes: 5
            }
        }});
        this.m_sn.init({
            networkManager, 
            encrypted: options.encrypted,
        });
        return ErrorCode.success;
    }

    async destroy() {
        this.m_sn!.destroy();
        let opts: Promise<ErrorCode>[] = [];
        this.m_networkManager!.udpInterfaces.forEach((udpInterface) => opts.push(udpInterface.close()));
        return Promise.all(opts);
    }

    get localPeer(): PeerInfo {
        return this.m_sn!.localPeer;
    }

    private m_sn?: SuperNodeServer;
    private m_networkManager?: NetworkManager;
}

const snKeyPair = {
    publicKey: Buffer.from('30819f300d06092a864886f70d010101050003818d0030818902818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a50203010001', 'hex'),
    secret: Buffer.from('3082025e02010002818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a502030100010281803002b8ddbca99a3c3d809b5703bc1646d03ae2fbc2ccfa5777d6a2516285c46a1ebc765e28334bd0638cb5d0ecd41bcbb3a39149c5b47368ed871c0b9d81d2f459c20e3103257ecb8f8961e90b7cc958487a995176d1fbcd5cb860f8f4b714372bd2315bedd0348cb3a51c9c0bc6ebcb3c719e564d9fda7f8115170e9452e171024100d9edcd5b8e1dc265667d518e1b91a555811c4ab11aeae260fca51e03ceb592b84b18ef018d59e51f1208e8700203234dd4f3847a5f7819f1285224186865deb7024100d80c3d006eabb1a589429d2fcb244320d7267f8e0d8676857164157b605710f81aa57914e8fa80f5c327ef1c02fb3b6e66f0bc0e563a5414faa2ecbad17496830241008cf569bcec818739bb3f17bf4949bd9d3eb3a404461ae36e443c30dbd99a4c5a74089e9f6c6456f4efdf5f2903c42fd3aa08110a6e31eae5b764da000796cca50241008a64c48acb59e671108d005dc636135e2d13f72f8ad07089a88a210ca838fda0c088f11808e9b6c437601456103ed8e22ec4d4e2263034fe3f53306bb792847b024100cfe6eb463fde203546aab93eeaba59b2fa31de343aa00308ea5ed97d68fa864803f895e77042c24ba364342131a9fe32dac4eac8ba0acde7ddf967e9a0a23b6e', 'hex'),
};

let snLocalPeer = newLocalPeer(Buffer.from('sn--server'),
[{
    protocol: EndpointProtocol.udp,
    ipVersion: EndpointIPVersion.ipv4,
    address: '106.75.175.123',
    port: 10020,
},
{
    protocol: EndpointProtocol.tcp,
    ipVersion: EndpointIPVersion.ipv4,
    address: '106.75.175.123',
    port: 10030,
}],
{
    keyPair: snKeyPair,
});

export let publicSNForTest = {
    peerid: snLocalPeer.peerid,
    constInfo: snLocalPeer.constInfo,
    mutableInfo: snLocalPeer.mutableInfo,
};
