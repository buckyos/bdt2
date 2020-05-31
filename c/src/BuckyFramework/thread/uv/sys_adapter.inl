#include "../common/sys_adapter.h"
#include "../../uv_comnon.h"

//////////////////////////////////////////////////////////////////////////
// proxy

static void _onAsyncCallback(uv_async_t* proxy) {
    _buckySysMessageLoop* loop = (_buckySysMessageLoop*)proxy->data;

    loop->activeCallBack();
}

static uv_async_t* _newProxy(uv_loop_t* loop) {
    uv_async_t* proxy = (uv_async_t*)BfxMalloc(sizeof(uv_async_t));
    proxy->data = loop;
  
    int ret = uv_async_init(loop, proxy, _onAsyncCallback);
    if (ret != 0) {
        BfxFree(proxy);
        return NULL;
    }

    return proxy;
}


//////////////////////////////////////////////////////////////////////////
// loop

_buckySysMessageLoop* _newSysLoop() {
    _buckySysMessageLoop* loop = (_buckySysMessageLoop*)BfxMalloc(sizeof(_buckySysMessageLoop));
    int ret = uv_loop_init(&loop->loop);
    if (ret != 0) {
        BfxFree(loop);

        return NULL;
    }

    loop->loop.data = loop;

    loop->proxy = _newProxy(&loop->loop);
    if (loop->proxy == NULL) {
        _deleteSysLoop(loop);

        return NULL;
    }

    return loop;
}

static void _finalDeleteSysLoop(_buckySysMessageLoop* loop) {
    assert(loop->proxy == NULL);

    loop->activeCallBack = NULL;
    int err = uv_loop_close(&loop->loop);
    BLOG_CHECK(err == 0);

    BfxFree(loop);
}

static void _deleteProxy(uv_async_t* proxy) {
    // _buckySysMessageLoop* sysLoop = (_buckySysMessageLoop*)((uv_loop_t*)proxy->data)->data;

    BfxFree(proxy);
}

static void _proxyCloseCallBack(uv_handle_t* proxy) {
    _deleteProxy((uv_async_t*)proxy);
}

void _deleteSysLoop(_buckySysMessageLoop* loop) {
    
    if (loop->proxy) {
        // 必须proxy完全关闭后，才可以close loop
        uv_close((uv_handle_t*)loop->proxy, _proxyCloseCallBack);
        loop->proxy = NULL;

        uv_run(&loop->loop, UV_RUN_NOWAIT);

        _finalDeleteSysLoop(loop);

    } else {
        _finalDeleteSysLoop(loop);
    }
}

int _sysLoopRun(_buckySysMessageLoop* loop) {
    int ret = uv_run(&loop->loop, UV_RUN_DEFAULT);
    if (ret != 0) {
        ret = _uvTranslateError(ret);
    }

    return ret;
}

int _sysLoopStop(_buckySysMessageLoop* loop) {
    uv_stop(&loop->loop);

    return 0;
}

void _sysLoopSetActiveCallBack(_buckySysMessageLoop* loop, void (*cb)()) {
    loop->activeCallBack = cb;
}

int _sysLoopActive(_buckySysMessageLoop* loop) {
    int ret = uv_async_send(loop->proxy);
    if (ret != 0) {
        ret = _uvTranslateError(ret);
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////
// uv timers

void _initSysTimer(_buckySysMessageLoop* loop, _buckySysMessageLoopTimer* timer) {
    uv_timer_init(&loop->loop, timer);
}

void _uninitSysTimer(_buckySysMessageLoopTimer* timer, void (*cb)(_buckySysMessageLoopTimer*)) {
    uv_close((uv_handle_t*)timer, (uv_close_cb)cb);
}

int _startSysTimer(_buckySysMessageLoopTimer* timer, void (*onTimer)(_buckySysMessageLoopTimer* timer), int64_t timeout, bool once) {

    // 转换到毫秒
    uint64_t delayInMicroseconds = timeout / 1000;
    if (timeout % 1000) {
        ++delayInMicroseconds;
    }

    uint64_t repeat;
    if (once) {
        repeat = 0;
    } else {
        repeat = delayInMicroseconds;
    }

    int ret = uv_timer_start(timer, (uv_timer_cb)onTimer, (uint64_t)delayInMicroseconds, repeat);
    if (ret != 0) {
        ret = _uvTranslateError(ret);
    }

    return ret;
}

int _stopSysTimer(_buckySysMessageLoopTimer* timer) {

    int ret = uv_timer_stop(timer);
    if (ret != 0) {
        ret = _uvTranslateError(ret);
    }

    return ret;
}
