#include "./send_pool.h"

static void _initSendPool(_TCPSendPool* pool, int32_t higtWaterMark) {

    pool->pendingBytes = 0;
    pool->pendingReqs = 0;
    pool->highWaterMark = 1024 * 1024 * 8;
    pool->drainWaterMark = pool->highWaterMark / 2;
    pool->needDrain = 0;
}

static void _uninitSendPool(_TCPSendPool* pool) {
    BLOG_CHECK(pool->pendingBytes == 0);
    BLOG_CHECK(pool->pendingReqs == 0);

    pool->pendingBytes = 0;
    pool->pendingReqs = 0;
    pool->needDrain = false;
}

static int _requireSend(_TCPSendPool* pool, size_t* len) {

    int ret = 0;
    for (;;) {
        int cur = pool->pendingBytes;
        if (cur >= pool->highWaterMark) {
            *len = 0;
            ret = BFX_RESULT_PENDING;
            break;
        }

        int next = cur + (int)* len;
        if (next > pool->highWaterMark) {
            *len = pool->highWaterMark - cur;
            next = pool->highWaterMark;

            ret = BFX_RESULT_PENDING;
        }

        int old = BfxAtomicCompareAndSwap32(&pool->pendingBytes, cur, next);
        if (old == cur) {
            BfxAtomicInc32(&pool->pendingReqs);
            break;
        }
    }

    if (ret == BFX_RESULT_PENDING) {
        pool->needDrain = 1;
    }

    return ret;
}


static int _releaseSend(_TCPSendPool* pool, size_t len) {
    BLOG_CHECK(len > 0);
    BLOG_CHECK(pool->pendingBytes >= len);

    BLOG_CHECK(pool->pendingReqs > 0);
    BfxAtomicDec32(&pool->pendingReqs);

    int ret = 0;
    BfxAtomicAdd32(&pool->pendingBytes, -(int32_t)len);

    if (pool->needDrain && pool->pendingBytes <= pool->drainWaterMark &&
        BfxAtomicCompareAndSwap32(&pool->needDrain, 1, 0) == 1) {

        ret = -1;
    }

    return ret;
}