#include <uv.h>
#include <heap-inl.h>
#include <assert.h>
#include <limits.h>

#include "./loop_timer.h"
#include "../message_loop.h"


static int _timerCompare(const struct heap_node* left,
    const struct heap_node* right) {
    const _BuckyLoopTimer* a = (_BuckyLoopTimer*)left;
    const _BuckyLoopTimer* b = (_BuckyLoopTimer*)right;

    if (a->timeout < b->timeout) {
        return 1;
    }
    if (b->timeout < a->timeout) {
        return 0;
    }

    if (a->index < b->index) {
        return 1;
    }
     
    return 0;
}

static void _initBuckyLoopTimerManager(_BuckyMessageLoop* loop, _BuckyLoopTimerManager* tm) {
    tm->loop = loop;
    tm->index = 0;

    heap_init((struct heap*)&tm->timers);
}

static void _uninitBuckyLoopTimerManager(_BuckyLoopTimerManager* tm) {
    BLOG_CHECK(tm->loop);
    tm->loop = NULL;
    tm->index = 0;
}

void _initBuckyTimer(_BuckyMessageLoop* loop, _BuckyLoopTimer* timer) {
    memset(timer, 0, sizeof(_BuckyLoopTimer));

    timer->loop = loop;
    timer->cb = NULL;
    timer->index = 0;
    timer->timeout = 0;
    timer->once = true;
    timer->state = _BuckyLoopTimerState_Init;
}

void _uninitBuckyTimer(_BuckyLoopTimer* timer) {
    BLOG_CHECK(timer->state == _BuckyLoopTimerState_Stoped);
    timer->loop = NULL;
}

int _startBuckyTimer(_BuckyLoopTimer* timer, _buckyLoopTimerCallback cb, int64_t timeout, bool once) {
    assert(cb);
    assert(timeout >= 0);
    if (timer->state != _BuckyLoopTimerState_Init) {
        assert(false);
        return BFX_RESULT_INVALID_STATE;
    }

    _BuckyLoopTimerManager* tm = &timer->loop->timerManager;
    timer->cb = cb;
    timer->timeout = timer->loop->time + timeout;
    timer->once = once;
    timer->index = tm->index ++;
    timer->state = _BuckyLoopTimerState_Start;

    heap_insert((struct heap*)&tm->timers, (struct heap_node*)&timer->stub, _timerCompare);

    return 0;
}

int _stopBuckyTimer(_BuckyLoopTimer* timer) {
    assert(timer->loop);
    if (timer->state == _BuckyLoopTimerState_Stoped) {
        return 0;
    }

    // 必须start后才可以被stop
    BLOG_CHECK(timer->state == _BuckyLoopTimerState_Start)
    timer->state = _BuckyLoopTimerState_Stoped;

    _BuckyLoopTimerManager* tm = &timer->loop->timerManager;
    heap_remove((struct heap*)&tm->timers, (struct heap_node*)&timer->stub, _timerCompare);
   
    return 0;
}

static int64_t _buckyTimersNextTimeout(_BuckyLoopTimerManager* tm) {
    const _BuckyLoopTimer* firstTimer = (_BuckyLoopTimer*)heap_min((struct heap*)&tm->timers);
    if (firstTimer == NULL) {
        return -1;
    }

    if (firstTimer->timeout <= tm->loop->time) {
        return 0;
    }
        
    int64_t diff = firstTimer->timeout - tm->loop->time;
    return diff;
}

static int _buckyLoopTimerAgain(_BuckyLoopTimer* timer) {
    BLOG_CHECK(timer->state == _BuckyLoopTimerState_Stoped);
    timer->state = _BuckyLoopTimerState_Init;

    return _startBuckyTimer(timer, timer->cb, timer->timeout, false);
}

static int _buckyPollTimer(_BuckyLoopTimerManager* tm, int64_t* timeout) {
    int count = 0;
    *timeout = -1;
    for (;;) {
        _BuckyLoopTimer* firstTimer = (_BuckyLoopTimer*)heap_min((struct heap*)&tm->timers);
        if (firstTimer == NULL) {
            break;
        }

        if (firstTimer->timeout > tm->loop->time) {
            *timeout = firstTimer->timeout - tm->loop->time;
            break;
        }

        _stopBuckyTimer(firstTimer);
        if (!firstTimer->once) {
            _buckyLoopTimerAgain(firstTimer);
        }

        firstTimer->cb(firstTimer);
        ++count;
    }

    return count;
}