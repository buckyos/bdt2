#pragma once

typedef struct {
    volatile int32_t pendingReqs;
    volatile int32_t pendingBytes;
    int32_t highWaterMark;
    int32_t drainWaterMark;

	// 是否需要触发drain事件
    volatile int32_t needDrain;

}_TCPSendPool;

static void _initSendPool(_TCPSendPool* pool, int32_t higtWaterMark);
static void _uninitSendPool(_TCPSendPool* pool);

static int _requireSend(_TCPSendPool* pool, size_t* len);

// return -1表示需要触发drain事件
static int _releaseSend(_TCPSendPool* pool, size_t len);
