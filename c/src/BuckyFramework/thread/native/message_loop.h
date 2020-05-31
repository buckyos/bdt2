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

// �̰߳�ȫ�������������̵߳���
int _buckyMessageLoopActive(_BuckyMessageLoop* loop);

// ������Ϣѭ��ֱ��stop
int _buckyMessageLoopRun(_BuckyMessageLoop* loop);

// ֹͣһ���̣߳�ֻ�����ڵ�ǰ�̵߳���
void _buckyMessageLoopStop(_BuckyMessageLoop* loop);