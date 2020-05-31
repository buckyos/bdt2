#include "./message_pump.h"
#include "./timer/loop_timer.h"

//////////////////////////////////////////////////////////////////////////
//

typedef enum {
    _BuckyMessageLoopState_Running = 0,
    _BuckyMessageLoopState_End = 1,
} _BuckyMessageLoopState;

typedef struct _BuckyMessageLoop {
    _BuckyMessagePump pump;

    _BuckyLoopTimerManager timerManager;

    int64_t time;

    volatile int32_t state;
} _BuckyMessageLoop;

int _initBuckyMessageLoop(_BuckyMessageLoop* loop);
int _uninitBuckyMessageLoop(_BuckyMessageLoop* loop);

void _buckyMessageLoopSetActiveCallBack(_BuckyMessageLoop* loop, void (*onProxyActive)(_BuckyMessageLoop*));

// 线程安全，可以在任意线程调用
int _buckyMessageLoopActive(_BuckyMessageLoop* loop);

// 运行消息循环直到stop
int _buckyMessageLoopRun(_BuckyMessageLoop* loop);

// 停止一个线程，只可以在当前线程调用
void _buckyMessageLoopStop(_BuckyMessageLoop* loop);