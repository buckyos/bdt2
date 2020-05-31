#pragma once


typedef struct _BuckyLoopTimer _BuckyLoopTimer;
typedef struct _BuckyMessageLoop _BuckyMessageLoop;

typedef void (*_buckyLoopTimerCallback)(_BuckyLoopTimer* timer);

typedef enum {
    _BuckyLoopTimerState_Init = 0,
    _BuckyLoopTimerState_Start = 1,
    _BuckyLoopTimerState_Stoped = 2,
}_BuckyLoopTimerState;

typedef struct _BuckyLoopTimer {
    void* stub[3];

    _BuckyMessageLoop* loop;

    int64_t timeout;
    int index;
    int state;
    bool once;
    _buckyLoopTimerCallback cb;

    // only for userdata
    void* data;
} _BuckyLoopTimer;


typedef struct {
    _BuckyMessageLoop* loop;

    struct {
        void* min;
        unsigned int nelts;
    } timers;

    int index;
} _BuckyLoopTimerManager;

void _initBuckyTimer(_BuckyMessageLoop* loop, _BuckyLoopTimer* timer);
void _uninitBuckyTimer(_BuckyLoopTimer* timer);

int _startBuckyTimer(_BuckyLoopTimer* timer, _buckyLoopTimerCallback cb, int64_t timeout, bool once);
int _stopBuckyTimer(_BuckyLoopTimer* timer);