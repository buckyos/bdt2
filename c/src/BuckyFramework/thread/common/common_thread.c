#include <BuckyBase.h>
#include "../bucky_thread.h"


#include "./msg_manager.h"
#include "./thread_info.h"
#include "./thread_manager.h"


static void _threadMain(_ThreadInfo* info) {
    assert(info->state == _ThreadState_init);
    info->state = _ThreadState_running;

    if (info->preMain) {
        int ret = info->preMain(info);
        if (ret != 0) {
            return;
        }
    }

    // 保存到当前线程tls
    _setThreadInfo(info);

    // 注册到线程管理器
    _threadManagerAdd(info);

    // 初始化完毕，触发事件
    assert(info->initLock);
    uv_sem_post(info->initLock);

    assert(info->main);
    info->exitCode = info->main(info->args);

    if (info->postMain) {
        info->postMain(info);
    }

    // 检查并执行退出事件
    if (info->atExitManager) {
        _execAtExits(info->atExitManager);
        _deleteAtExitManager(info->atExitManager);
        info->atExitManager = NULL;
    }

    // 从线程管理器移除
    _threadManagerDelete(info);

    // 重置当前线程tls
    _resetThreadInfo();

    assert(info->state == _ThreadState_running);
    info->state = _ThreadState_end;

    _threadInfoRelease(info);
}

BFX_API(int32_t) BfxThreadAddRef(BFX_THREAD_HANDLE thandle) {
    return _threadInfoAddRef((_ThreadInfo*)thandle);
}

BFX_API(int32_t) BfxThreadRelease(BFX_THREAD_HANDLE thandle) {
    return _threadInfoRelease((_ThreadInfo*)thandle);
}

BFX_API(bool) BfxIsCurrentThread(BFX_THREAD_HANDLE thandle) {
    _ThreadInfo* info = _getCurrentThreadInfo();
    if (info == NULL) {
        return false;
    }

    return info == (_ThreadInfo*)thandle;
}

static int _startThread(_ThreadInfo* info, BfxThreadOptions options) {
    info->state = _ThreadState_init;

    /*
    uv_thread_options_t top = {
        .flags = UV_THREAD_NO_FLAGS
    };

    if (options.stackSize > 0) {
        top.flags = UV_THREAD_HAS_STACK_SIZE;
        top.stack_size = options.stackSize;
    }
    */

    uv_sem_t initSem;
    uv_sem_init(&initSem, 0);
    info->initLock = &initSem;

    int ret = uv_thread_create(&info->thread, (uv_thread_cb)_threadMain, info);
    if (ret != 0) {
        return BFX_RESULT_FAILED;
    }

    // 等待初始化完成
    uv_sem_wait(&initSem);
    uv_sem_destroy(&initSem);
    info->initLock = NULL;

    return 0;
}

BFX_API(int) BfxCreateThread(BfxThreadMain main, void* args, BfxThreadOptions options, BFX_THREAD_HANDLE* thandle) {
    _ThreadInfo* info = _newThreadInfo(BfxThreadType_Native);
    info->main = main;
    info->args = args;

    int ret = _startThread(info, options);
    if (ret != 0) {
        _threadInfoRelease(info);
        if (thandle) {
            thandle = NULL;
        }
    } else {
        if (thandle) {
            *thandle = (BFX_THREAD_HANDLE)info;
            _threadInfoAddRef(info);
        }
    }

    return ret;
}


static void _onAsyncCallback() {
    _IOThreadInfo* info = (_IOThreadInfo*)_getCurrentThreadInfo();
    assert(info);

    while (_messageManagerDispatchMsg(info->msgManager)) {};
}

static int _ioThreadPreMain(void* args) {
    _IOThreadInfo* info = (_IOThreadInfo*)args;

    // 初始化消息循环
    info->loop = _newSysLoop();
    if (info->loop == NULL) {
        BLOG_ERROR("init thread loop failed!");
        return BFX_RESULT_FAILED;
    }

    // 初始化消息管理器
    info->msgManager = _messageManagerNew();

    _sysLoopSetActiveCallBack(info->loop, _onAsyncCallback);

    return 0;
}

static void _ioThreadPostMain(void* args) {
    _IOThreadInfo* info = (_IOThreadInfo*)args;

    _deleteSysLoop(info->loop);
    info->loop = NULL;

    _messageManagerRelease(info->msgManager);
    info->msgManager = NULL;
}

static int _ioThreadMain(_IOThreadInfo* info) {

    // 暂不支持消息循环的嵌套
    _sysLoopRun(info->loop);

    return 0;
}

BFX_API(int) BfxCreateIOThread(BfxThreadOptions options, BFX_THREAD_HANDLE* thandle) {
    _IOThreadInfo* info = (_IOThreadInfo*)_newThreadInfo(BfxThreadType_IO);
    info->base.main = (BfxThreadMain)_ioThreadMain;
    info->base.preMain = _ioThreadPreMain;
    info->base.postMain = _ioThreadPostMain;
    info->base.args = info;

    int ret = _startThread((_ThreadInfo*)info, options);
    if (ret != 0) {
        _threadInfoRelease((_ThreadInfo*)info);
        if (thandle) {
            *thandle = NULL;
        }
    } else {
        if (thandle) {
            *thandle = (BFX_THREAD_HANDLE)info;
            _threadInfoAddRef((_ThreadInfo*)info);
        }
    }

    return ret;
}

BFX_API(BFX_THREAD_HANDLE) BfxGetCurrentThread() {
    _ThreadInfo* info = _getCurrentThreadInfo();
    if (info == NULL) {
        BLOG_ERROR("threadInfo is null!");
        return NULL;
    }

    _threadInfoAddRef(info);

    return (BFX_THREAD_HANDLE)(info);
}

BFX_API(int) BfxPostMessage(BFX_THREAD_HANDLE thandle, BfxOnMessage onMessage, BfxUserData userData) {
    _ThreadInfo* info = NULL;
    if (thandle == NULL) {
        info = _getCurrentThreadInfo();
    } else {
        info = (_ThreadInfo*)thandle;
    }

    if (info->type != BfxThreadType_IO) {
        BLOG_ERROR("invalid thread type for postMessage! type=%d", info->type);
        return BFX_RESULT_FAILED;
    }

    _ThreadMessageProxy* proxy = _getThreadProxy(info);
    if (proxy == NULL) {
        BLOG_ERROR("thread proxy not found!");
        return BFX_RESULT_FAILED;
    }

    int err = _threadProxyPostMessage(proxy, onMessage, userData);
    _threadProxyRelease(proxy);

    return err;
}

BFX_API(int) BfxThreadJoin(BFX_THREAD_HANDLE thandle) {
    _ThreadInfo* info = (_ThreadInfo*)thandle;
    assert(info->thread);

    int ret = uv_thread_join(&info->thread);
    if (ret != 0) {
        BLOG_ERROR("join thread error, ret=%d", ret);
        return BFX_RESULT_FAILED;
    }

    return ret;
}

BFX_API(int) BfxThreadAtExit(BfxThreadExitCallBack proc, BfxUserData userData) {
    return _atExit(proc, userData);
}

static void _postQuitCallBack(void* args) {
    _IOThreadInfo* info = _getCurrentIOThreadInfo();
    assert(info);

    info->base.exitCode = (int)(intptr_t)args;

    _sysLoopStop(info->loop);
}

BFX_API(int) BfxPostQuit(BFX_THREAD_HANDLE thandle, int code) {
    BfxUserData ud = { 0 };
    ud.userData = (void*)(intptr_t)code;

    return BfxPostMessage(thandle, _postQuitCallBack, ud);
}