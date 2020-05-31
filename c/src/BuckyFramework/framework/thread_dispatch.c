#include <BuckyBase.h>
#include "../internal.h"
#include "./thread_dispatch.h"
#include "../net/uv/uv_sock.h"

//////////////////////////////////////////////////////////////////////////
// dispatch packages

// TODO 对recvPackage的统一优化
typedef struct {
    BfxListItem stub;

    _UVUDPSocket* socket;
    const void* data;
    size_t bytes;
    BdtEndpoint remote;
}_UDPDispatchPackage;

static _UDPDispatchPackage* _newUDPDispatchPackage(_UVUDPSocket* socket, const char* data, size_t bytes, const BdtEndpoint* remote) {

    _UDPDispatchPackage* pkg = (_UDPDispatchPackage*)BfxMalloc(sizeof(_UDPDispatchPackage));
    pkg->socket = socket;
    _uvUdpSocketAddref(socket);

    memcpy(&pkg->remote, remote, sizeof(BdtEndpoint));
    pkg->bytes = bytes;
    pkg->data = data;

    return pkg;
}

static void _deleteUDPDispatchPackage(_UDPDispatchPackage* pkg) {

    _uvUdpSocketRelease(pkg->socket);

    BfxFree((void*)pkg->data);

    BfxFree(pkg);
}

//////////////////////////////////////////////////////////////////////////
// dispatch threads

typedef struct _PackageDispatchThread {
    _BuckyFramework* framework;

    BFX_THREAD_HANDLE handle;

    uv_sem_t sem;

    uv_mutex_t lock; // for packages   
    BfxList packages;

    volatile int32_t duringWakeup;
} _PackageDispatchThread;

static size_t _dispatchPeekPackages(_PackageDispatchThread* thread, BfxList* list) {
    size_t count = 0;
    if (thread->packages.count > 0) {
        uv_mutex_lock(&thread->lock);

        count = thread->packages.count;
        if (count > 0) {
            BfxListSwap(&thread->packages, list);
        }

        uv_mutex_unlock(&thread->lock);
    }

    return count;
}

static void _dispatchDealPackage(_PackageDispatchThread* thread, _UDPDispatchPackage* pkg) {

    _BuckyFramework* framework = pkg->socket->framework;
    framework->events.bdtPushUdpSocketData(framework->base.stack, (BDT_SYSTEM_UDP_SOCKET)pkg->socket
        , pkg->data, pkg->bytes, &pkg->remote
        , pkg->socket->udata.userData);

    _deleteUDPDispatchPackage(pkg);
}

static void _dispatchDealPackages(_PackageDispatchThread* thread) {
    BfxList tmp;
    BfxListInit(&tmp); 

    while (_dispatchPeekPackages(thread, &tmp) > 0) {

        BfxListItem* it = BfxListFirst(&tmp);
        while (it) {
            _UDPDispatchPackage* pkg = (_UDPDispatchPackage*)it;
            it = BfxListNext(&tmp, it);

            _dispatchDealPackage(thread, pkg);
        }

        BfxListClear(&tmp);
    }
}

static int _recvDispatchMain(_PackageDispatchThread* thread) {
    for (;;) {
        uv_sem_wait(&thread->sem);

        // BLOG_INFO("thread wakeup %p", thread);

        BfxAtomicExchange32(&thread->duringWakeup, 0);

        _dispatchDealPackages(thread);
    }

    return 0;
}

static int _initPackageDispatchThread(_PackageDispatchThread* thread, _BuckyFramework* framework) {
    thread->framework = framework;
    uv_sem_init(&thread->sem, 0);
    uv_mutex_init(&thread->lock);
    BfxListInit(&thread->packages);
    thread->duringWakeup = 0;

    BfxThreadOptions options = {
       .stackSize = 0,
       .ssize = sizeof(BfxThreadOptions),
    };

    return BfxCreateThread((BfxThreadMain)_recvDispatchMain, thread, options, &thread->handle);
}

static void _dispatchPackage(_PackageDispatchThread* thread, _UDPDispatchPackage* pkg) {
    // BLOG_INFO("will dispatch package to %p", thread);

    {
        uv_mutex_lock(&thread->lock);

        BfxListPushBack(&thread->packages, &pkg->stub);

        uv_mutex_unlock(&thread->lock);
    }

    int cur = BfxAtomicCompareAndSwap32(&thread->duringWakeup, 0, 1);
    if (cur == 0) {
        uv_sem_post(&thread->sem);
    }
}

//////////////////////////////////////////////////////////////////////////
// dispatch manager

int _initPackageDispatchManager(_BuckyFramework* framework, _PackageDispatchManager* manager, const BuckyFrameworkOptions* options) {
    manager->count = options->dispatchThreadCount > 0? options->dispatchThreadCount : 4;
    manager->threads = BFX_MALLOC_ARRAY(_PackageDispatchThread, manager->count);
    memset(manager->threads, 0, sizeof(_PackageDispatchThread) * manager->count);

    int err = 0;
    for (size_t i = 0; i < manager->count; ++i) {
        err = _initPackageDispatchThread(&manager->threads[i], framework);
        if (err != 0) {
            break;
        }
    }

    return err;
}

void _uninitPackageDispatchManager(_BuckyFramework* framework, _PackageDispatchManager* manager) {
    // TODO
}

static _PackageDispatchThread* _selectDispatchThread(_PackageDispatchManager* manager, _UVUDPSocket* socket) {
    size_t index = manager->index++ % manager->count;

    return &manager->threads[index];
}


void _uvUdpSocketOnData(_UVUDPSocket* socket, const char* data, size_t bytes, const BdtEndpoint* remote) {
    _PackageDispatchManager* manager = &socket->framework->dispatchManager;
    
    _PackageDispatchThread* thread = _selectDispatchThread(manager, socket);
    BLOG_CHECK(thread);

    _UDPDispatchPackage* pkg = _newUDPDispatchPackage(socket, data, bytes, remote);
    _dispatchPackage(thread, pkg);
}

