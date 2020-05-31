#pragma once

#include "../../BuckyBase/BuckyBase.h"

typedef int (*BfxThreadMain)(void*);

BFX_DECLARE_HANDLE(BFX_THREAD_HANDLE);

// �߳�����
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

// thandle�ǲ��ǵ�ǰ�߳�
BFX_API(bool) BfxIsCurrentThread(BFX_THREAD_HANDLE thandle);

// ���thandle��Ϊ�գ���ô���ؾ�����ⲿʹ�������Ҫ�ͷ�
BFX_API(int) BfxCreateThread(BfxThreadMain main, void* args, BfxThreadOptions options, BFX_THREAD_HANDLE* thandle);
BFX_API(int) BfxCreateIOThread(BfxThreadOptions options, BFX_THREAD_HANDLE* thandle);

typedef void (*BfxOnMessage)(void* lpUserData);

// ��ȡ�����̵߳ľ������ΪNULL�Ļ���Ҫ�ͷ�
BFX_API(BFX_THREAD_HANDLE) BfxGetCurrentThread();

BFX_API(int) BfxThreadJoin(BFX_THREAD_HANDLE thandle);

// ������Ϣ��ָ���߳�
// thandle=NULL�����͵���ǰ�߳�
BFX_API(int) BfxPostMessage(BFX_THREAD_HANDLE thandle, BfxOnMessage onMessage, BfxUserData userData);

BFX_API(int) BfxPostQuit(BFX_THREAD_HANDLE thandle, int code);

// ע�ᵱǰ�̵߳��˳��ص����˳�ʱ��ᰴ�շ������ε���
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

// ͬһ��timerֻ����clearһ�Σ�
// BfxSetImmediate��BfxSetTimeout�����󲻿����ٵ���Clear
// ����Ϊ�˼��߼���û��ʹ�����ü�����handle
BFX_API(int) BfxClearTimer(BFX_TIMER_HANDLE timer);