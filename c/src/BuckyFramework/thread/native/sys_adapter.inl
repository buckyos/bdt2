#include "../common/sys_adapter.h"

_buckySysMessageLoop* _newSysLoop() {
    _buckySysMessageLoop* loop = (_buckySysMessageLoop*)BfxMalloc(sizeof(_buckySysMessageLoop));
    int ret = _initBuckyMessageLoop(&loop->loop);
    if (ret != 0) {
        BfxFree(loop);
        return NULL;
    }

    return loop;
}

void _deleteSysLoop(_buckySysMessageLoop* loop) {
    _uninitBuckyMessageLoop(&loop->loop);

    BfxFree(loop);
}

int _sysLoopRun(_buckySysMessageLoop* loop) {
    return _buckyMessageLoopRun(&loop->loop);
}

int _sysLoopStop(_buckySysMessageLoop* loop) {
    _buckyMessageLoopStop(&loop->loop);

    return 0;
}

static void _onProxyActive(_BuckyMessageLoop* loop) {
    _buckySysMessageLoop* sysLoop = (_buckySysMessageLoop*)loop;
    
    sysLoop->activeCallBack();
}

void _sysLoopSetActiveCallBack(_buckySysMessageLoop* loop, void (*cb)()) {

    loop->activeCallBack = cb;
    _buckyMessageLoopSetActiveCallBack(&loop->loop, _onProxyActive);
}

int _sysLoopActive(_buckySysMessageLoop* loop) {
    return _buckyMessageLoopActive(&loop->loop);
}

void _initSysTimer(_buckySysMessageLoop* loop, _buckySysMessageLoopTimer* timer) {
    _initBuckyTimer(&loop->loop, timer);
}

void _uninitSysTimer(_buckySysMessageLoopTimer* timer, void (*cb)(_buckySysMessageLoopTimer*)) {
    _uninitBuckyTimer(timer);
    cb(timer);
}

int _startSysTimer(_buckySysMessageLoopTimer* timer, void (*onTimer)(_buckySysMessageLoopTimer* timer), int64_t timeout, bool once) {
    return _startBuckyTimer(timer, onTimer, timeout, once);
}

int _stopSysTimer(_buckySysMessageLoopTimer* timer) {
    return _stopBuckyTimer(timer);
}