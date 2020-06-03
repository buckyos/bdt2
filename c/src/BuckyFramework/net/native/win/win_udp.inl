#include "./win_udp.h"
#include "./sock_iocp.inl"


static void _fillUDPSocketEvents(_NativeUDPSocket* sock);

static _NativeUDPSocket* _newTCPSocket(_BuckyFramework* framework, const BfxUserData* udata) {

    BuckyNativeSocketType sock = _createUdpSocket();
    if (!BFX_NATIVE_SOCKET_IS_VALID(sock)) {
        return NULL;
    }

    _NativeUDPSocket* sock = (_NativeUDPSocket*)BfxMalloc(sizeof(_NativeUDPSocket));
    memset(sock, 0, sizeof(_NativeUDPSocket));

    _initSocketCommon((_NativeSocketCommon*)sock, framework, udata);

    sock->socket = sock;

    sock->recvBufferSize = 1500;
    sock->recvBuffer = BfxMalloc(sock->recvBufferSize);

    _fillUDPSocketEvents(sock);

    return sock;
}

// 最后释放之前，必须已经被正确close
static void _deleteUDPSocket(_NativeUDPSocket* sock) {

    BLOG_CHECK(_nativeSocketIsClosed(sock));
    BLOG_CHECK(!BFX_NATIVE_SOCKET_IS_VALID(sock->socket));

    _uninitSocketCommon((_NativeSocketCommon*)sock);

    BfxFree(sock);
}


static int32_t _udpSocketAddref(_NativeUDPSocket* sock) {
    assert(sock->ref > 0);
    return BfxAtomicInc32(&sock->ref);
}

static int32_t _udpSocketRelease(_NativeUDPSocket* sock) {
    assert(sock->ref > 0);
    int32_t ret = BfxAtomicDec32(&sock->ref);
    if (ret == 0) {
        _deleteUDPSocket(sock);
    }

    return ret;
}

static void _closeUDPSocket(_NativeUDPSocket* sock) {

    int state = BfxAtomicExchange32(&sock->state, _NativeSocketState_Closed);
    if (state == _NativeSocketState_Closed) {
        BLOG_ERROR("socket been closed already! sock=%p", sock);
        return;
    }

    if (sock->socket != BFX_NATIVE_SOCKET_INVALID_HANDLE) {
        _closeSocket(sock->socket);
        sock->socket = BFX_NATIVE_SOCKET_INVALID_HANDLE;
    }
}


static int _udpSocketOpen(_NativeUDPSocket* socket, const BdtEndpoint* local) {
    BLOG_CHECK(BFX_NATIVE_SOCKET_IS_VALID(socket->socket));

    bool ret = _nativeSocketChangeState(socket, _NativeSocketState_Bind, _NativeSocketState_Init);
    if (!ret) {
        BLOG_ERROR("invalid udp socket state! state=%d", socket->state);
        return BFX_RESULT_INVALID_STATE;
    }

    int bindRet = _rawSocketBind(socket, local);
    if (bindRet != 0) {
        BLOG_ERROR("udp socket bind error! ret=%d", bindRet);
        _nativeSocketChangeState(socket, _NativeSocketState_Init, _NativeSocketState_Bind);

        return bindRet;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// sendto

static void  _udpSocketSendToEvent(_NativeUDPSocket* socket, int err, void* buffer, size_t len) {

    do {
        if (_nativeSocketIsClosed(socket)) {
            BLOG_WARN("send complete event but socket has been closed! socket=%p, err=%d", socket->socket, err);
            break;
        }

        if (err) {
            BLOG_ERROR("udp socket sendto error, socket=%p, err=%d", socket->socket, err);
            socket->framework->udpEvents.error(socket->socket, err);
            break;
        }

    } while (0);

    // 释放缓冲区
    BfxFree(buffer);
}

static int _udpSocketSendTo(_NativeUDPSocket* socket, const BdtEndpoint* remote, const void* buffer, size_t len) {

    // TODO 拷贝内存
    void* sendBuffer = BfxMalloc(len);
    memcpy(sendBuffer, buffer, len);

    int sendRet = _udpSocketSendToRaw(socket, remote, sendBuffer, len);
    if (sendRet != 0) {
        BfxFree(sendBuffer);

        return sendRet;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// recvfrom


static int _requireRecvFrom(_NativeUDPSocket* socket, void** buffer, size_t* len) {

    // TODO 
    *buffer = BfxMalloc(1500);
    *len = 1500;
}

static int _releaseRecvFrom(_NativeUDPSocket* socket, void* buffer) {
    BfxFree(buffer);

    return 0;
}

static void  _udpSocketRecvFromEvent(_NativeUDPSocket* socket, int err, void* buffer, size_t len) {
    do {
        if (_nativeSocketIsClosed(socket)) {
            BLOG_WARN("recvfrom complete event but socket has been closed! socket=%p, err=%d", socket->socket, err);
            break;
        }

        if (err) {
            // TODO 出错后如何处理？
            BLOG_ERROR("udp socket recvfrom error, socket=%p, err=%d", socket->socket, err);
            socket->framework->udpEvents.error(socket->socket, err);
        } else {
            socket->framework->udpEvents.data(socket->socket, (char*)buffer, len);
            buffer = NULL;
        }

        if (buffer) {
            _releaseRecvFrom(socket, buffer);
        }

        // TODO error后是否要继续？
        _recvFromOnce(socket);
    
    } while (0);

    if (buffer) {
        BfxFree(buffer);
    }
}

static int _recvFromOnce(_NativeUDPSocket* socket) {
    void* buffer = NULL;
    size_t len = 0;
    _requireRecvFrom(socket, &buffer, &len);

    int ret = _udpSocketRecvFromRaw(socket, buffer, len);
    if (ret != 0) {
        BLOG_ERROR("udp socket recvfrom got error! err=%d", ret);
        socket->framework->udpEvents.error(socket->socket, ret);

        return ret;
    }

    // 等待事件
}

//////////////////////////////////////////////////////////////////////////
// error

static void _udpSocketErrorEvent(_NativeUDPSocket* socket, int err) {
    BLOG_ERROR("udp socket error, socket=%p, err=%d", socket->socket, err);
    socket->framework->udpEvents.error(socket->socket, err);
}

static void _fillUDPSocketEvents(_NativeUDPSocket* sock) {
    sock->events.sendTo = _udpSocketSendToEvent;
    sock->events.recvFrom = _udpSocketRecvFromEvent;
    sock->events.error = _udpSocketErrorEvent;
}