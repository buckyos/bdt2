#include <BuckyBase.h>
#include "../message_pump.h"
#include "./epoll.inl"
#include "./activer.inl"


//////////////////////////////////////////////////////////////////////////
// 
int _initBuckyMessagePump(_BuckyMessagePump* pump) {
    
    pump->epoll = _epollCreate(64);
    if (pump->epoll < 0) {
        return BFX_RESULT_FAILED;
    }

    int ret = _initActiver(&pump->activer, pump->epoll);
    if (ret != 0) {
        return ret;
    }

    pump->duringActive = 0;

    return 0;
}

int _uninitBuckyMessagePump(_BuckyMessagePump* pump) {
    BLOG_CHECK(pump->epoll > 0);

    _uninitActiver(pump->activer, pump->epoll);

    _epollClose(pump->epoll);
    pump->epoll = NULL;

    return 0;
}

int _buckyMessagePumpBindFile(_BuckyMessagePump* pump, _BuckyMessagePumpIOFile* file) {
    BLOG_CHECK(pump->epoll > 0);
    BLOG_CHECK(file->fd >= 0);
    BLOG_CHECK(file->events != 0);

    return  _epollAdd(pump->epoll, file->events, file);
}

int _buckyMessagePumpUnbindFile(_BuckyMessagePump* pump, _BuckyMessagePumpIOFile* file) {
    BLOG_CHECK(pump->epoll > 0);
    BLOG_CHECK(file->fd >= 0);

    return _epollRemove(pump->epoll, file->fd);
}

static void _handleActiveRequest(_BuckyMessagePump* pump) {
    assert(pump->duringActive == 1);
    BfxAtomicExchange32(&pump->duringActive, 0);

    // 需要清空投递的唤醒字节
    _handleActive(&pump->activer);

    // 须最后触发事件
    if (pump->onProxyActive) {
        BLOG_CHECK(pump->loop);
        pump->onProxyActive(pump->loop);
    }
}

// 线程安全
int _buckyMessagePumpActive(_BuckyMessagePump* pump) {

    // 一个唤醒周期内，多次唤醒只需要触发一次
    int cur = BfxAtomicCompareAndSwap32(&pump->duringActive, 0, 1);
    if (cur != 0) {
        return 0;
    }

    BLOG_CHECK(pump->epoll > 0);

    return _activerActive(&pump->activer);
}


int _buckyMessagePumpPoll(_BuckyMessagePump* pump, int64_t timeout) {

    struct epoll_event events[256];
    int ret = _epollWait(events, BFX_ARRAYSIZE(events), timeout);
    if (ret <= 0) {
        return ret;
    }

    for (int index = 0; index < ret; ++index) {
        const struct epoll_event& event = events[index];
        void* ptr = event.data.ptr;
        if (ptr != &pump->activer) {

            _BuckyMessagePumpIOFile* file = (_BuckyMessagePumpIOFile*)ptr;
            if (file->onIOEvent) {
                file->onIOEvent(file, event.events)
            }

        } else {
            BLOG_CHECK(event.events == EPOLLIN);
            _handleActiveRequest()
        }
    }

    return ret;
}