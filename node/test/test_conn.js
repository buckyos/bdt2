let {Stack, PeerConstInfo} = require('../build/Debug/bdt2-js-c');
const NodeRSA = require('node-rsa');

function createPeerInfo(deviceId)
{
    let rsaKeyPair = new NodeRSA({b: 1024});
    let secret = rsaKeyPair.exportKey('private-der');
    let publicKey = rsaKeyPair.exportKey('public-der');
    return {
        constInfo: {
            areaCode: {
                continent: 0,
                country: 0,
                carrier: 0,
                city: 0,
                inner: 0,
            },
            deviceId, 
            publicKey, 
        }, 
        supernodes: [],
        secret, 
    };
}

let peerLocal = createPeerInfo('local');
peerLocal.endpoints = ['v4udp127.0.0.1:10000', ];

console.log(peerLocal)

let peerRemote = createPeerInfo('remote');
peerRemote.endpoints = ['v4udp127.0.0.1:10001', ];

let stackLocal = new Stack({
    local: peerLocal, 
    initialPeers: [peerRemote, ]});

let stackRemote = new Stack({local: peerRemote});

stackRemote.listenConnection({vport: 1});
stackRemote.acceptConnection({
    vport: 1, 
    onAccept: (remoteConn, question) => {
        console.log('conn accept ', question.toString('utf8'));
        remoteConn.confirm({
            answer: Buffer.from('first answer'), 
            onConnect: (result) => {
                console.log('conn remote establish');
                let buffer = new Buffer(100);
                remoteConn.recv({
                    buffer, 
                    onRecv: (buffer, recv) => {
                        console.log('recv ', buffer.toString(), '  ', recv);
                    } 
                });
            }
        });
    }   
});

stackLocal.connectRemote({
    remote: PeerConstInfo.toPeerid(peerRemote.constInfo),
    vport: 1, 
    constInfo: peerRemote.constInfo, 
    question: Buffer.from('first question'), 
    onAnswer: (answer) => {
        console.log('conn pre accepted ', answer.toString());
    }, 
    onConnect: (connection, result) => {
        localConn = connection;
        console.log('conn local establish');
        localConn.send({
            data: Buffer.from('first data'), 
            onSend: (result, data, sent) => {
                console.log(data.toString(), ' sent ', sent);
            }
        });
    }
});

let m = stackLocal.createNetModifier();
m.changeLocalAddress({
    src: 'v4unk1.1.1.1:1', 
    dst: 'v4unk2.2.2.2:2'} 
);
m.apply();