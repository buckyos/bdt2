'use strict'

const assert = require('assert');
const net = require('net');
const {Stack, Connection, PeerConstInfo, BdtResult, NetModifier} = require('./bdt2-js-c');

const PEER_ID_LENGTH = 32;
const MAX_DEVICE_ID_LENGTH = 128;

function isBuffer(buf) {
    return (Buffer.isBuffer(buf) || buf instanceof ArrayBuffer);
}

const KeyPair = {
    rsa1024: 0,
    rsa2048: 1,
    rsa3072: 2,

    generate(keyType) {
        const NodeRSA = require('node-rsa');
        let bits = 1024;
        switch (keyType) {
            case KeyPair.rsa2048:
                bits = 2048;
                break;
            case KeyPair.rsa3072:
                bits = 3072;
                break;
        }

        const maxSecretLength = KeyPair.getMaxSecretKeyLength(keyType);

        while (true) {
            let rsaKeyPair = new NodeRSA({b: bits});
            let secret = rsaKeyPair.exportKey('private-der');
            if (secret.byteLength < maxSecretLength) {
                return {
                    secret,
                    publicKey: rsaKeyPair.exportKey('public-der'),
                }
            }
        }

    },

    getPublicKeyLength(keyType) {
        if (!keyType) { // 默认 KeyPair.rsa1024
            return 162;
        }

        switch (keyType) {
            case KeyPair.rsa2048:
                return 249;
            case KeyPair.rsa3072:
                return 422;
        }
        assert(false, 'unkown key type.');
    },

    getMaxSecretKeyLength(keyType) {
        if (!keyType) { // 默认 KeyPair.rsa1024
            return 1770; // 610;
        }

        switch (keyType) {
            case KeyPair.rsa2048:
                return 1770; // 1194;
            case KeyPair.rsa3072:
                return 1770;
        }
        assert(false, 'unkown key type.');
    },
}

const JSPeerConstInfo = {
    toPeerid(constInfo) {
        assert(JSPeerConstInfo._check(constInfo));

        return PeerConstInfo.toPeerid(constInfo);
    },

    _check(constInfo) {
        assert(constInfo.areaCode);
        assert(constInfo.areaCode.continent >= 0 && constInfo.areaCode.continent <= 63);
        assert(constInfo.areaCode.country >= 0 && constInfo.areaCode.country <= 255);
        assert(constInfo.areaCode.carrier >= 0 && constInfo.areaCode.carrier <= 15);
        assert(constInfo.areaCode.city >= 0 && constInfo.areaCode.city <= (1 << 14 - 1)); // 14bit
        assert(constInfo.areaCode.inner >= 0 && constInfo.areaCode.inner <= 255);
        assert((isBuffer(constInfo.deviceId) || typeof constInfo.deviceId === 'string') && (constInfo.deviceId.length || constInfo.deviceId.byteLength) <= MAX_DEVICE_ID_LENGTH);
        assert(!constInfo.publicKeyType || (constInfo.publicKeyType >= 0 && constInfo.publicKeyType <= 2));
        assert(isBuffer(constInfo.publicKey) && constInfo.publicKey.byteLength === KeyPair.getPublicKeyLength(constInfo.publicKeyType));
        return true;
    },

    _checkPeerid(peerid) {
        const ok = (isBuffer(peerid) && peerid.byteLength === PEER_ID_LENGTH);
        assert(ok);
        return ok;
    },

    _checkPeeridList(peeridList) {
        assert(Array.isArray(peeridList));
        for (const peerid of peeridList) {
            assert(JSPeerConstInfo._checkPeerid(peerid));
        }
        return true;
    }
}

function makeEndpoint(ip, port, protocol) {
    assert(net.isIP(ip));
    assert(port > 0 && port < 65536);
    assert(protocol && (protocol === 'udp' || protocol === 'tcp' || protocol === 'unk'));

    const v = net.isIPv6(ip)? 'v6' : 'v4';
    return `${v}${protocol}${ip}:${port}`;
}

function checkSuperNodesFieldInPeerInfo(supernodes) {
    assert(!supernodes || Array.isArray(supernodes));
    if (supernodes) {
        for (const sn of supernodes) {
            if (isBuffer(sn)) {
                assert(sn.byteLength === PEER_ID_LENGTH);
            } else {
                assert(JSPeerInfo._check(sn));
            }
        }
    }
    return true;
}

const JSPeerInfo = {
    _check(peerInfo) {
        // 暂时没有用
        // signature[BDT_PEER_LENGTH_SIGNATRUE];
        // createTime;

        assert(!peerInfo.peerid ||
            (JSPeerConstInfo._checkPeerid(peerInfo.peerid) &&
                (Buffer.compare(Buffer.from(peerInfo.peerid), Buffer.from(JSPeerConstInfo.toPeerid(peerInfo.constInfo))) === 0)
            )
        );

        assert(JSPeerInfo._checkEndpointList(peerInfo.endpoints));

        assert(checkSuperNodesFieldInPeerInfo(peerInfo.supernodes));

        assert(!peerInfo.secret ||
            (isBuffer(peerInfo.secret) && peerInfo.secret.byteLength <= KeyPair.getMaxSecretKeyLength(peerInfo.constInfo.publicKeyType)),
			`${(peerInfo.secret && peerInfo.secret.byteLength > 0)? peerInfo.secret.byteLength : peerInfo.secret}`);
    
        return true;
    },

    _checkList(peerInfoList) {
        assert(Array.isArray(peerInfoList));
        for (const peerinfo of peerInfoList) {
            assert(JSPeerInfo._check(peerinfo));
        }
        return true;
    },

    _checkEndpoint(ep) {
        const fields = ep.split(':');
        assert(fields.length === 2);
        const ip = fields[0].slice(5);
        const protocol = fields[0].slice(2, 5);
        const port = fields[1];
        assert(makeEndpoint(ip, port, protocol) === ep);
        return true;
    },

    _checkEndpointList(epList) {
        assert(Array.isArray(epList));
        for (const ep of epList) {
            assert(JSPeerInfo._checkEndpoint(ep));
        }
        return true;
    }
}

function wrapNativeCallback(callback) {
    return function (...args) {
        try {
            callback(...args);
        } catch (error) {
            setImmediate(() => {throw error});
        }
    }
}

class JSStack {
    constructor(options) {
        assert(JSPeerInfo._check(options.local));
        assert(!options.initialPeers || JSPeerInfo._checkList(options.initialPeers));

        this.m_stack = new Stack(options);
    }

    connectRemote(options) {
        assert(JSPeerConstInfo._checkPeerid(options.remote));
        assert(JSPeerConstInfo._check(options.constInfo));
        assert(options.vport >= 0 && options.vport <= 65535);
        assert(!options.question || isBuffer(options.question));
        assert(!options.supernodes || JSPeerConstInfo._checkPeeridList(options.supernodes));

        assert(typeof options.onConnect === 'function');
        assert(!options.onAnswer || typeof options.onAnswer === 'function');

        const onConnect = (nativeConnection, result) => {
            let connection = null;
            if (nativeConnection) {
                connection = new JSConnection(nativeConnection);
            }
            options.onConnect(connection, result);
        };

        let nativeOptions = {...options};
        nativeOptions.onConnect = wrapNativeCallback(onConnect);
        if (options.onAnswer) {
            nativeOptions.onAnswer = wrapNativeCallback(options.onAnswer);
        }
        return this.m_stack.connectRemote(nativeOptions);
    }

    listenConnection(options) {
        assert(options.vport >= 0 && options.vport <= 65535);
        return this.m_stack.listenConnection(options);
    }

    acceptConnection(options) {
        assert(options.vport >= 0 && options.vport <= 65535);
        assert(typeof options.onAccept === 'function');

        const onAccept = (nativeConnection, question) => {
            assert(nativeConnection);
            let connection = new JSConnection(nativeConnection);
            return options.onAccept(connection, question);
        }

        let nativeOptions = {...options};
        nativeOptions.onAccept = wrapNativeCallback(onAccept);
        return this.m_stack.acceptConnection(nativeOptions);
    }

    addStaticPeerInfo(peerInfo) {
        assert(JSPeerInfo._check(peerInfo));

        this.m_stack.addStaticPeerInfo(peerInfo);
    }

    createNetModifier() {
        return new JSNetModifier(this.m_stack.createNetModifier());
    }
}

class JSConnection {
    constructor(connection) {
        this.m_connection = connection;
    }

    confirm(options) {
        assert(!options.answer || isBuffer(options.answer));
        assert(typeof options.onConnect === 'function');

        let nativeOptions = {...options};
        nativeOptions.onConnect = wrapNativeCallback(options.onConnect);
        this.m_connection.confirm(nativeOptions);
    }

    send(options) {
        assert(isBuffer(options.data) && options.data.byteLength > 0);
        assert(typeof options.onSend === 'function');

        let nativeOptions = {...options};
        nativeOptions.onSend = wrapNativeCallback(options.onSend);
        return this.m_connection.send(nativeOptions);
    }

    recv(options) {
        assert(typeof options.onRecv === 'function');
        
        let nativeOptions = {...options};
        nativeOptions.onRecv = wrapNativeCallback(options.onRecv);
        return this.m_connection.recv(nativeOptions);
    }
    
    close() {
        return this.m_connection.close();
    }

    reset() {
        return this.m_connection.reset();
    }

    get name() {
        return this.m_connection.name;
    }

    get providerName() {
        return this.m_connection.providerName;
    }
}

class JSNetModifier {
    constructor(netModifier) {
        this.m_netModifier = netModifier;
    }

    changeLocalAddress(options) {
        assert(JSPeerInfo._checkEndpoint(options.src));
        assert(JSPeerInfo._checkEndpoint(options.dst));

        return this.m_netModifier.changeLocalAddress(options);
    }

    apply() {
        return this.m_netModifier.apply();
    }
}

module.exports = {
    Stack: JSStack,
    Connection: JSConnection,
    PeerConstInfo: JSPeerConstInfo,
    NetModifier: JSNetModifier,
    BdtResult,
    makeEndpoint,
    KeyPair,
};
