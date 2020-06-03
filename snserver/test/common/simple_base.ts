import { initUnhandledRejection, initLogger, PeerConstInfo, Peerid, Endpoint, PeerMutableInfo, EndpointList, EndpointOptions, PeerPublicKeyType, rsa } from '../../src/base';
import { LocalPeer } from '../../src/framework';

// 等待 x 毫秒
export const delay = (ms: number) => new Promise((res) => setTimeout(res, ms));

const logger = initLogger({
    loggerOptions: {
        console: false, 
        file: {
            root: 'e:/project/bdt2.0/logs',
            filename: 'debug.log',
        },
        level: 'debug',
    }
});

// initUnhandledRejection(logger);

export { logger };

export function getConstInfo(
    deviceId: Buffer,
    options?: {
        keyPair: {
            publicKey: Buffer,
            secret: Buffer
        }
    }): {
    constInfo: PeerConstInfo,
    peerSecret: Buffer,
} {
    let areaCode = {
        continent: 0,
        country: 0,
        carrier: 0,
        city: 0,
        inner: 0
    };
    let constInfo = new PeerConstInfo();

    // const publicKeyType = PeerPublicKeyType.SECP256K1;
    // const publicKey = Buffer.from('03b5c2396cd2a045e05a057a56cb6a862e57c1b9e75b817d55fc8f83ef23b60413');
    // const peerSecret = Buffer.from('68f99f942bd66d85934b04baea93c7a2b19e34684325cd3073ac74db925cf223');

    const publicKeyType = PeerPublicKeyType.RSA1024;
    // const [publicKey, peerSecret] = rsa.createKeyPair();
    let publicKey = Buffer.from('30819f300d06092a864886f70d010101050003818d0030818902818100c6ac143ffbe5f7cdd8299799d50eba1fd319bd386419eafa1d0030b4302ac601e2b31b793d169264b077e055b0564da7684cd3c5af6ef9ff05c6deb86bad28f848c1850063964b0040a1ca7908bcd474a15ea4fe4c45985485a60d62d9a02e1d0333c12ca04beeebfad3718e83fa4f55fcfb2e2449825e9c1938c8fc1cec6f1d0203010001', 'hex');
    let peerSecret = Buffer.from('3082025d02010002818100c6ac143ffbe5f7cdd8299799d50eba1fd319bd386419eafa1d0030b4302ac601e2b31b793d169264b077e055b0564da7684cd3c5af6ef9ff05c6deb86bad28f848c1850063964b0040a1ca7908bcd474a15ea4fe4c45985485a60d62d9a02e1d0333c12ca04beeebfad3718e83fa4f55fcfb2e2449825e9c1938c8fc1cec6f1d020301000102818069b04155887bde47f326ad2f78d84ccb8151c007afb8d9f81455759365cc5b69c55fe2cdac61c59f2b019aa5fb18ee569075ce72ebd0edcd6d154e866d41c0f8e444a95a1116fea3416cc914e1aa736272a6036c2940e3950812a6f3f3651fb0892094dce189fa7fb342e315c10b28e098aebab8869d6bd1ea5dc27cd7709601024100e43c8b72dbecae1766ca41f8be2f461a3ffb2b782da7dd0b0574d668c78c5355d810da5370ac7c90958fb8c1c857646f44f9de5ec08bc740f9dc9d4220680fa9024100ded6e120131c0b9f6c1c4d64d6999eb4ccb53322662fabda644ab756ece2b922bdfc07690ee95fb92a449d7ad0c6e9a3dbb9bcfabef674b3e55325c63fdadc55024048e92e831ace992ed09f7c43c23dc7df4ae12a19d23bf5d9377d03a1a55da6a19dcc8472736426c6e980683d3f8aeb82c03e3253829f24a01531eceadaff6341024100968a6fd7ed50fb011e56186ad11742c23db103f46f38314efe860349b40a8eabcbd1216875ec6f00766f983bca2336dfbda6c2e65a6fc0f36f2f36cbb183b7690241009af875d6ecc6f414d82c1fd716de909ceb6503c4aebae0ab8edeb52a6fa0fd1fee25e6835bf1a68d6c392bf438f97c4dc791bf95a426268c9dc8f8bba6ca251d', 'hex');

    if (options) {
        publicKey = options.keyPair.publicKey;
        peerSecret = options.keyPair.secret;
    }

    constInfo.fromObject({
        areaCode,
        deviceId,
        publicKeyType,
        publicKey,
    });

    return {
        constInfo,
        peerSecret,
    };
}

export function newLocalPeer(
    deviceId: Buffer,
    eplist: EndpointOptions[],
    options?: {
        keyPair: {
            publicKey: Buffer,
            secret: Buffer
        }
    }): LocalPeer {
    const { constInfo, peerSecret } = getConstInfo(deviceId, options);
    const peerid = new Peerid();
    const err = peerid.fromConstInfo(constInfo);

    const mutableInfo = new PeerMutableInfo();
    const endpointList = new EndpointList();
    if (eplist) {
        for (const ep of eplist) {
            endpointList.addEndpoint(new Endpoint(ep));
        }
    }
    mutableInfo.fromObject({
        endpointList
    });

    let localPeer = new LocalPeer({logger});
    localPeer.init({
        constInfo,
        secret: peerSecret,
        mutableInfo 
    });

    return localPeer;
}
