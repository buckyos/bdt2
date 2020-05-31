#include "../BuckyBase.h"
#include "./event_emitter.h"


typedef struct {
    BfxListItem stub;

    BfxEventListener cb;
    BfxUserData udata;
    int32_t cookie;

} _BfxEventListener;


typedef struct {
    BfxListItem stub;

    char name[BFX_EVENTEMITTER_EVENTNAME_MAX_LENGTH + 1];
    BfxList listeners;

    int32_t nextCookie;
} _BfxEvent;


typedef struct {
    uv_mutex_t lock;
    BfxList events;
}_BfxEventEmitter;


_BfxEvent* _newBfxEvent(void* place, const char* name) {
    assert(strlen(name) <= BFX_EVENTEMITTER_EVENTNAME_MAX_LENGTH);

    _BfxEvent* event;
    if (place) {
        event = (_BfxEvent*)place;
    } else {
        event = (_BfxEvent*)BfxMalloc(sizeof(_BfxEvent));
    }
    
    strcpy(event->name, name);
    BfxListInit(&event->listeners);
    event->nextCookie = 1;

    return event;
}

static void _deleteBfxEvent(void* place, _BfxEvent* event) {
    event->nextCookie = 0;

    assert(event->listeners.count == 0);
    BfxListClear(&event->listeners);

    if (!place) {
        BfxFree(event);
    }
}

static _BfxEventListener* _newListener(int32_t cookie, BfxEventListener cb, BfxUserData udata) {
    _BfxEventListener* listener = (_BfxEventListener*)BfxMalloc(sizeof(_BfxEventListener));
    listener->cookie = cookie;
    listener->cb = cb;
    listener->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }

    return listener;
}

static void _deleteListener(_BfxEventListener* listener) {
    if (listener->udata.lpfnReleaseUserData) {
        listener->udata.lpfnReleaseUserData(listener->udata.userData);
    }

    listener->cookie = 0;
    listener->cb = NULL;
    BfxFree(listener);
}

static int32_t _eventAddListener(_BfxEvent* event, BfxEventListener cb, BfxUserData udata) {
    _BfxEventListener* listener = _newListener(event->nextCookie++, cb, udata);
    BfxListPushBack(&event->listeners, &listener->stub);

    return listener->cookie;
}

static _BfxEventListener* _eventRemoveListener(_BfxEvent* event, int32_t cookie) {
    for (_BfxEventListener* listener = (_BfxEventListener*)BfxListFirst(&event->listeners);
        listener; listener = (_BfxEventListener*)BfxListNext(&event->listeners, &listener->stub)) {
        if (listener->cookie == cookie) {
            BfxListErase(&event->listeners, &listener->stub);
            return listener;
        }
    }

    return NULL;
}

static void _freeListeners(BfxList *listeners) {
    _BfxEventListener* listener = (_BfxEventListener*)BfxListFirst(listeners);

    while (listener) {
        _BfxEventListener* tmp = listener;
        listener = (_BfxEventListener*)BfxListNext(listeners, &listener->stub);

        _deleteListener(tmp);
    }
}

static void _eventRemoveAllListeners(_BfxEvent* event, BfxList *listeners) {
    assert(listeners->count == 0);

    BfxListSwap(&event->listeners, listeners);
}

static size_t _prepareBfxEvents(_BfxEvent* event, _BfxEventListener listeners[], size_t size) {
    assert(event->listeners.count <= size);

    for (_BfxEventListener* listener = (_BfxEventListener*)BfxListFirst(&event->listeners);
        listener; listener = (_BfxEventListener*)BfxListNext(&event->listeners, &listener->stub)) {

        *listeners++ = *listener;
        if (listener->udata.lpfnAddRefUserData) {
            listener->udata.lpfnAddRefUserData(listener->udata.userData);
        }
    }

    return event->listeners.count;
}

static _BfxEventEmitter* _newEventEmitter() {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)BfxMalloc(sizeof(_BfxEventEmitter));
    uv_mutex_init(&emitter->lock);
    BfxListInit(&emitter->events);

    return emitter;
}

static void _deleteEventEmitter(_BfxEventEmitter* emitter) {
    uv_mutex_destroy(&emitter->lock);
    BfxFree(emitter);
}

static void _freeEvents(BfxList* events) {
    _BfxEvent* event = (_BfxEvent*)BfxListFirst(events);

    while (event) {
        _BfxEvent* tmp = event;
        event = (_BfxEvent*)BfxListNext(events, &event->stub);

        _deleteBfxEvent(NULL, tmp);
    }
}


static void _eventEmitterAddEvent(_BfxEventEmitter* emitter, const char* name) {
    _BfxEvent* event = _newBfxEvent(NULL, name);
    BfxListPushBack(&emitter->events, &event->stub);
}

static void _eventEmitterRemoveEvent(_BfxEventEmitter* emitter, _BfxEvent* event) {
    BfxListErase(&emitter->events, &event->stub);
    _deleteBfxEvent(NULL, event);
}


static _BfxEvent* _eventEmitterGetEvent(_BfxEventEmitter* emitter, const char* name) {
    for (_BfxEvent* event = (_BfxEvent*)BfxListFirst(&emitter->events);
        event; event = (_BfxEvent*)BfxListNext(&emitter->events, &event->stub)) {
        if (strcmp(name, event->name) == 0) {
            return event;
        }
    }

    return NULL;
}


BFX_API(BFX_EVENT_EMITTER) BfxCreateEventEmitter() {
    return (BFX_EVENT_EMITTER)_newEventEmitter();
}

BFX_API(void) BfxDestroyEventEmitter(BFX_EVENT_EMITTER eventEmitter) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    BfxList allListeners, events;
    BfxListInit(&allListeners);
    BfxListInit(&events);

    {
        uv_mutex_lock(&emitter->lock);

        // 清空所有事件的回调
        for (_BfxEvent* event = (_BfxEvent*)BfxListFirst(&emitter->events);
            event; event = (_BfxEvent*)BfxListNext(&emitter->events, &event->stub)) {
            
            BfxList listeners;
            BfxListInit(&listeners);
            _eventRemoveAllListeners(event, &listeners);

            BfxListMerge(&allListeners, &listeners, true);
        }

        // 清空事件
        BfxListSwap(&emitter->events, &events);
        
        uv_mutex_unlock(&emitter->lock);
    }

    _deleteEventEmitter(emitter);

    // 最后清空两个队列
    _freeEvents(&events);
    _freeListeners(&allListeners);
}

BFX_API(bool) BfxEventEmitterAddEvent(BFX_EVENT_EMITTER eventEmitter, const char* name) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    {
        uv_mutex_lock(&emitter->lock);

        _eventEmitterAddEvent(emitter, name);

        uv_mutex_unlock(&emitter->lock);
    }

    return true;
}


BFX_API(int32_t) BfxEventEmitterOn(BFX_EVENT_EMITTER eventEmitter, const char* name, BfxEventListener cb, BfxUserData udata) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    int32_t cookie = 0;
    {
        uv_mutex_lock(&emitter->lock);

        _BfxEvent* event = _eventEmitterGetEvent(emitter, name);
        if (event) {
            cookie = _eventAddListener(event, cb, udata);
        } else {
            BLOG_ERROR("event not found! event=%s", name);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    return cookie;
}

BFX_API(bool) BfxEventEmitterOff(BFX_EVENT_EMITTER eventEmitter, const char* name, int32_t cookie) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    bool ret = false;
    {
        uv_mutex_lock(&emitter->lock);

        _BfxEvent* event = _eventEmitterGetEvent(emitter, name);
        if (event) {
            ret = _eventRemoveListener(event, cookie);
        } else {
            BLOG_ERROR("event not found! event=%s", name);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    return ret;
}

BFX_API(bool) BfxEventEmitterRemoveEvent(BFX_EVENT_EMITTER eventEmitter, const char* name) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    bool ret = false;
    BfxList listeners;
    BfxListInit(&listeners);

    {
        uv_mutex_lock(&emitter->lock);

        _BfxEvent* event = _eventEmitterGetEvent(emitter, name);
        if (event) {
            // 需要清空回调
            _eventRemoveAllListeners(event, &listeners);

            _eventEmitterRemoveEvent(emitter, event);
            ret = true;
        } else {
            BLOG_ERROR("event not found! event=%s", name);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    if (ret && listeners.count > 0) {
        _freeListeners(&listeners);
    }

    return ret;
}

BFX_API(bool) BfxEventEmitterRemoveAllListeners(BFX_EVENT_EMITTER eventEmitter, const char* name) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    bool ret = false;
    BfxList listeners;
    BfxListInit(&listeners);

    {
        uv_mutex_lock(&emitter->lock);

        _BfxEvent* event = _eventEmitterGetEvent(emitter, name);
        if (event) {
            _eventRemoveAllListeners(event, &listeners);
            ret = true;
        } else {
            BLOG_ERROR("event not found! event=%s", name);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    if (ret && listeners.count > 0) {
        _freeListeners(&listeners);
    }

    return ret;
}

BFX_API(void) BfxEventEmitterEmit(BFX_EVENT_EMITTER eventEmitter, const char* name, BfxEventEmitter emitterCallBack, void* udata) {
    _BfxEventEmitter* emitter = (_BfxEventEmitter*)eventEmitter;

    _BfxEventListener listeners[BFX_EVENTEMITTER_MAX_LISTENERS];
    size_t count = 0;

    {
        uv_mutex_lock(&emitter->lock);

        _BfxEvent* event = _eventEmitterGetEvent(emitter, name);
        if (event && event->listeners.count > 0) {
            count = _prepareBfxEvents(event, listeners, BFX_EVENTEMITTER_MAX_LISTENERS);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    // 依次触发事件
    for (size_t i = 0; i < count; ++i) {
        _BfxEventListener listener = listeners[i];
        emitterCallBack(udata, listener.cb, listener.udata.userData);

        if (listener.udata.lpfnReleaseUserData) {
            listener.udata.lpfnReleaseUserData(listener.udata.userData);
        }
    }
}


typedef struct {
    uv_mutex_t lock;
    _BfxEvent event;
}_BfxSingleEventEmitter;

static _BfxSingleEventEmitter* _newSingleEventEmitter(const char* name) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)BfxMalloc(sizeof(_BfxSingleEventEmitter));

    uv_mutex_init(&emitter->lock);

    _newBfxEvent(&emitter->event, name);

    return emitter;
}

static void _deleteSingleEventEmitter(_BfxSingleEventEmitter* emitter) {
    _deleteBfxEvent(&emitter->event, &emitter->event);

    uv_mutex_destroy(&emitter->lock);

    BfxFree(emitter);
}

BFX_API(BFX_SINGLE_EVENT_EMITTER) BfxCreateSingleEventEmitter(const char* name) {
    return (BFX_SINGLE_EVENT_EMITTER)_newSingleEventEmitter(name);
}

BFX_API(void) BfxDestroySingleEventEmitter(BFX_SINGLE_EVENT_EMITTER eventEmitter) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)eventEmitter;

    BfxList allListeners;
    BfxListInit(&allListeners);

    {
        uv_mutex_lock(&emitter->lock);

        // 清空事件的回调
        _eventRemoveAllListeners(&emitter->event, &allListeners);

        uv_mutex_unlock(&emitter->lock);
    }

    _deleteSingleEventEmitter(emitter);

    // 最后清空队列
    if (allListeners.count > 0) {
        _freeListeners(&allListeners);
    }
}

BFX_API(int32_t) BfxSingleEventEmitterOn(BFX_SINGLE_EVENT_EMITTER eventEmitter, BfxEventListener cb, BfxUserData udata) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)eventEmitter;

    int32_t cookie = 0;
    {
        uv_mutex_lock(&emitter->lock);

        cookie = _eventAddListener(&emitter->event, cb, udata);

        uv_mutex_unlock(&emitter->lock);
    }

    return cookie;
}

BFX_API(bool) BfxSingleEventEmitterOff(BFX_SINGLE_EVENT_EMITTER eventEmitter, int32_t cookie) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)eventEmitter;

    bool ret = false;
    {
        uv_mutex_lock(&emitter->lock);

        ret = _eventRemoveListener(&emitter->event, cookie);

        uv_mutex_unlock(&emitter->lock);
    }

    return ret;
}

BFX_API(bool) BfxSingleEventEmitterRemoveAllListeners(BFX_SINGLE_EVENT_EMITTER eventEmitter) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)eventEmitter;

    BfxList listeners;
    BfxListInit(&listeners);

    {
        uv_mutex_lock(&emitter->lock);

        _eventRemoveAllListeners(&emitter->event, &listeners);

        uv_mutex_unlock(&emitter->lock);
    }

    if (listeners.count > 0) {
        _freeListeners(&listeners);
    }

    return true;
}

BFX_API(void) BfxSingleEventEmitterEmit(BFX_SINGLE_EVENT_EMITTER eventEmitter, BfxEventEmitter emitterCallBack, void* udata) {
    _BfxSingleEventEmitter* emitter = (_BfxSingleEventEmitter*)eventEmitter;

    _BfxEventListener listeners[BFX_EVENTEMITTER_MAX_LISTENERS];
    size_t count = 0;

    {
        uv_mutex_lock(&emitter->lock);

        if (emitter->event.listeners.count > 0) {
            count = _prepareBfxEvents(&emitter->event, listeners, BFX_EVENTEMITTER_MAX_LISTENERS);
        }

        uv_mutex_unlock(&emitter->lock);
    }

    // 依次触发事件
    for (size_t i = 0; i < count; ++i) {
        _BfxEventListener listener = listeners[i];
        emitterCallBack(udata, listener.cb, listener.udata.userData);

        if (listener.udata.lpfnReleaseUserData) {
            listener.udata.lpfnReleaseUserData(listener.udata.userData);
        }
    }
}