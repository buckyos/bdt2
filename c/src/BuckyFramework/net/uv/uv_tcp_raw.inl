#pragma once
#include "./uv_sock.h"
#include "../common/send_pool.inl"


static _UVTCPSocket* _newTCPSocket(_BuckyFramework* framework, BfxUserData udata) {
    _UVTCPSocket* sock = (_UVTCPSocket*)BfxMalloc(sizeof(_UVTCPSocket));
    memset(sock, 0, sizeof(_UVTCPSocket));

    _initSocketCommon((_UVSocketCommon*)sock, framework, udata);

    sock->socket.data = sock;
    sock->backlog = 128;

    _initSendPool(&sock->sendPool, 1024 * 1024 * 8);

    uv_loop_t* loop = BfxThreadGetCurrentLoop();
    uv_tcp_init(loop, &sock->socket);

    return sock;
}

static void _deleteTCPSocket(_UVTCPSocket* sock) {
    _uninitSendPool(&sock->sendPool);

    _uninitSocketCommon((_UVSocketCommon*)sock);

    BfxFree(sock);
}

static int32_t _uvTcpSocketAddref(_UVTCPSocket* sock) {
    assert(sock->ref > 0);
    return BfxAtomicInc32(&sock->ref);
}

static int32_t _uvTcpSocketRelease(_UVTCPSocket* sock) {
    assert(sock->ref > 0);
    int32_t ret = BfxAtomicDec32(&sock->ref);
    if (ret == 0) {
        _deleteTCPSocket(sock);
    }

    return ret;
}

static _UVTCPSocket* _createTcpSocket(_BuckyFramework* framework, const BfxUserData* userData) {


    _UVTCPSocket* sock = _newTCPSocket(framework, *userData);

    return sock;
}

/*

static void _uvShutDownCallback(uv_shutdown_t* req, int status) {
    if (status != 0) {
        // TODO
    }

    BfxFree(req);
}

static int _tcpSocketShutDown(_UVTCPSocket* socket) {

    uv_shutdown_t* req = (uv_shutdown_t*)BfxMalloc(sizeof(uv_shutdown_t));
    
    int ret = uv_shutdown(req, (uv_stream_t*)&socket->socket, _uvShutDownCallback);
    if (ret != 0) {
        return -1;
    }

    return 0;
}
*/

static void _uvTcpSocketCloseCallback(uv_handle_t* handle) {
    _UVTCPSocket* sock = (_UVTCPSocket*)handle->data;
    handle->data = NULL;
    _uvTcpSocketRelease(sock);

    // TODO change status
}

static int _tcpSocketClose(_UVTCPSocket* socket) {
    _uvTcpSocketAddref(socket);
    uv_close((uv_handle_t*)&socket->socket, _uvTcpSocketCloseCallback);

    return 0;
}

static int _tcpSocketDestroy(_UVTCPSocket* socket) {
    
    // 这里只需要释放一次引用计数
    _uvTcpSocketRelease(socket);

    return 0;
}

static void _onNewConnection(uv_stream_t* server, int status) {
    if (status < 0) {
        return;
    }

    _UVTCPSocket* serverSocket = (_UVTCPSocket*)server->data;
    assert(serverSocket);

    BfxUserData udata = { 0 };
    _UVTCPSocket* sock = _newTCPSocket(serverSocket->framework, udata);

    int acceptRet = uv_accept(server, (uv_stream_t*)&sock->socket);
    if (acceptRet != 0) {
        _uvTcpSocketRelease(sock);
        return;
    }

    // 保存地址到socket
    {
        struct sockaddr_storage addr = { 0 };
        int addrLen = sizeof(addr);
        int ret = uv_tcp_getsockname(&sock->socket, (struct sockaddr*) & addr, &addrLen);
        if (ret != 0) {
            _uvTcpSocketRelease(sock);
            return;
        }

        SockAddrToBdtEndpoint((struct sockaddr*) & addr, &sock->local, BDT_ENDPOINT_PROTOCOL_TCP);
    }

    {
        struct sockaddr_storage addr = { 0 };
        int addrLen = sizeof(addr);
        int ret = uv_tcp_getpeername(&sock->socket, (struct sockaddr*) & addr, &addrLen);
        if (ret != 0) {
            _uvTcpSocketRelease(sock);
            return;
        }

        SockAddrToBdtEndpoint((struct sockaddr*) & addr, &sock->remote, BDT_ENDPOINT_PROTOCOL_TCP);
    }

    // 触发回调
    _BuckyFramework* bframe = serverSocket->framework;
    bframe->tcpEvents.connection(serverSocket, sock);
}

static int _tcpSocketListen(_UVTCPSocket* socket, const BdtEndpoint* endpoint) {

    _UVTCPSocket* sock = (_UVTCPSocket*)socket;

    struct sockaddr_storage addr;
    int ret = BdtEndpointToSockAddr(endpoint, (struct sockaddr*)&addr);
    if (ret != 0) {
        return ret;
    }

    int bindRet = uv_tcp_bind(&sock->socket, (const struct sockaddr*)&addr, 0);
    if (bindRet != 0) {
        return _uvTranslateError(ret);
    }

    ret = uv_listen((uv_stream_t*)&sock->socket, sock->backlog, _onNewConnection);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    return 0;
}

static void _uvOnConnect(uv_connect_t* req, int status) {
    _UVTCPSocket* socket = (_UVTCPSocket*)req->handle->data;
    assert(socket);

    BfxFree(req);

    // 触发回调
    _BuckyFramework* bframe = socket->framework;
    if (status == 0) {
        bframe->tcpEvents.established(socket);
    } else {
        bframe->tcpEvents.error(socket, _uvTranslateError(status), NULL);
    }
}

static int _tcpSocketBind(_UVTCPSocket* socket, const BdtEndpoint* local) {

    struct sockaddr_storage dest;
    int ret = BdtEndpointToSockAddr(local, (struct sockaddr*)&dest);
    if (ret != 0) {
        return ret;
    }

   ret = uv_tcp_bind(&socket->socket, (struct sockaddr*)&dest, 0);
   if (ret != 0) {
       return _uvTranslateError(ret);
   }

   return 0;
}

static int _tcpSocketConnect(_UVTCPSocket* socket, const BdtEndpoint* local, const BdtEndpoint* remote) {

    if (local) {
        int bindRet = _tcpSocketBind(socket, local);
        if (bindRet) {
            return bindRet;
        }
    }

    struct sockaddr_storage dest;
    int ret = BdtEndpointToSockAddr(remote, (struct sockaddr*)&dest);
    if (ret != 0) {
        return ret;
    }

    uv_connect_t* connect = (uv_connect_t*)BfxMalloc(sizeof(uv_connect_t));
    ret = uv_tcp_connect(connect, &socket->socket, (const struct sockaddr*)&dest, _uvOnConnect);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 发送相关逻辑

typedef struct {
    uv_write_t req;

    _UVTCPSocket* owner;

    const uint8_t* buffer;
    size_t size;
}_UVWriteOp;

static _UVWriteOp* _newWriteOp(_UVTCPSocket* owner, const uint8_t* buffer, size_t size) {
    _UVWriteOp* op = BfxMalloc(sizeof(_UVWriteOp));

    op->owner = owner;
    op->buffer = buffer;
    op->size = size;
    //op->buffer = BfxMalloc(size);
    //memcpy(op->buffer, buffer, size);

    return op;
}

static void _deleteWriteOP(_UVWriteOp* op) {
    if (op->buffer) {
        BfxFree((void*)op->buffer);
        op->buffer = NULL;
    }

    BfxFree(op);
}


static void _onSendComplete(uv_write_t* req, int status) {
    if (status != 0) {
        int err = _uvTranslateError(status);
        BLOG_ERROR("tcp send failed! err=%d", err);
    }

    _UVWriteOp* op = (_UVWriteOp*)req;

    int ret = _releaseSend(&op->owner->sendPool, op->size);
    if (ret == -1) {
        op->owner->framework->tcpEvents.drain(op->owner);
    }

    _deleteWriteOP(op);
}

// 调用成功后，内部会托管buffer
static int _tcpSocketSend(_UVTCPSocket* socket, const uint8_t* buffer, size_t len) {

    _UVWriteOp* op = _newWriteOp(socket, buffer, len);
    uv_buf_t wrbuf = uv_buf_init((void*)op->buffer, (unsigned int)op->size);

    int ret = uv_write(&op->req, (uv_stream_t*)&socket->socket, &wrbuf, 1, _onSendComplete);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 接收相关逻辑

static int _tcpSocketPause(_UVTCPSocket* socket) {

    int ret = uv_read_stop((uv_stream_t*)&socket->socket);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

static void _readAllocBuffer(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    buf->base = (char*)BfxMalloc(suggestedSize);
#ifdef BFX_OS_WIN
    buf->len = (ULONG)suggestedSize;
#else
    buf->len = (size_t)suggestedSize;
#endif 
}

static void _onRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    
    _UVTCPSocket* sock = (_UVTCPSocket*)client->data;
    assert(sock);
    _BuckyFramework* bframe = sock->framework;

    if (nread > 0) {
        bframe->tcpEvents.data(sock, buf->base, nread);
    }
    else if (nread < 0) {
        if (nread == UV_EOF) {
            // 对端关闭
            bframe->tcpEvents.data(sock, NULL, 0);
        } else {
            // 出错
            BLOG_ERROR("read error: %s", uv_err_name((int)nread));
            bframe->tcpEvents.error(sock, _uvTranslateError((int)nread), uv_err_name((int)nread));
        }
    } else {
        // 不做处理
        // nread might be 0, which does not indicate an error or EOF. This is equivalent to EAGAIN or EWOULDBLOCK under read(2).
    }

    if (buf->base) {
        BfxFree(buf->base);
    }
}

static int _tcpSocketResume(_UVTCPSocket* socket) {
    int ret = uv_read_start((uv_stream_t*)&socket->socket, _readAllocBuffer, _onRead);
    if (ret != 0) {
        return _uvTranslateError(ret);
    }

    return ret;
}