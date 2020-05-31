#include "../BuckyBase.h"
#include "./blog_thread.h"


typedef  struct _BLogItem {
    BfxListItem stub;
    size_t len;
} _BLogItem;

typedef struct _BLogThread {
    uv_thread_t thread;

    uv_mutex_t lock;

    int64_t lastFlushTick;
    int64_t flushInterval;
    _BLogFile* fileTarget;

    BfxList pool;
} _BLogThread;

// TODO 这里考虑固定大小内存池的优化
static _BLogItem* _newBLogItem(const char* line, size_t len) {
    _BLogItem* item = (_BLogItem*)BfxMalloc(sizeof(_BLogItem) + (len + 1) * sizeof(char));
    item->len = len;
    strncpy((char*)(item + 1), line, len);

    return item;
}

static void _deleteBLogItem(_BLogItem* item) {
    BfxFree(item);
}

static void _flushToFileTarget(_BLogThread* thread) {
    if (thread->pool.count == 0) {
        return;
    }

    BfxList lines;
    BfxListInit(&lines);
    {
        uv_mutex_lock(&thread->lock);

        BfxListSwap(&thread->pool, &lines);

        uv_mutex_unlock(&thread->lock);
    }
    
    _BLogItem* item = (_BLogItem*)BfxListFirst(&lines);

    while (item) {
        _BLogItem* tmp = item;
        item = (_BLogItem*)BfxListNext(&lines, &item->stub);

        _blogFileOutput(thread->fileTarget, (char*)(tmp + 1), tmp->len);
        _deleteBLogItem(tmp);
    }
}

static void _flushToDisk(_BLogThread* thread) {
    int64_t now = BfxTimeGetNow(false);
    if (now - thread->lastFlushTick < thread->flushInterval) {
        return;
    }

    thread->lastFlushTick = now;

    // 刷新到磁盘
    _blogFileFlush(thread->fileTarget);
}

static void _blogThreadMain(_BLogThread* thread) {
    for (;;) {
        
        BfxThreadSleep(1000 * 1000 * 5);

        _flushToFileTarget(thread);
        
        _flushToDisk(thread);
    }
}

static void _initBLogThread(_BLogThread* thread) {
    uv_thread_create(&thread->thread, (uv_thread_cb)_blogThreadMain, thread);

    uv_mutex_init(&thread->lock);

    thread->flushInterval = 1000 * 1000 * 15;

    BfxListInit(&thread->pool);
}

_BLogThread* _newBLogThread(_BLogFile* fileTarget, int64_t flushInterval) {
    _BLogThread* thread = (_BLogThread*)BfxMalloc(sizeof(_BLogThread));
    memset(thread, 0, sizeof(_BLogThread));

    _initBLogThread(thread);

    thread->fileTarget = fileTarget;

    if (flushInterval > 0) {
        thread->flushInterval = flushInterval * 1000;
    }

    return thread;
}

int _blogThreadOutput(_BLogThread* thread, const char* line, size_t len) {
    if (line == NULL || len == 0) {
        return 0;
    }

    _BLogItem* item = _newBLogItem(line, len);
    {
        uv_mutex_lock(&thread->lock);

        BfxListPushBack(&thread->pool, &item->stub);

        uv_mutex_unlock(&thread->lock);
    }

    return 0;
}