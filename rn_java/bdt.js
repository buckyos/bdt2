import { NativeModules } from 'react-native';

const { BdtStack,
    BdtConnection,
    BdtPeerConstInfo,
    BdtNetModifier,
    BdtResult
} = NativeModules;

let g_lastReturnValue = BdtResult.failed;
function onReturnOneValue(arg) {
    g_lastReturnValue = arg;
};

class Stack {
    constructor(options) {
        const onDone = (err, stackId) => {
            if (err) {
                throw 'create bdt stack failed.';
            } else {
                this.m_stackId = stackId;
            }
        };

        BdtStack.create(options.local,
            options.initialPeers,
            onDone,
        );
    }

    connectRemote(options) {
        const onConnect = (err, connectId) => {
            let connection = null;
            if (err === BdtResult.success) {
                connection = new Connection(connectId);
            }
            options.onConnect(connection, err);
        };

        BdtStack.connectRemote(this.m_stackId,
            options.constInfo,
            options.vport,
            options.supernodes,
            options.question,
            options.onAnswer,
            onReturnOneValue,
            onConnect
        );

        return g_lastReturnValue;
    }

    listenConnection(options) {
        BdtStack.listenConnection(this.m_stackId, options.vport, onReturnOneValue);
        return g_lastReturnValue;
    }

    acceptConnection(options) {
        const onPreAccept = (connectId, question) => {
            let connection = new Connection(connectId);
            options.onAccept(connection, question);
        }

        const onError = err => {
            options.onAccept(null, null);
        };

        BdtStack.acceptConnection(this.m_stackId,
            options.vport,
            onReturnOneValue,
            onPreAccept,
            onError);
        return g_lastReturnValue;
    }

    addStaticPeerInfo(peerInfo) {
        // <TODO>
        // BdtStack.addStaticPeerInfo(this.m_stackId, peerInfo);
    }

    createNetModifier() {
        // <TODO>
        // BdtStack.createNetModifier(this.m_stackId, onReturnOneValue);
        if (g_lastReturnValue > 0) {
            // return new NetModifier(g_lastReturnValue);
        }
        return null;
    }
}

class Connection {
    constructor(connectId) {
        this.m_connectId = connectId;
    }

    confirm(options) {
        BdtConnection.confirm(this.m_connectId,
            options.answer,
            onReturnOneValue,
            options.onConnect,
        );
        return g_lastReturnValue;
    }

    send(options) {
        const onSend = (err, sentSize) => {
            options.onSend(err, options.data, sentSize);
        };

        BdtConnection.send(this.m_connectId,
            options.data,
            onReturnOneValue,
            onSend
        );
        return g_lastReturnValue;
    }

    recv(options) {
        BdtConnection.recv(this.m_connectId,
            options.buffer,
            onReturnOneValue,
            options.onRecv
        );
        return g_lastReturnValue;
    }

    close() {
        BdtConnection.close(this.m_connectId, onReturnOneValue);
        return g_lastReturnValue;
    }

    reset() {
        BdtConnection.reset(this.m_connectId);
    }
}

class NetModifier {
    constructor(modifierId) {
        this.m_modifierId = modifierId;
    }

    changeLocalAddress(options) {
        // <TODO>
        // BdtNetModifier.changeLocalAddress(this.m_modifierId,
        //     options.src,
        //     options.dst,
        //     onReturnOneValue
        // );
        return g_lastReturnValue;
    }

    apply() {
        // <TODO>
        // BdtNetModifier.apply(this.m_modifierId, onReturnOneValue);
        return g_lastReturnValue;
    }
}

class PeerConstInfo {
    static toPeerid(constInfo) {
        BdtPeerConstInfo.toPeerid(constInfo, onReturnOneValue);
        return g_lastReturnValue;
    }
}

module.exports = {
    Stack,
    Connection,
    PeerConstInfo,
    NetModifier,
    BdtResult,
};