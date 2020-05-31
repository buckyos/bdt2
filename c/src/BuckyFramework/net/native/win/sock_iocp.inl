
// win平台的全局API
_WinSocketAPI _winSocketAPI;

static SOCKET _createRawTcpSocket() {

    SOCKET sock = WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (!BFX_NATIVE_SOCKET_IS_VALID(sock)) {
        BLOG_ERROR("call WSASocket create tcp socket got error!");
    }

    return sock;
}

static SOCKET _createRawUdpSocket() {

    SOCKET sock = WSASocket(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (!BFX_NATIVE_SOCKET_IS_VALID(sock)) {
        BLOG_ERROR("call WSASocket create udp socket got error!");
    }

    return sock;
}

static int _closeRawSocket(BuckyNativeSocketType sock) {
    int err = closesocket(sock);
    if (err) {
        err = _nativeTranslateErr(err);
    }

    return err;
}

static int _rawSocketBind(BuckyNativeSocketType sock, const BdtEndpoint* endpoint) {
    BLOG_CHECK(BFX_NATIVE_SOCKET_IS_VALID(sock));

    struct sockaddr_storage addr;
    int ret = BdtEndpointToSockAddr(endpoint, (struct sockaddr*) & addr);
    if (ret != 0) {
        return ret;
    }

    int addrLen = 0;
    if (((const struct sockaddr*) & addr)->sa_family == AF_INET) {
        addrLen = sizeof(struct sockaddr_in);
    } else if (((const struct sockaddr*) & addr)->sa_family == AF_INET6) {
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        assert(false);
        return BFX_RESULT_INVALID_PARAM;
    }
      
    int ret = bind(sock, (const struct sockaddr*) & addr, addrLen);
    if (ret != 0) {
        return _nativeTranslateErr(ret);
    }

    return 0;
}

static int _tcpSocketListenRaw(_NativeTCPSocket* socket) {
    BLOG_CHECK(BFX_NATIVE_SOCKET_IS_VALID(socket->socket));

    int ret = listen(socket->socket, socket->backlog);
    int err = _nativeTranslateErr(ret);
    if (err != 0) {
        return err;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// connect

static void _onConnectComplete(struct _BuckyMessagePumpIORequest* request) {
    _NativeTCPSocket* socket = (_NativeTCPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        int ret = setsockopt(socket->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
        if (ret == 0) {
            
            if (_nativeSocketChangeState(socket, _NativeSocketState_Connecting, _NativeSocketState_Connected)) {
                socket->events.connect(socket, 0);
            } else {
                // 状态错误
                socket->events.connect(socket, BFX_RESULT_INVALID_STATE);
            }
        } else {
            // 在异步请求过程中socket被关闭了
            DWORD err = GetLastError();
            if (err == WSAENOTCONN || err == WSAENOTSOCK) {
                BLOG_INFO("call setsockopt err, maybe socket had been closed! socket=%d", socket->socket);
                result = BFX_RESULT_CLOSED;
            } else {
                BLOG_ERROR("call setsockopt on socket failed! socket=%d", socket->socket);
                result = BFX_RESULT_FAILED;`                            
            }

            socket->events.connect(socket, result);
        }
    } else {
        int err = _winSocketStatusTranslateToError((NTSTATUS)request->overlapped.Internal);
        socket->events.connect(socket, err);
    }

    BfxFree(request);
}

static int _tcpSocketConnectRaw(_NativeTCPSocket* socket, const BdtEndpoint* remote) {
    BLOG_CHECK(BFX_NATIVE_SOCKET_IS_VALID(socket->socket));

    _BuckyFramework* framework = sock->framework;

    // 计算地址
    struct sockaddr_storage storage;
    int ret = BdtEndpointToSockAddr(endpoint, (struct sockaddr*)&storage);
    if (ret != 0) {
        return ret;
    }

    int addrLen = 0;
    if (((const struct sockaddr*) & addr)->sa_family == AF_INET) {
        addrLen = sizeof(struct sockaddr_in);
    } else if (((const struct sockaddr*) & addr)->sa_family == AF_INET6) {
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        assert(false);
        return BFX_RESULT_INVALID_PARAM;
    }

    _BuckyMessagePumpIORequest* req = (_BuckyMessagePumpIORequest*)BfxMalloc(sizeof(_BuckyMessagePumpIORequest));
    memset(req, 0, sizeof(_BuckyMessagePumpIORequest));

    req->onIOComplete = _onConnectComplete;

    const struct sockaddr* addr = (const struct sockaddr*)&storage;
    BOOL connectResult;
    if (addr->sa_family == AF_INET) {
        connectResult = framework->api.connectEx(socket->socket, addr, addrLen, NULL, 0, NULL, &req->overlapped);
    } else {
        connectResult = framework->api.connectEx6(socket->socket, addr, addrLen, NULL, 0, NULL, &req->overlapped));
    }

    int err = _winSocketDealWithBOOLResult(connectResult);

    // 出错的情况下，交由用户处理，这里不触发error事件
    return err;
}

//////////////////////////////////////////////////////////////////////////
// accept op

#define _WIN_SOCKET_ACCEPT_ADDR_LEN sizeof(sockaddr_storage) + 16

typedef struct {
    _BuckyMessagePumpIORequest io;

    char data[_WIN_SOCKET_ACCEPT_ADDR_LEN * 2];
    DWORD bytesRecved;

    _NativeTCPSocket* comingSocket;
} _WinSocketAcceptRequest;

static _WinSocketAcceptRequest* _newAcceptRequest() {

    // 创建一个socket用以接收到来的连接
    _NativeTCPSocket* comingSocket = _createTcpSocket();
    _nativeSocketChangeState(comingSocket, _NativeSocketState_Init, _NativeSocketState_Connecting);

    _WinSocketAcceptRequest* req = BfxMalloc(sizeof(_WinSocketAcceptRequest)));
    memset(req, 0, sizeof(_WinSocketAcceptRequest));

    req->comingSocket = comingSocket;
    req->io.onIOComplete = _onAcceptComplete;
}

static void _deleteAcceptRequest(_WinSocketAcceptRequest* request) {
    if (request->comingSocket) {
        _nativeTcpSocketRelease(request->comingSocket);
        request->comingSocket = NULL;
    }

    BfxFree(request);
}

static void _onAcceptComplete(struct _BuckyMessagePumpIORequest* request) {
    _WinSocketAcceptRequest* acceptRequest = (_WinSocketAcceptRequest*)request;
    _NativeTCPSocket* socket = (_NativeTCPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        int ret = setsockopt(acceptRequest->comingSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT
            , (char*)(&socket->socket), sizeof(socket->socket));
        BLOG_CHECK(ret == 0);

        struct sockaddr* local = NULL;
        struct sockaddr* remote = NULL;
        int localLen = 0;
        int remoteLen = 0;

        const int family = BdtEndpointGetFamily(&socket->addr);
        if (family == AF_INET) {
            _winSocketAPI.getAcceptExSockaddrs(acceptRequest->data, acceptRequest->bytesRecved,
                _WIN_SOCKET_ACCEPT_ADDR_LEN, _WIN_SOCKET_ACCEPT_ADDR_LEN
                , &local, &localLen
                , &remote, &remoteLen);
        } else {
            BLOG_CHECK(family == AF_INET6);
            _winSocketAPI.getAcceptExSockaddrs6(acceptRequest->data, acceptRequest->bytesRecved,
                _WIN_SOCKET_ACCEPT_ADDR_LEN, _WIN_SOCKET_ACCEPT_ADDR_LEN
                , &local, &localLen
                , &remote, &remoteLen);
        }

        _NativeTCPSocket* comingSocket = acceptRequest->comingSocket;
        acceptRequest->comingSocket = NULL;

        SocketAddrToBdtEndpoint(local, &acceptRequest->comingSocket->local);
        SocketAddrToBdtEndpoint(remote, &acceptRequest->comingSocket->remote);

        bool changeRet =_nativeSocketChangeState(comingSocket, _NativeSocketState_Connected, _NativeSocketState_Connecting);
        BLOG_CHECK(changeRet);

        // 触发事件
        socket->events.connection(socket, 0, comingSocket);
    } else {
        int err = _winSocketStatusTranslateToError((NTSTATUS)request->overlapped.Internal);
        socket->events.connection(socket, err, NULL);
    }

    _deleteAcceptRequest(request);
}

static int _tcpSocketAcceptRaw(_NativeTCPSocket* socket) {
    _BuckyFramework* framework = sock->framework;

    _WinSocketAcceptRequest* req = _newAcceptRequest();
    assert(req->comingSocket);

    BOOL acceptRet;
    const int family = BdtEndpointGetFamily(&socket->addr);
    if (family == AF_INET) {
        acceptRet = _winSocketAPI.acceptEx(socket->socket, comingSocket->socket,
            req->data, 0, _WIN_SOCKET_ACCEPT_ADDR_LEN, _WIN_SOCKET_ACCEPT_ADDR_LEN, &req->bytesRecved, &req->io.overlapped);
    } else {
        BLOG_CHECK(family == AF_INET6);
        
        acceptRet = _winSocketAPI.acceptEx6(socket->socket, comingSocket->socket
            , req->data, 0, _WIN_SOCKET_ACCEPT_ADDR_LEN, _WIN_SOCKET_ACCEPT_ADDR_LEN, &req->bytesRecved, &req->io.overlapped);
    }

    int err = _winSocketDealWithBOOLResult(acceptRet);
    if (err != 0) {
        _deleteAcceptRequest(req);
    }

    return err;
}

//////////////////////////////////////////////////////////////////////////
// send op

typedef struct {
    _BuckyMessagePumpIORequest io;

    const uint8_t* buffer;
    size_t len;

    _NativeTCPSocket* socket;
} _WinSocketSendRequest;

static void _deleteSendRequest(_WinSocketSendRequest* req);

static void _onSendComplete(struct _BuckyMessagePumpIORequest* request) {
    _WinSocketSendRequest* sendRequest = (_WinSocketSendRequest*)request;
    _NativeTCPSocket* socket = (_NativeTCPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        size_t sendLen = (size_t)request->overlapped.InternalHigh;
        socket->events.send(sendRequest->socket, 0, sendRequest->buffer, sendRequest->len)
    } else {
        int err = _winSocketStatusTranslateToError(request->overlapped.Internal);
        socket->events.send(sendRequest->socket, err, sendRequest->buffer, sendRequest->len);
    }

    _deleteSendRequest(sendRequest);
}

// req会托管buffer
static _WinSocketSendRequest* _newSendRequest(const uint8_t* buffer, size_t len) {
    _WinSocketSendRequest* req = BfxMalloc(sizeof(_WinSocketSendRequest)));
    memset(req, 0, sizeof(_WinSocketSendRequest));
    req->io.onIOComplete = _onSendComplete;

    req->buffer = buffer;
    req->len = len;

    return req;
}

static void _deleteSendRequest(_WinSocketSendRequest* req) {
    //if (req->buffer) {
    //    BfxFree((void*)req->buffer);
    //    req->buffer = NULL;
    //}

    req->buffer = NULL;
    req->len = 0;

    BfxFree(req);
}

static int _tcpSocketSendRaw(_NativeTCPSocket* socket, const uint8_t* buffer, size_t len) {
    BLOG_CHECK(socket->state == _NativeSocketState_Connected);

    _WinSocketSendRequest* req = _newSendRequest(buffer, len);

    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)req->buffer;
    wsaBuf.len = (ULONG)req->len;

    int ret = WSASend(socket->socket, &wsaBuf, 1, NULL, 0, &(req->io.overlapped), NULL);
    ret = _nativeTranslateErr(ret);
    if (ret != 0) {
        _deleteSendRequest(req);
    } else {
        // 投递完成，等待完成时的异步通知
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// recv op

typedef struct {
    _BuckyMessagePumpIORequest io;

    void* buffer;
    size_t len;

    _NativeTCPSocket* socket;
} _WinSocketRecvRequest;

static void _deleteRecvRequest(_WinSocketRecvRequest* req);

static void _onRecvComplete(struct _BuckyMessagePumpIORequest* request) {
    _WinSocketRecvRequest* recvRequest = (_WinSocketSendRequest*)request;
    _NativeTCPSocket* socket = (_NativeTCPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        size_t recvLen = (size_t)request->overlapped.InternalHigh;
        socket->events.recv(recvRequest->socket, 0, recvRequest->buffer, recvRequest->len);
    } else {
        int err = _winSocketStatusTranslateToError(request->overlapped.Internal);
        socket->events.recv(recvRequest->socket, err, recvRequest->buffer, recvRequest->len);
    }

    _deleteRecvRequest(recvRequest);
}

static _WinSocketSendRequest* _newRecvRequest(const void* buffer, size_t len) {
    _WinSocketRecvRequest* req = BfxMalloc(sizeof(_WinSocketRecvRequest)));
    memset(req, 0, sizeof(_WinSocketRecvRequest));
    req->io.onIOComplete = _onRecvComplete;

    req->buffer = buffer;
    req->len = len;

    return req;
}

static void _deleteRecvRequest(_WinSocketRecvRequest* req) {
    req->buffer = NULL;
    req->len = 0;

    BfxFree(req);
}

static int _tcpSocketRecvRaw(_NativeTCPSocket* socket, void* buffer, size_t len) {
    BLOG_CHECK(socket->state == _NativeSocketState_Connected);

    _WinSocketRecvRequest* req = _newRecvRequest(buffer, len);

    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)req->buffer;
    wsaBuf.len = (ULONG)req->len;

    DWORD flags = 0;
    int err = WSARecv(socket->socket, &wsaBuf, 1, NULL, &flags, &(req->io.overlapped), NULL);
    err = _nativeTranslateErr(err);
    if (ret != 0) {
        req->events.onRecv(socket, err);
        _deleteRecvRequest(req);
    } else {
        // 投递完成，等待完成时的异步通知
    }

    return err;
}


//////////////////////////////////////////////////////////////////////////
// sendto op

typedef struct {
    _BuckyMessagePumpIORequest io;

    const uint8_t* buffer;
    size_t len;

    _NativeUDPSocket* socket;
} _WinSocketSendToRequest;

static void _deleteSendToRequest(_WinSocketSendToRequest* req);

static void _onSendToComplete(struct _BuckyMessagePumpIORequest* request) {
    _WinSocketSendToRequest* sendRequest = (_WinSocketSendToRequest*)request;
    _NativeUDPSocket* socket = (_NativeUDPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        size_t sendLen = (size_t)request->overlapped.InternalHigh;
        socket->events.sendTo(sendRequest->socket, 0, sendRequest->buffer, sendRequest->len)
    } else {
        int err = _winSocketStatusTranslateToError(request->overlapped.Internal);
        socket->events.sendTo(sendRequest->socket, err, sendRequest->buffer, sendRequest->len);
    }

    _deleteSendToRequest(sendRequest);
}

// req会托管buffer
static _WinSocketSendToRequest* _newSendToRequest(const uint8_t* buffer, size_t len) {
    _WinSocketSendToRequest* req = BfxMalloc(sizeof(_WinSocketSendToRequest)));
    memset(req, 0, sizeof(_WinSocketSendToRequest));
    req->io.onIOComplete = _onSendComplete;

    req->buffer = buffer;
    req->len = len;

    return req;
}

static void _deleteSendToRequest(_WinSocketSendToRequest* req) {
    //if (req->buffer) {
    //    BfxFree((void*)req->buffer);
    //    req->buffer = NULL;
    //}

    req->buffer = NULL;
    req->len = 0;

    BfxFree(req);
}

static int _udpSocketSendToRaw(_NativeTCPSocket* socket, const BdtEndpoint* remote, const uint8_t* buffer, size_t len) {
    BLOG_CHECK(socket->state == _NativeSocketState_Bind);

    struct sockaddr_storage addr;
    int ret = BdtEndpointToSockAddr(remote, (struct sockaddr*) & addr);
    if (ret != 0) {
        return ret;
    }

    _WinSocketSendToRequest* req = _newSendToRequest(buffer, BdtEndpointGetSockaddrSize(remote));

    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)req->buffer;
    wsaBuf.len = (ULONG)req->len;

    int ret = WSASendTo(socket->socket, &wsaBuf, 1, NULL, 0,
        (const struct sockaddr*)&addr,
        &(req->io.overlapped), NULL);
    ret = _nativeTranslateErr(ret);
    if (ret != 0) {
        _deleteSendToRequest(req);
    } else {
        // 投递完成，等待完成时的异步通知
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////
// recvfrom op

typedef struct {
    _BuckyMessagePumpIORequest io;

    void* buffer;
    size_t len;

    sockaddr_storage remote;
    int remoteLen;

    _NativeUDPSocket* socket;
} _WinSocketRecvFromRequest;

static void _deleteRecvFromRequest(_WinSocketRecvFromRequest* req);

static void _onRecvFromComplete(struct _BuckyMessagePumpIORequest* request) {
    _WinSocketRecvFromRequest* recvRequest = (_WinSocketRecvFromRequest*)request;
    _NativeUDPSocket* socket = (_NativeUDPSocket*)request->data;

    if (NT_SUCCESS(request->overlapped.Internal)) {
        size_t recvLen = (size_t)request->overlapped.InternalHigh;

        const BdtEndpoint ep;
        SockAddrToBdtEndpoint((struct sockaddr*) & recvRequest->remote, &ep, BDT_ENDPOINT_PROTOCOL_UDP);

        socket->events.recvFrom(recvRequest->socket, 0, recvRequest->buffer, recvRequest->len, &ep);
    } else {
        int err = _winSocketStatusTranslateToError(request->overlapped.Internal);
        socket->events.recvFrom(recvRequest->socket, err, recvRequest->buffer, recvRequest->len, NULL);
    }

    _deleteRecvFromRequest(recvRequest);
}

static _WinSocketRecvFromRequest* _newRecvFromRequest(const void* buffer, size_t len) {
    _WinSocketRecvFromRequest* req = BfxMalloc(sizeof(_WinSocketRecvFromRequest)));
    memset(req, 0, sizeof(_WinSocketRecvRequest));
    req->io.onIOComplete = _onRecvFromComplete;

    req->buffer = buffer;
    req->len = len;

    return req;
}

static void _deleteRecvFromRequest(_WinSocketRecvFromRequest* req) {
    req->buffer = NULL;
    req->len = 0;

    BfxFree(req);
}

static int _udpSocketRecvFromRaw(_NativeTCPSocket* socket, void* buffer, size_t len) {
    BLOG_CHECK(socket->state == _NativeSocketState_Bind);

    _WinSocketRecvFromRequest* req = _newRecvFromRequest(buffer, len);

    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)req->buffer;
    wsaBuf.len = (ULONG)req->len;

    DWORD flags = 0;
    int err = WSARecvFrom(socket->socket, &wsaBuf, 1, NULL, &flags,
        (struct sockaddr*)&req->remote, &req->remoteLen,
        &(req->io.overlapped), NULL);
    err = _nativeTranslateErr(err);
    if (ret != 0) {
        _deleteRecvFromRequest(req);
    } else {
        // 投递完成，等待完成时的异步通知
    }

    return err;
}
