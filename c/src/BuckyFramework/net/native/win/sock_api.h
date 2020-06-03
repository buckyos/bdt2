#pragma once

#include <mswsock.h>
#include <win/winapi.h>

typedef struct {

    LPFN_ACCEPTEX acceptEx;
    LPFN_CONNECTEX connectEx;
    LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrs;

    LPFN_ACCEPTEX acceptEx6;
    LPFN_CONNECTEX connectEx6;
    LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrs6;

}_WinSocketAPI;

// 使用win socket进程需要的必要初始化
void _initSocketProcess(_WinSocketAPI* api);
void _uninitSocketProcess(_WinSocketAPI* api);

int _nativeTranslateErr(int err) {
    if (err == 0) {
        return 0;
    }

    // TODO 完善

    int ret = err;
    switch (err) {
    case WSAEWOULDBLOCK:
        ret = BFX_RESULT_PENDING;
        break;
    default:

    }
    return err;
}


// NTSTATUS -> errCode
int _winSocketStatusTranslateToError(NTSTATUS status);

// sock apis which return BOOL -> errCode
int _winSocketDealWithBOOLResult(BOOL ret);