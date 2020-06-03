#include "./uv_sock.h"
#include "./uv_tcp_raw.inl"
#include "../../framework/thread_pool.h"

//////////////////////////////////////////////////////////////////////////
// create

typedef struct {
    uv_sem_t sem;
    
    _BuckyFramework* framework;
    BfxUserData userData;
    
    _UVTCPSocket* socket;
}_TcpSocketCreateSyncData;


static void _onTcpSocketCreate(void* udata) {
    _TcpSocketCreateSyncData* sync = (_TcpSocketCreateSyncData*)udata;

    sync->socket = _createTcpSocket(sync->framework, &sync->userData);
    uv_sem_post(&sync->sem);
}

static _UVTCPSocket* _createTcpSocketSync(_BuckyFramework* framework, const BfxUserData* userData) {

    // 选择一个目标线程
    BFX_THREAD_HANDLE thread = _threadContainerSelectThread(&framework->threadPool, _BuckyThreadTarget_TCP);
    assert(thread);
    if (thread == NULL) {
        return NULL;
    }

    _UVTCPSocket* socket = NULL;
    // 发起创建
    if (BfxIsCurrentThread(thread)) {
        socket = _createTcpSocket(framework, userData);
    } else {
        _TcpSocketCreateSyncData sync;
        uv_sem_init(&sync.sem, 0);
        sync.framework = framework;
        sync.userData = *userData;

        do {
            BfxUserData udata = { .userData = &sync };
            int ret = BfxPostMessage(thread, _onTcpSocketCreate, udata);
            if (ret != 0) {
                break;
            }

            uv_sem_wait(&sync.sem);
            socket = sync.socket;
        } while (0);

        uv_sem_destroy(&sync.sem);
    }

    BfxThreadRelease(thread);

    return socket;
}

//////////////////////////////////////////////////////////////////////////
// LISTEN

typedef struct {
    uv_sem_t sem;

    _UVTCPSocket* socket;
    const BdtEndpoint* endpoint;
    int ret;
}_TcpSocketListenSyncData;


static void _onTcpSocketListen(void* udata) {
    _TcpSocketListenSyncData* sync = (_TcpSocketListenSyncData*)udata;

    sync->ret = _tcpSocketListen(sync->socket, sync->endpoint);
    uv_sem_post(&sync->sem);
}

static int _tcpSocketListenSync(_UVTCPSocket* socket, const BdtEndpoint* endpoint) {
    int ret;
    if (BfxIsCurrentThread(socket->thread)) {
        ret = _tcpSocketListen(socket, endpoint);
    } else {
        _TcpSocketListenSyncData sync;
        uv_sem_init(&sync.sem, 0);
        sync.socket = socket;
        sync.endpoint = endpoint;

        do {
            BfxUserData udata = { .userData = &sync };
            ret = BfxPostMessage(socket->thread, _onTcpSocketListen, udata);
            if (ret != 0) {
                break;
            }

            uv_sem_wait(&sync.sem);
            ret = sync.ret;
        } while (0);

        uv_sem_destroy(&sync.sem);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// connect

typedef struct {
    uv_sem_t sem;

    _UVTCPSocket* socket;
    const BdtEndpoint* local;
    const BdtEndpoint* remote;
    int ret;
}_TcpSocketConnectSyncData;


static void _onTcpSocketConnect(void* udata) {
    _TcpSocketConnectSyncData* sync = (_TcpSocketConnectSyncData*)udata;

    sync->ret = _tcpSocketConnect(sync->socket, sync->local, sync->remote);
    uv_sem_post(&sync->sem);
}


static int _tcpSocketConnectSync(_UVTCPSocket* socket, const BdtEndpoint* local, const BdtEndpoint* remote) {
    int ret;
    if (BfxIsCurrentThread(socket->thread)) {
        ret = _tcpSocketConnect(socket, local, remote);
    } else {
        _TcpSocketConnectSyncData sync;
        uv_sem_init(&sync.sem, 0);
        sync.socket = socket;
        sync.local = local;
        sync.remote = remote;

        do {
            BfxUserData udata = { .userData = &sync };
            ret = BfxPostMessage(socket->thread, _onTcpSocketConnect, udata);
            if (ret != 0) {
                break;
            }

            uv_sem_wait(&sync.sem);
            ret = sync.ret;
        } while (0);

        uv_sem_destroy(&sync.sem);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// send

typedef struct {
    _UVTCPSocket* socket;
    const uint8_t* data;
    size_t len;
}_TcpSocketSendAsyncData;


static void _onTcpSocketSend(void* udata) {
    _TcpSocketSendAsyncData* async = (_TcpSocketSendAsyncData*)udata;

    int ret = _tcpSocketSend(async->socket, async->data, async->len);
    if (ret != 0) {
        // 如果调用失败了，那么这里需要释放
        BfxFree((void*)async->data);
    }

    _uvTcpSocketRelease(async->socket);
    BfxFree(async);
}

static int _tcpSocketSendAsync(_UVTCPSocket* socket, const uint8_t* buffer, size_t *len) {
    BLOG_CHECK(*len > 0);

    int ret = _requireSend(&socket->sendPool, len);
    if (*len == 0) {
        // 暂时不能发送数据了
        return ret;
    }

    // 可以发送全部或者部分数据
    uint8_t* local = (uint8_t*)BfxMalloc(*len);
    memcpy(local, buffer, *len);

    int sendRet;
    if (BfxIsCurrentThread(socket->thread)) {
        sendRet = _tcpSocketSend(socket, local, *len);
    } else {

        _TcpSocketSendAsyncData* async = (_TcpSocketSendAsyncData*)BfxMalloc(sizeof(_TcpSocketSendAsyncData));
        async->socket = socket;
        _uvTcpSocketAddref(socket);
        async->data = local;
        async->len = *len;
        
        BfxUserData udata = { .userData = async };
        sendRet = BfxPostMessage(socket->thread, _onTcpSocketSend, udata);
        if (sendRet != 0) {
            _uvTcpSocketRelease(socket);
            BfxFree(local);
            BfxFree(async);
        }
    }

    if (sendRet != 0) {
        ret = sendRet;
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////
// pause/resume/ShutDown/Close/Destroy

#define _UV_TCP_SOCKET_SYNC_METHOD(method) \
typedef struct { \
    uv_sem_t sem; \
    _UVTCPSocket* socket; \
    int ret; \
} _TcpSocket ## method ## SyncData; \
static void _onTcpSocket ## method(void* udata) { \
    _TcpSocket ## method ## SyncData* sync = (_TcpSocket ## method ## SyncData*)udata; \
    sync->ret = _tcpSocket ## method(sync->socket); \
    uv_sem_post(&sync->sem); \
} \
static int _tcpSocket ## method ## Sync(_UVTCPSocket* socket) { \
    int ret; \
    if (BfxIsCurrentThread(socket->thread)) { \
        ret = _tcpSocket ## method(socket); \
    } else { \
        _TcpSocket ## method ## SyncData sync; \
        uv_sem_init(&sync.sem, 0); \
        sync.socket = socket; \
        do { \
            BfxUserData udata = { .userData = &sync }; \
            ret = BfxPostMessage(socket->thread, _onTcpSocket ## method, udata); \
            if (ret != 0) { \
                break; \
            } \
            uv_sem_wait(&sync.sem); \
            ret = sync.ret; \
        } while (0); \
        uv_sem_destroy(&sync.sem); \
    } \
    return ret; \
}

_UV_TCP_SOCKET_SYNC_METHOD(Pause);
_UV_TCP_SOCKET_SYNC_METHOD(Resume);
// _UV_TCP_SOCKET_SYNC_METHOD(ShutDown);
_UV_TCP_SOCKET_SYNC_METHOD(Close);
_UV_TCP_SOCKET_SYNC_METHOD(Destroy);