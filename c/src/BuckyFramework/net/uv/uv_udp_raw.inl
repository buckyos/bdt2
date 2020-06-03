
#include "./uv_sock.h"


static _UVUDPSocket* _newUdpSocket(_BuckyFramework* framework, BfxUserData udata) {
    _UVUDPSocket* sock = (_UVUDPSocket*)BfxMalloc(sizeof(_UVUDPSocket));
    memset(sock, 0, sizeof(_UVUDPSocket));

    _initSocketCommon((_UVSocketCommon*)sock, framework, udata);

    sock->socket.data = sock;

    uv_loop_t* loop = BfxThreadGetCurrentLoop();
    uv_udp_init(loop, &sock->socket);

    return sock;
}

static void _deleteUDPSocket(_UVUDPSocket* sock) {

    _uninitSocketCommon((_UVSocketCommon*)sock);

    BfxFree(sock);
}

int32_t _uvUdpSocketAddref(_UVUDPSocket* sock) {
    assert(sock->ref > 0);
    return BfxAtomicInc32(&sock->ref);
}

int32_t _uvUdpSocketRelease(_UVUDPSocket* sock) {
    assert(sock->ref > 0);
    int32_t ret = BfxAtomicDec32(&sock->ref);
    if (ret == 0) {
        _deleteUDPSocket(sock);
    }

    return ret;
}

static _UVUDPSocket* _createUdpSocket(_BuckyFramework* framework, const BfxUserData* userData) {
    _UVUDPSocket* sock = _newUdpSocket(framework, *userData);

    return sock;
}

static void _recvAllocBuffer(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    buf->base = (char*)BfxMalloc(suggestedSize);
#ifdef BFX_OS_WIN
    buf->len = (ULONG)suggestedSize;
#else
    buf->len = (size_t)suggestedSize;
#endif 
}

static void _onRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    _UVUDPSocket* sock = (_UVUDPSocket*)handle->data;
    assert(sock);
    _BuckyFramework* bframe = sock->framework;

    if (nread > 0 || (nread == 0 && addr)) {
        BdtEndpoint remote;
        SockAddrToBdtEndpoint(addr, &remote, BDT_ENDPOINT_PROTOCOL_UDP);
        bframe->udpEvents.data(sock, buf->base, nread, &remote);
        ((uv_buf_t*)buf)->base = NULL;

    } else if (nread < 0) {

        // 出错
        BLOG_ERROR("udp recv error: %s", uv_err_name((int)nread));
        bframe->udpEvents.error(sock, _uvTranslateError((int)nread), uv_err_name((int)nread));
        
    } else {
        
        // 不做处理
        // =0并且addr不为null，那么说明收到了有效包，需要触发data事件；其余情况不需要
        // 0 if there is no more data to read.You may discard or repurpose the read buffer.Note that 0 may also mean that an empty datagram was received(in this case addr is not NULL).
    }

    if (buf->base) {
        BfxFree(buf->base);
    }
}

static int _udpSocketStart(_UVUDPSocket* sock) {
    int ret = uv_udp_recv_start(&sock->socket, _recvAllocBuffer, _onRecv);
    if (ret != 0 && ret != UV_EALREADY) {
        return _uvTranslateError(ret);
    }

    return 0;
}

static int _udpSocketOpen(_UVUDPSocket* sock, const BdtEndpoint* local) {
    struct sockaddr_storage dest;
    int ret = BdtEndpointToSockAddr(local, (struct sockaddr*) & dest);
    if (ret != 0) {
        return ret;
    }

    ret = uv_udp_bind(&sock->socket, (struct sockaddr*) & dest, 0);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    // 打开后需要立即开始接收数据包
    return _udpSocketStart(sock);
}

//////////////////////////////////////////////////////////////////////////
// sendto

typedef struct {
    uv_udp_send_t req;

    _UVUDPSocket* owner;

    const uint8_t* buffer;
    size_t size;
}_UVSendToOp;

static _UVSendToOp* _newSendToOp(_UVUDPSocket* owner, const uint8_t* buffer, size_t size) {
    _UVSendToOp* op = BfxMalloc(sizeof(_UVSendToOp));

    op->owner = owner;
    op->buffer = buffer;
    op->size = size;
    //op->buffer = BfxMalloc(size);
    //memcpy(op->buffer, buffer, size);

    return op;
}

static void _deleteSendToOP(_UVSendToOp* op) {
    if (op->buffer) {
        BfxFree((void*)op->buffer);
        op->buffer = NULL;
    }

    BfxFree(op);
}

static void _onSendToComplete(uv_udp_send_t* req, int status) {
    _UVSendToOp* op = (_UVSendToOp*)req;

    _deleteSendToOP(op);
}

// 内部会托管buffer
static int _udpSocketSendto(_UVUDPSocket* sock, const BdtEndpoint* remote, const uint8_t* buffer, size_t size) {

    struct sockaddr_storage dest;
    int ret = BdtEndpointToSockAddr(remote, (struct sockaddr*)&dest);
    if (ret != 0) {
        return ret;
    }

    _UVSendToOp* op = _newSendToOp(sock, buffer, size);
    uv_buf_t wrbuf = uv_buf_init((void*)op->buffer, (unsigned int)op->size);

    ret = uv_udp_send(&op->req, &sock->socket, &wrbuf, 1, (struct sockaddr*)&dest, _onSendToComplete);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    return 0;
}

static void _uvUdpSocketCloseCallback(uv_handle_t* handle) {
    _UVUDPSocket* sock = (_UVUDPSocket*)handle->data;
    handle->data = NULL;

    _uvUdpSocketRelease(sock);

    // TODO change status
}

static int _udpSocketClose(_UVUDPSocket* socket) {
    _uvUdpSocketAddref(socket);
    uv_close((uv_handle_t*)& socket->socket, _uvUdpSocketCloseCallback);

    return 0;
}

static int _udpSocketDestroy(_UVUDPSocket* sock) {
    _uvUdpSocketRelease(sock);

    return 0;
}