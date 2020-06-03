#pragma once

#include "../../BuckyBase/BuckyBase.h"

typedef int (*BfxThreadMain)(void*);

BFX_DECLARE_HANDLE(BFX_THREAD_HANDLE);

// 线程类型
typedef enum {
    BfxThreadType_Native = 0,
    BfxThreadType_IO = 1,
} BfxThreadType;

typedef struct {
    char name[32];
    size_t stackSize;

    size_t ssize;
} BfxThreadOptions;

BFX_API(int32_t) BfxThreadAddRef(BFX_THREAD_HANDLE thandle);
BFX_API(int32_t) BfxThreadRelease(BFX_THREAD_HANDLE thandle);

// thandle是不是当前线程
BFX_API(bool) BfxIsCurrentThread(BFX_THREAD_HANDLE thandle);

// 如果thandle不为空，那么返回句柄，外部使用完毕需要释放
BFX_API(int) BfxCreateThread(BfxThreadMain main, void* args, BfxThreadOptions options, BFX_THREAD_HANDLE* thandle);
BFX_API(int) BfxCreateIOThread(BfxThreadOptions options, BFX_THREAD_HANDLE* thandle);

typedef void (*BfxOnMessage)(void* lpUserData);

// 获取当先线程的句柄，不为NULL的话需要释放
BFX_API(BFX_THREAD_HANDLE) BfxGetCurrentThread();

BFX_API(int) BfxThreadJoin(BFX_THREAD_HANDLE thandle);

// 发送消息到指定线程
// thandle=NULL，则发送到当前线程
BFX_API(int) BfxPostMessage(BFX_THREAD_HANDLE thandle, BfxOnMessage onMessage, BfxUserData userData);

BFX_API(int) BfxPostQuit(BFX_THREAD_HANDLE thandle, int code);

// 注册当前线程的退出回调，退出时候会按照反序依次调用
typedef void (*BfxThreadExitCallBack)(void* userData);
BFX_API(int) BfxThreadAtExit(BfxThreadExitCallBack proc, BfxUserData userData);

//#ifdef BFX_FRAMEWORK_UV

BFX_API(uv_loop_t*) BfxThreadGetCurrentLoop();


//////////////////////////////////////////////////////////////////////////
// timer

BFX_DECLARE_HANDLE(BFX_TIMER_HANDLE);

typedef void (*BfxTimerCallBack)(void* userData);

BFX_API(BFX_TIMER_HANDLE) BfxSetImmediate(BfxTimerCallBack cb, BfxUserData userData);
BFX_API(BFX_TIMER_HANDLE) BfxSetInterval(int64_t delayInMicroseconds, BfxTimerCallBack cb, BfxUserData userData);
BFX_API(BFX_TIMER_HANDLE) BfxSetTimeout(int64_t delayInMicroseconds, BfxTimerCallBack cb, BfxUserData userData);

// 同一个timer只可以clear一次！
// BfxSetImmediate和BfxSetTimeout触发后不可以再调用Clear
// 这里为了简化逻辑，没有使用引用计数的handle
BFX_API(int) BfxClearTimer(BFX_TIMER_HANDLE timer);