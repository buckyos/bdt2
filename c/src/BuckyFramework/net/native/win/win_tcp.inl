#include "./win_tcp.h"
#include "./sock_iocp.inl"


static void _fillTCPSocketEvents(_NativeTCPSocket* sock);

static _NativeTCPSocket* _newTCPSocket(_BuckyFramework* framework, const BfxUserData* udata) {

    BuckyNativeSocketType sock = _createRawTcpSocket();
    if (!BFX_NATIVE_SOCKET_IS_VALID(sock)) {
        return NULL;
    }

    _NativeTCPSocket* sock = (_NativeTCPSocket*)BfxMalloc(sizeof(_NativeTCPSocket));
    memset(sock, 0, sizeof(_NativeTCPSocket));

    _initSocketCommon((_NativeSocketCommon*)sock, framework, udata);

    sock->socket = sock;
    sock->backlog = 128;

    _initSendPool(&sock->sendPool, 1024 * 1024 * 8);
    
    sock->recvBufferSize = 1024 * 8;
    sock->recvBuffer = BfxMalloc(sock->recvBufferSize);

    _fillTCPSocketEvents(sock);

    return sock;
}

// 最后释放之前，必须已经被正确close
static void _deleteTCPSocket(_NativeTCPSocket* sock) {

    BLOG_CHECK(_nativeSocketIsClosed(sock));
    BLOG_CHECK(!BFX_NATIVE_SOCKET_IS_VALID(sock->socket));

    _uninitSendPool(&sock->sendPool);

    _uninitSocketCommon((_NativeSocketCommon*)sock);

    BfxFree(sock);
}


static int32_t _tcpSocketAddref(_NativeTCPSocket* sock) {
    assert(sock->ref > 0);
    return BfxAtomicInc32(&sock->ref);
}

static int32_t _tcpSocketRelease(_NativeTCPSocket* sock) {
    assert(sock->ref > 0);
    int32_t ret = BfxAtomicDec32(&sock->ref);
    if (ret == 0) {
        _deleteTCPSocket(sock);
    }

    return ret;
}

static void _closeTCPSocket(_NativeTCPSocket* sock) {

    int state = BfxAtomicExchange32(&sock->state, _NativeSocketState_Closed);
    if (state == _NativeSocketState_Closed) {
        BLOG_ERROR("socket been closed already! sock=%p", sock);
        return;
    }

    if (sock->socket != BFX_NATIVE_SOCKET_INVALID_HANDLE) {
        _closeRawSocket(sock->socket);
        sock->socket = BFX_NATIVE_SOCKET_INVALID_HANDLE;
    }
}

//////////////////////////////////////////////////////////////////////////
// listen & accept

// new connection event
static void _tcpConnectionEvent(_NativeTCPSocket* socket, int err, _NativeTCPSocket* comingSocket) {

    do {
        if (_nativeSocketIsClosed(socket)) {
            BLOG_WARN("recv connection event but socket has been closed! socket=%p, err=", socket->socket, err);
            break;
        }

        if (err) {
            BLOG_ERROR("tcp socket connection error, socket=%p, err=", socket->socket, err);
            socket->framework->tcpEvents.error(socket->socket, err);
            break;
        }

        BLOG_CHECK(socket->state == _NativeSocketState_Listen);
        BLOG_CHECK(comingSocket);

        socket->framework->tcpEvents.connection(socket, comingSocket);
        comingSocket = NULL;

    } while (0);

    if (comingSocket) {
        _tcpSocketRelease(comingSocket);
    }
}

static int _tcpSocketListen(_NativeTCPSocket* sock, const BdtEndpoint* endpoint) {

    int ret = 0;
    do {
        int state = BfxAtomicCompareAndSwap32(sock->state, _NativeSocketState_Init, _NativeSocketState_Bind);
        if (state != _NativeSocketState_Init) {
            ret = BFX_RESULT_INVALID_STATE;
            break;
        }

        ret = _rawSocketBind(sock->socket, endpoint);
        if (ret != 0) {
            sock->state = _NativeSocketState_Init;
            break;
        }

        state = BfxAtomicCompareAndSwap32(sock->state, _NativeSocketState_Bind, _NativeSocketState_Listen);
        if (state != _NativeSocketState_Bind) {
            BLOG_ERROR("invalid tcp socket state for listen, state=%d", state);
            ret = BFX_RESULT_INVALID_STATE;
            break;
        }

        ret = _tcpSocketListenRaw(sock);
        if (ret != 0) {
            sock->state = _NativeSocketState_Bind;
            break;
        }

        ret = _tcpSocketAcceptRaw(sock);

    } while (0);

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// connect

// connect event
static void _tcpConnectEvent(_NativeTCPSocket* socket, int err) {
    if (_nativeSocketIsClosed(socket)) {
        BLOG_WARN("recv connect event but socket has been closed! socket=%p, err=", socket->socket, err);
        return;
    }

    if (err) {
        BLOG_ERROR("tcp socket connect error, socket=%p, err=", socket->socket, err);
        socket->framework->tcpEvents.error(socket->socket, err);
        return;
    }

    int state = BfxAtomicCompareAndSwap32(&socket->state, _NativeSocketState_Connecting, _NativeSocketState_Connected);
    if (state != _NativeSocketState_Connecting) {
        BLOG_ERROR("invalid tcp socket state, expect=%d, cur=%d", _NativeSocketState_Connecting, state);
        socket->framework->tcpEvents.error(socket->socket, BFX_RESULT_INVALID_STATE);
        return;
    }

    _BuckyFramework* bframe = socket->framework;
    bframe->tcpEvents.established(socket);
}


static int _tcpSocketConnect(_NativeTCPSocket* socket,
    const BdtEndpoint* local,
    const BdtEndpoint* remote) {

    int ret = 0;
    do {
        int state = BfxAtomicCompareAndSwap32(sock->state, _NativeSocketState_Init, _NativeSocketState_Bind);
        if (state != _NativeSocketState_Init) {
            ret = BFX_RESULT_INVALID_STATE;
            break;
        }

        ret = _rawSocketBind(sock, endpoint);
        if (ret != 0) {
            sock->state = _NativeSocketState_Init;
            break;
        }

        state = BfxAtomicCompareAndSwap32(sock->state, _NativeSocketState_Bind, _NativeSocketState_Connecting);
        if (state != _NativeSocketState_Bind) {
            BLOG_ERROR("invalid tcp socket state for listen, state=%d", state);
            ret = BFX_RESULT_INVALID_STATE;
            break;
        }

        ret = _tcpSocketConnectRaw(sock, remote);
        if (ret != 0) {
            sock->state = _NativeSocketState_Bind;
            break;
        }

    } while (0);

    return ret;
}

static void _tcpSendEvent(_NativeTCPSocket* socket, int err, void* buffer, size_t len) {

    do {
        if (_nativeSocketIsClosed(socket)) {
            BLOG_WARN("send complete event but socket has been closed! socket=%p, err=%d", socket->socket, err);
            break;
        }

        if (err) {
            BLOG_ERROR("tcp socket send error, socket=%p, err=%d", socket->socket, err);
            socket->framework->tcpEvents.error(socket->socket, err);
            break;
        }

        int ret = _releaseSend(&socket->sendPool, len);
        if (ret == -1) {
            socket->framework->tcpEvents.drain(socket->socket);
        }
    } while (0);

    // 释放缓冲区
    BfxFree(buffer);
}

static int  _nativeTcpSocketSend(_NativeTCPSocket* socket, const uint8_t* buffer, size_t* inoutSent) {

    int ret = _requireSend(&socket->sendPool, inoutSent);
    if (*inoutSent == 0) {
        return ret;
    }

    // TODO 拷贝内存
    void* sendBuffer = BfxMalloc(*inoutSent);
    memcpy(sendBuffer, buffer, *inoutSent);

    int sendRet = _tcpSocketSend(socket, buffer, *inoutSent);
    if (sendRet != 0) {
        return sendRet;
    }

    return ret;
}

static int _nativeTcpSocketPause(_NativeTCPSocket* socket) {
    int ret = BfxAtomicCompareAndSwap32(&sock->reading, 1, 0);
    if (ret != 1) {
        return 0;
    }

    return 0;
}

static int _requireRecv(_NativeTCPSocket* socket, void** buffer, size_t* len) {

    // TODO 
    BLOG_CHECK(socket->recvBuffer);
    *buffer = socket->recvBuffer;
    socket->recvBuffer = NULL;
    *len = socket->recvBufferSize;
}

static int _releaseRecv(_NativeTCPSocket* socket, void* buffer) {
    BLOG_CHECK(socket->recvBuffer == NULL);
    socket->recvBuffer = buffer;

    return 0;
}

static int _recvOnce(_NativeTCPSocket* socket) {
    void* buffer = NULL;
    size_t len = 0;
    _requireRecv(socket, &buffer, &len);

    int ret = _tcpSocketRecv(socket, buffer, len);
    if (ret != 0) {
        BLOG_ERROR("tcp socket recv got error! err=%d", ret);
        socket->framework->tcpEvents.error(socket->socket, ret);

        return ret;
    }

    // 等待事件
}

static int _nativeTcpSocketResume(_NativeTCPSocket* socket) {
    int ret = BfxAtomicCompareAndSwap32(&sock->reading, 0, 1);
    if (ret != 0) {
        BLOG_WARN("tcp socket already in reading! sock=%p", socket->socket);
        return 0;
    }

    // 发起recv
    return _recvOnce(socket);
}

static void _nativeTcpRecvEvent(_NativeTCPSocket* socket, int err, void* buffer, size_t len) {
    do {
        if (_nativeSocketIsClosed(socket)) {
            BLOG_WARN("recv complete event but socket has been closed! socket=%p, err=%d", socket->socket, err);
            break;
        }

        if (err) {
            // TODO 出错后如何处理？
            BLOG_ERROR("tcp socket recv error, socket=%p, err=%d", socket->socket, err);
            socket->framework->tcpEvents.error(socket->socket, err);
        } else {
            socket->framework->tcpEvents.data(socket->socket, (char*)buffer, len);
        }

        int ret = _releaseRecv(socket, buffer);
        buffer = NULL;

        // TODO error后是否要继续？
        if (socket->reading) {
            _recvOnce(socket);
        }
    } while (0);

    if (buffer) {
        BfxFree(buffer);
    }
}

static void _nativeTcpErrorEvent(_NativeTCPSocket* socket, int err) {
    BLOG_ERROR("tcp socket error, socket=%p, err=%d", socket->socket, err);
    socket->framework->tcpEvents.error(socket->socket, err);
}

static void _fillTCPSocketEvents(_NativeTCPSocket* sock) {
    sock->events.connect = _tcpConnectEvent;
    sock->events.connection = _tcpConnectionEvent;
    sock->events.send = _tcpSendEvent;
    sock->events.recv = _nativeTcpRecvEvent;
    sock->events.error = _nativeTcpErrorEvent;
}