#include "../BuckyBase.h"
#include "../../BuckyFramework/thread/bucky_thread.h"


typedef struct {
    size_t size;
} _ThreadBuffer;

static void* _getData(_ThreadBuffer* buf) {
    return (buf + 1);
}

static _ThreadBuffer* _fromData(void* data) {
    return (_ThreadBuffer*)((char*)data - sizeof(_ThreadBuffer));
}

static _ThreadBuffer* _newBuffer(size_t size) {
    _ThreadBuffer* buf = (_ThreadBuffer*)BfxMalloc(sizeof(_ThreadBuffer) + size);
    buf->size = size;

    return buf;
}

static void _deleteBuffer(_ThreadBuffer* buf) {
    BfxFree(buf);
}

typedef struct {
    _ThreadBuffer* theOne;
}_ThreadBufferInstance;

static _ThreadBufferInstance* _newInstance() {
    _ThreadBufferInstance* instance = BFX_MALLOC_OBJ(_ThreadBufferInstance);
    instance->theOne = NULL;

    return instance;
}

static void _deleteInstance(_ThreadBufferInstance* instance) {
    if (instance->theOne) {
        _deleteBuffer(instance->theOne);
        instance->theOne = NULL;
    }

    BfxFree(instance);
}

static void* _threadBufferMalloc(_ThreadBufferInstance* instance, size_t size) {

    void* data = NULL;
    if (instance->theOne && instance->theOne->size >= size) {
        data = _getData(instance->theOne);
        instance->theOne = NULL;
    }
    else {
        _ThreadBuffer* buf = _newBuffer(size);
        if (buf) {
            data = _getData(buf);
        }
        else {
            BLOG_ERROR("alloc thread buf failed!");
        }
    }

    return data;
}

static void _threadBufferFree(_ThreadBufferInstance* instance, void* data) {
    if (data == NULL) {
        return;
    }

    _ThreadBuffer* buf = _fromData(data);

    if (instance->theOne == NULL) {
        instance->theOne = buf;
    }
    else if (buf->size > instance->theOne->size) {
        _deleteBuffer(instance->theOne);
        instance->theOne = buf;
    }
    else {
        _deleteBuffer(buf);
    }
}

// 基于隐式TLS的实现
static void _onBufferThreadQuit(void** slot) {
    if (*slot) {
        _ThreadBufferInstance* instance = (_ThreadBufferInstance*)(*slot);
        *slot = NULL;

        _deleteInstance(instance);
    }
}

BFX_API(void*) BfxThreadBufferMallocStatic(void** slot, size_t size) {
    if (*slot == NULL) {
        *slot = _newInstance();

        BfxUserData data = { 0 };
        data.userData = slot;

        BfxThreadAtExit((BfxThreadExitCallBack)_onBufferThreadQuit, data);
    }

    return _threadBufferMalloc((_ThreadBufferInstance*)*slot, size);
}

BFX_API(void) BfxThreadBufferFreeStatic(void** slot, void* data) {
    if (*slot == NULL) {
        BLOG_ERROR("invalid thread buffer slot state!");
        return;
    }

    _threadBufferFree((_ThreadBufferInstance*)*slot, data);
}


// 基于显式TLS的实现
static void _onBufferThreadQuit2(BfxTlsKey* key) {
    _ThreadBufferInstance* instance = (_ThreadBufferInstance*)BfxTlsGetData(key);
    if (instance) {
        BfxTlsSetData(key, NULL);
        _deleteInstance(instance);
    }
}

BFX_API(void*) BfxThreadBufferMallocDynamic(BfxTlsKey* key, size_t size) {
    BfxTlsGetData(key);

    _ThreadBufferInstance* instance = (_ThreadBufferInstance*)BfxTlsGetData(key);
    if (instance == NULL) {
        instance = _newInstance();
        BfxTlsSetData(key, instance);

        BfxUserData data = { 0 };
        data.userData = key;

        BfxThreadAtExit((BfxThreadExitCallBack)_onBufferThreadQuit2, data);
    }

    return _threadBufferMalloc(instance, size);
}

BFX_API(void) BfxThreadBufferFreeDynamic(BfxTlsKey* key, void* data) {
    _ThreadBufferInstance* instance = (_ThreadBufferInstance*)BfxTlsGetData(key);
    if (instance == NULL) {
        BLOG_ERROR("invalid thread buffer tls key state!");
        return;
    }

    _threadBufferFree(instance, data);
}