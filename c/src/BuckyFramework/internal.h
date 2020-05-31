#pragma once

#include "./net/uv/uv_sock.h"
#include "./framework/thread_pool.h"
#include "./framework/thread_dispatch.h"


#if defined(BFX_FRAMEWORK_NATIVE)
#if defined(BFX_OS_WIN)
#define BFX_FRAMEWORK_NATIVE_WIN    1
#else // posix
#define BFX_FRAMEWORK_NATIVE_POSIX    1
#endif // posix
#endif // BFX_FRAMEWORK_NATIVE

/*
三个核心宏
BFX_FRAMEWORK_NATIVE_WIN        基于win平台的原始实现
BFX_FRAMEWORK_NATIVE_POSIX      基于posix平台的原始实现
BFX_FRAMEWORK_UV                基于libuv的次级全平台实现
*/

#if defined(BFX_FRAMEWORK_NATIVE_WIN)
#include "./net/native/win/sock_api.h"
#endif // BFX_FRAMEWORK_NATIVE_WIN

typedef struct _BuckyFramework {
    BdtSystemFramework base;

    BdtSystemFrameworkEvents events;

	// 状态
    volatile int state;

    _UVTCPSocketEvents tcpEvents;
    _UVUDPSocketEvents udpEvents;

    _BuckyThreadContainer threadPool;

    _PackageDispatchManager dispatchManager;

#if defined(BFX_FRAMEWORK_NATIVE_WIN)
    _WinSocketAPI api;
#endif // BFX_FRAMEWORK_NATIVE_WIN

}_BuckyFramework;