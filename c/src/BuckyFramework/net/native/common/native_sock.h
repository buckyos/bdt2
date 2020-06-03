#pragma once

enum {
    _NativeSocketState_Init = 0,
    _NativeSocketState_Bind = 1,
    _NativeSocketState_Listen = 2,
    _NativeSocketState_Connecting = 3,
    _NativeSocketState_Connected = 4,
    _NativeSocketState_Closed = 5,
} _NativeSocketState;

#define NATIVE_SOCKET_COMMON                \
struct _BuckyFramework* framework;          \
    volatile int32_t ref;                   \
    volatile int32_t state;                 \
    BfxUserData udata;                      \
    BFX_THREAD_HANDLE thread;               \
    BuckyNativeSocketType socket;           \

typedef struct {
    NATIVE_SOCKET_COMMON
} _NativeSocketCommon;

bool _nativeSocketIsClosed(_NativeSocketCommon* sock) {
    return sock->state == _NativeSocketState_Closed;
}

bool _nativeSocketChangeState(_NativeSocketCommon* sock, int state, int oldState) {
    return BfxAtomicCompareAndSwap32(&sock->state, oldState, state) == oldState;
}

void _initSocketCommon(_NativeSocketCommon* common, _BuckyFramework* framework, BfxUserData udata);
void _uninitSocketCommon(_NativeSocketCommon* common);
