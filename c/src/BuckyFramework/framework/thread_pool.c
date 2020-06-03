#include "../framework.h"
#include "./thread_pool.h"

typedef struct _BuckyThreadPoolItem {
    BFX_THREAD_HANDLE handle;
} _BuckyThreadPoolItem;

typedef struct _BuckyThreadPool {
    size_t count;

    uv_mutex_t lock;

    _BuckyThreadPoolItem* threads;
} _BuckyThreadPool;


static int _initThreadItem(_BuckyThreadPoolItem* item, const char* name, int index) {

    BfxThreadOptions options = {
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    snprintf(options.name, sizeof(options.name), "bucky_framework_thread_%s_%d", name, index);
    BFX_THREAD_HANDLE thread = NULL;
    int ret = BfxCreateIOThread(options, &thread);
    if (ret != 0) {
        return ret;
    }

    item->handle = thread;

    return 0;
}

static void _uninitThreadItem(_BuckyThreadPoolItem* item) {
    if (item->handle) {
        BfxThreadRelease(item->handle);
        item->handle = NULL;
    }
}

/*
static _BuckyThreadPool* _newThreadPool() {
    _BuckyThreadPool* pool = (_BuckyThreadPool*)BfxMalloc(sizeof(_BuckyThreadPool));
    pool->count = 0;
    BfxListInit(&pool->threads);
    
    uv_mutex_init(&pool->lock);

    return pool;
}
*/

static int _uninitThreadPool(_BuckyThreadPool* pool);
static int _initThreadPool(_BuckyThreadPool* pool, const char* name,  size_t count) {
    assert(count > 0);

    pool->count = count;

    pool->threads = BFX_MALLOC_ARRAY(_BuckyThreadPoolItem, count);
    memset(pool->threads, 0, sizeof(_BuckyThreadPoolItem) * count);

    uv_mutex_init(&pool->lock);

    int err = 0;
    for (int i = 0; i < count; ++i) {
        err = _initThreadItem(&pool->threads[i], name, i);
        if (err != 0) {
            break;
        }
    }
    
    if (err) {
        _uninitThreadPool(pool);
        return err;
    }

    return 0;
}

static int _uninitThreadPool(_BuckyThreadPool* pool) {
    
    _BuckyThreadPoolItem* threads = NULL;
    size_t count = 0;

    {
        threads = pool->threads;
        pool->threads = NULL;

        count = pool->count;
        pool->count = 0;
    }

    uv_mutex_destroy(&pool->lock);

    for (size_t i = 0; i < count; ++i) {
        const _BuckyThreadPoolItem* item = &threads[i];
        if (item->handle) {
            BfxPostQuit(item->handle, 0);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        _BuckyThreadPoolItem* item = &threads[i];
        if (item->handle) {
            BfxThreadJoin(item->handle);
        }

        _uninitThreadItem(item);
    }

    BfxFree(threads);

    return 0;
}

static BFX_THREAD_HANDLE _threadPoolSelectThread(_BuckyThreadPool* pool) {

    int index = 0;
    if (pool->count > 1) {
        index = BfxRandomRange32(0, (int)pool->count - 1);
    }

    BFX_THREAD_HANDLE handle = NULL;
    {
        uv_mutex_lock(&pool->lock);

        handle = pool->threads[index].handle;
        BfxThreadAddRef(handle);
       
        uv_mutex_unlock(&pool->lock);
    }

    return handle;
}

static _BuckyThreadPool* _newThreadPool(const char* name, size_t count) {
    _BuckyThreadPool* pool = (_BuckyThreadPool*)BfxMalloc(sizeof(_BuckyThreadPool));
    memset(pool, 0, sizeof(_BuckyThreadPool));

    int err = _initThreadPool(pool, name, count);
    if (err != 0) {
        BfxFree(pool);
        return NULL;
    }

    return pool;
}

static void _deleteThreadPool(_BuckyThreadPool* pool) {
    _uninitThreadPool(pool);

    BfxFree(pool);
}

//////////////////////////////////////////////////////////////////////////
// thread container

int _initThreadContainer(_BuckyThreadContainer* container, const BuckyFrameworkOptions* options) {
    
    // TODO 基于不同的策略创建不同的线程池
    container->tcpPool = _newThreadPool("tcp", options->tcpThreadCount > 0 ? options->tcpThreadCount : 4);
    container->udpPool = _newThreadPool("udp", options->udpThreadCount > 0 ? options->udpThreadCount : 4);
    container->timerPool = _newThreadPool("timer", options->timerThreadCount > 0 ? options->timerThreadCount : 2);

    return 0;
}

int _uninitThreadContainer(_BuckyThreadContainer* container) {
    
    {
        _BuckyThreadPool* pool = container->tcpPool;
        container->tcpPool = NULL;

        _deleteThreadPool(pool);
    }

    {
        _BuckyThreadPool* pool = container->udpPool;
        container->udpPool = NULL;

        _deleteThreadPool(pool);
    }

    {
        _BuckyThreadPool* pool = container->timerPool;
        container->timerPool = NULL;

        _deleteThreadPool(pool);
    }

    return 0;
}

BFX_THREAD_HANDLE _threadContainerSelectThread(_BuckyThreadContainer* container, _BuckyThreadTarget target) {
    
    // TODO
    _BuckyThreadPool* pool;
    if (target == _BuckyThreadTarget_TCP) {
        pool = container->tcpPool;
    } else if (target == _BuckyThreadTarget_UDP) {
        pool = container->udpPool;
    } else if (target == _BuckyThreadTarget_TIMER) {
        pool = container->timerPool;
    } else {
        BLOG_NOT_REACHED();
        return NULL;
    }

    return _threadPoolSelectThread(pool);
}