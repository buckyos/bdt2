#include <BuckyBase.h>
#include "./message_pump.h"
#include "./timer/loop_timer.inl"

//////////////////////////////////////////////////////////////////////////
//

static void _loopUpdateTime(_BuckyMessageLoop* loop) {
    loop->time = BfxTimeGetNow(false);
}

int _initBuckyMessageLoop(_BuckyMessageLoop* loop) {
    _initBuckyLoopTimerManager(loop, &loop->timerManager);

    loop->pump.loop = loop;
    int ret = _initBuckyMessagePump(&loop->pump);
    if (ret != 0) {
        return ret;
    }

    return ret;
}

int _uninitBuckyMessageLoop(_BuckyMessageLoop* loop) {
    _uninitBuckyMessagePump(&loop->pump);

    _uninitBuckyLoopTimerManager(&loop->timerManager);

    return 0;
}

void _buckyMessageLoopSetActiveCallBack(_BuckyMessageLoop* loop, void (*onProxyActive)(_BuckyMessageLoop*)) {
    loop->pump.onProxyActive = onProxyActive;
}

static bool _loopCheckEnd(_BuckyMessageLoop* loop) {
    return (loop->state == _BuckyMessageLoopState_End);
}

int _buckyMessageLoopActive(_BuckyMessageLoop* loop) {
    return _buckyMessagePumpActive(&loop->pump);
}

int _buckyMessageLoopRun(_BuckyMessageLoop* loop) {
    for (;;) {
        _loopUpdateTime(loop);

        bool onceMore = false;

        // �����������û�е������ں��¼�
        int ret = _buckyMessagePumpPoll(&loop->pump, 0);
        if (ret > 0) {
            onceMore = true;
        }

        if (_loopCheckEnd(loop)) {
            break;
        }

        _loopUpdateTime(loop);

        // ���timer����
        int64_t timeout = 0;
        ret = _buckyPollTimer(&loop->timerManager, &timeout);
        if (ret > 0) {
            onceMore = true;
        }

        if (_loopCheckEnd(loop)) {
            break;
        }

        if (onceMore) {
            continue;
        }

        _loopUpdateTime(loop);

        // �����ȴ�
        assert(timeout == -1 || timeout > 0);
        _buckyMessagePumpPoll(&loop->pump, timeout);
    }

    return 0;
}

void _buckyMessageLoopStop(_BuckyMessageLoop* loop) {
    loop->state = _BuckyMessageLoopState_End;
}
