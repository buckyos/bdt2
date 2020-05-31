#pragma once


#ifdef BFX_FRAMEWORK_NATIVE
#include "../native/message_loop.h"
typedef struct {
    _BuckyMessageLoop loop;
    void (*activeCallBack)();
} _buckySysMessageLoop;

typedef _BuckyLoopTimer _buckySysMessageLoopTimer;

#elif defined(BFX_FRAMEWORK_UV)

typedef struct {
    uv_loop_t loop;
    uv_async_t* proxy;
    void (*activeCallBack)();
} _buckySysMessageLoop;

typedef uv_timer_t _buckySysMessageLoopTimer;

#else
#error "not impl"
#endif

//////////////////////////////////////////////////////////////////////////
// loop
_buckySysMessageLoop* _newSysLoop();
void _deleteSysLoop(_buckySysMessageLoop* loop);

int _sysLoopRun(_buckySysMessageLoop* loop);
int _sysLoopStop(_buckySysMessageLoop* loop);

void _sysLoopSetActiveCallBack(_buckySysMessageLoop* loop, void (*cb)());
int _sysLoopActive(_buckySysMessageLoop* loop);


//////////////////////////////////////////////////////////////////////////
// timer

void _initSysTimer(_buckySysMessageLoop* loop, _buckySysMessageLoopTimer* timer);
void _uninitSysTimer(_buckySysMessageLoopTimer* timer, void (*cb)(_buckySysMessageLoopTimer*));

int _startSysTimer(_buckySysMessageLoopTimer* timer, void (*onTimer)(_buckySysMessageLoopTimer* timer), int64_t timeout, bool once);
int _stopSysTimer(_buckySysMessageLoopTimer* timer);