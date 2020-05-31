#pragma once

#include <BuckyBase.h>

typedef struct _BuckyThreadPool _BuckyThreadPool;
typedef struct {
    _BuckyThreadPool* tcpPool;
    _BuckyThreadPool* udpPool;
    _BuckyThreadPool* timerPool;

} _BuckyThreadContainer;

int _initThreadContainer(_BuckyThreadContainer* container, const BuckyFrameworkOptions* options);
int _uninitThreadContainer(_BuckyThreadContainer* container);

typedef enum {
    _BuckyThreadTarget_TCP = 0,
    _BuckyThreadTarget_UDP = 1,
    _BuckyThreadTarget_TIMER = 2,
} _BuckyThreadTarget;

// 选择一个有效线程，返回值不为空则须要释放
BFX_THREAD_HANDLE _threadContainerSelectThread(_BuckyThreadContainer* container, _BuckyThreadTarget target);