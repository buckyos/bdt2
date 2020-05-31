
#include <BuckyBase.h>
#include "../bucky_thread.h"
#include "./thread_manager.h"


typedef struct {
	BfxListItem stub;

    _ThreadInfo* info;
    _ThreadMessageProxy* proxy;
} _ThreadInfoItem;

static uv_once_t _threadManagerOnce = UV_ONCE_INIT;
static _ThreadManager _threadManager;

static void _initThreadManager(_ThreadManager* manager) {
	uv_mutex_init(&manager->lock);
	BfxListInit(&manager->threads);
}
static void _initOnce(void) {
	_initThreadManager(&_threadManager);
}

_ThreadManager* _getThreadManger()
{
	uv_once(&_threadManagerOnce, _initOnce);
	return &_threadManager;
}

int _threadManagerAdd(_ThreadInfo* info) {
	assert(info);

	_ThreadManager* manager = _getThreadManger();

	_ThreadInfoItem* item = (_ThreadInfoItem*)BfxMalloc(sizeof(_ThreadInfoItem));
	item->info = info;
    item->proxy = NULL;

	// 如果是消息线程，那么创建对应的proxy
	if (info->type == BfxThreadType_IO) {
		_IOThreadInfo* ioInfo = (_IOThreadInfo*)info;
		assert(ioInfo->msgManager);
        item->proxy = _newThreadProxy(ioInfo);
	}

    {
        uv_mutex_lock(&manager->lock);
        BfxListPushBack(&manager->threads, &item->stub);
        uv_mutex_unlock(&manager->lock);
    }

    return 0;
}

static void _deleteThreadInfoItem(_ThreadInfoItem* item) {
    assert(item->info);

    if (item->proxy) {
        _threadProxyRelease(item->proxy);
        item->proxy = NULL;
    }

    item->info = NULL;
    BfxFree(item);
}

int _threadManagerDelete(_ThreadInfo* info) {
    _ThreadManager* manager = _getThreadManger();

    bool exists = false;
    _ThreadInfoItem* item = NULL;

    {
        uv_mutex_lock(&manager->lock);
        for (BfxListItem* it = BfxListFirst(&manager->threads);
            it != NULL;
            it = BfxListNext(&manager->threads, it)) {

            item = (_ThreadInfoItem*)it;
            if (item->info == info) {
                BfxListErase(&manager->threads, it);
                exists = true;
                break;
            }
        }
        uv_mutex_unlock(&manager->lock);
    }

    if (!exists) {
        BLOG_ERROR("thread info not found!");
        return BFX_RESULT_NOT_FOUND;
    }

    assert(item);
    _deleteThreadInfoItem(item);

    return 0;
}

_ThreadMessageProxy* _getThreadProxy(_ThreadInfo* info)
{
    _ThreadManager* manager = _getThreadManger();
    
    bool exists = false;
    _ThreadInfoItem* item = NULL;

    {
        uv_mutex_lock(&manager->lock);
        for (BfxListItem* it = BfxListFirst(&manager->threads);
            it != NULL;
            it = BfxListNext(&manager->threads, it)) {

            item = (_ThreadInfoItem*)it;
            if (item->info == info) {

                if (item->proxy) {
                    _threadProxyAddRef(item->proxy);
                }
                exists = true;
                break;
            }
        }
        uv_mutex_unlock(&manager->lock);
    }

    if (!exists) {
        BLOG_ERROR("thread info not found!");
        return NULL;
    }

    assert(item);
    if (item->proxy == NULL) {
        BLOG_ERROR("thread proxy is null!");
        return NULL;
    }

    return item->proxy;
}

