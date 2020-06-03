#include "./uv_sock.h"
#include "./uv_udp_raw.inl"
#include "../../framework/thread_pool.h"


//////////////////////////////////////////////////////////////////////////
// create

typedef struct {
    uv_sem_t sem;

    _BuckyFramework* framework;
    BfxUserData userData;

    _UVUDPSocket* socket;
}_UdpSocketCreateSyncData;


static void _onUdpSocketCreate(void* udata) {
    _UdpSocketCreateSyncData* sync = (_UdpSocketCreateSyncData*)udata;

    sync->socket = _createUdpSocket(sync->framework, &sync->userData);
    uv_sem_post(&sync->sem);
}

static _UVUDPSocket* _createUdpSocketSync(_BuckyFramework* framework, const BfxUserData* userData) {

    // 选择一个目标线程
    BFX_THREAD_HANDLE thread = _threadContainerSelectThread(&framework->threadPool, _BuckyThreadTarget_UDP);
    assert(thread);
    if (thread == NULL) {
        return NULL;
    }

    _UVUDPSocket* socket = NULL;

    // 发起创建
    if (BfxIsCurrentThread(thread)) {
        socket = _createUdpSocket(framework, userData);
    } else {
        _UdpSocketCreateSyncData sync;
        uv_sem_init(&sync.sem, 0);
        sync.framework = framework;
        sync.userData = *userData;

        do {
            BfxUserData udata = { .userData = &sync };
            int ret = BfxPostMessage(thread, _onUdpSocketCreate, udata);
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
// open

typedef struct {
    uv_sem_t sem;

    _UVUDPSocket* socket;
    const BdtEndpoint* local;
    int ret;
}_UdpSocketOpenSyncData;


static void _onUdpSocketOpen(void* udata) {
    _UdpSocketOpenSyncData* sync = (_UdpSocketOpenSyncData*)udata;

    sync->ret = _udpSocketOpen(sync->socket, sync->local);
    uv_sem_post(&sync->sem);
}

static int _udpSocketOpenSync(_UVUDPSocket* socket, const BdtEndpoint* local) {
    int ret;
    if (BfxIsCurrentThread(socket->thread)) {
        ret = _udpSocketOpen(socket, local);
    } else {
        _UdpSocketOpenSyncData sync;
        uv_sem_init(&sync.sem, 0);
        sync.socket = socket;
        sync.local = local;

        do {
            BfxUserData udata = { .userData = &sync };
            ret = BfxPostMessage(socket->thread, _onUdpSocketOpen, udata);
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
    _UVUDPSocket* socket;
    BdtEndpoint remote;
    const uint8_t* data;
    size_t len;
}_UdpSocketSendToAsyncData;


static void _onUdpSocketSend(void* udata) {
    _UdpSocketSendToAsyncData* async = (_UdpSocketSendToAsyncData*)udata;

    int ret = _udpSocketSendto(async->socket, &async->remote, async->data, async->len);
    if (ret != 0) {
        BfxFree((void*)async->data);
    }

    _uvUdpSocketRelease(async->socket);
    BfxFree(async);
}

static int _udpSocketSendToAsync(_UVUDPSocket* socket, const BdtEndpoint* remote, const uint8_t* buffer, size_t len) {

    uint8_t* local = (uint8_t*)BfxMalloc(len);
    memcpy(local, buffer, len);

    int ret;
    if (BfxIsCurrentThread(socket->thread)) {
        ret = _udpSocketSendto(socket, remote, local, len);
    } else {
  
        _UdpSocketSendToAsyncData* async = (_UdpSocketSendToAsyncData*)BfxMalloc(sizeof(_UdpSocketSendToAsyncData));
        BdtEndpointCopy(&async->remote, remote);
        async->data = local;
        async->len = len;
        async->socket = socket;
        _uvUdpSocketAddref(socket);

        BfxUserData udata = { .userData = async };
        ret = BfxPostMessage(socket->thread, _onUdpSocketSend, udata);
        if (ret != 0) {
            _uvUdpSocketRelease(async->socket);
            BfxFree(local);
            BfxFree(async);
        }
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// close/destroy

#define _UV_UDP_SOCKET_SYNC_METHOD(method) \
typedef struct { \
    uv_sem_t sem; \
    _UVUDPSocket* socket; \
    int ret; \
} _UdpSocket ## method ## SyncData; \
static void _onUdpSocket ## method(void* udata) { \
    _UdpSocket ## method ## SyncData* sync = (_UdpSocket ## method ## SyncData*)udata; \
    sync->ret = _udpSocket ## method(sync->socket); \
    uv_sem_post(&sync->sem); \
} \
static int _udpSocket ## method ## Sync(_UVUDPSocket* socket) { \
    int ret; \
    if (BfxIsCurrentThread(socket->thread)) { \
        ret = _udpSocket ## method(socket); \
    } else { \
        _UdpSocket ## method ## SyncData sync; \
        uv_sem_init(&sync.sem, 0); \
        sync.socket = socket; \
        do { \
            BfxUserData udata = { .userData = &sync }; \
            ret = BfxPostMessage(socket->thread, _onUdpSocket ## method, udata); \
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

_UV_UDP_SOCKET_SYNC_METHOD(Close);
_UV_UDP_SOCKET_SYNC_METHOD(Destroy);