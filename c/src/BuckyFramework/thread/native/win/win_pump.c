#include <BuckyBase.h>
#include "../message_pump.h"


int _initBuckyMessagePump(_BuckyMessagePump* pump) {
    pump->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    BLOG_CHECK(pump->iocp != NULL);
    if (pump->iocp == NULL) {
        return BFX_RESULT_FAILED;
    }

    pump->duringActive = 0;

    return 0;
}

int _uninitBuckyMessagePump(_BuckyMessagePump* pump) {
    BLOG_CHECK(pump->iocp);
    CloseHandle(pump->iocp);
    pump->iocp = NULL;

    return 0;
}

int _buckyMessagePumpBindFile(_BuckyMessagePump* pump, HANDLE obj, void* key) {
    BLOG_CHECK(obj);
    BLOG_CHECK(pump->iocp);

    HANDLE result = CreateIoCompletionPort(obj, pump->iocp, (ULONG_PTR)key, 1);
    if (result == NULL) {
        BLOG_ERROR("add obj to IOCP failed!! handle=%p", obj);
        return BFX_RESULT_FAILED;
    }

    assert(result == pump->iocp);

    return 0;
}

static void _handleActiveRequest(_BuckyMessagePump* pump) {
    BLOG_CHECK(pump->duringActive == 1);
    BfxAtomicExchange32(&pump->duringActive, 0);

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

    BLOG_CHECK(pump->iocp);

    // 投递一个特殊的唤醒任务
    BOOL ret = PostQueuedCompletionStatus(pump->iocp, 0, 0, (LPOVERLAPPED)pump);
    if (!ret) {
        BfxAtomicExchange32(&pump->duringActive, 0);
        BLOG_ERROR("active iocp thread failed! error=%d", GetLastError());

        return BFX_RESULT_FAILED;
    }

    return 0;
}

static void _handleIORequest(_BuckyMessagePumpIORequest* req) {
    if (req->onIOComplete) {
        req->onIOComplete(req);
    }
}

int _buckyMessagePumpPoll(_BuckyMessagePump* pump, int64_t timeout) {

    // 计算等待超时时长
    DWORD during ;
    if (timeout >= 0) {
        during = (DWORD)(timeout / 1000);
        if (timeout % 1000) {
            ++during;
        }
    } else if (timeout < 0) {
        during = INFINITE;
    }

    int dealCount = 0;

    ULONG count= 0;
    OVERLAPPED_ENTRY overlappeds[128];
    BOOL ret = GetQueuedCompletionStatusEx(pump->iocp, 
        overlappeds, 
        BFX_ARRAYSIZE(overlappeds),
        &count,
        during,
        FALSE);
    if (ret) {
        
        for (ULONG i = 0; i < count; i++) {
            const LPOVERLAPPED overlapped = overlappeds[i].lpOverlapped;
            if (overlapped) {
                if (overlapped == (void*)pump) {
                    _handleActiveRequest(pump);
                } else {
                    _BuckyMessagePumpIORequest* req = CONTAINING_RECORD(overlappeds[i].lpOverlapped, _BuckyMessagePumpIORequest, overlapped);
                    _handleIORequest(req);

                    ++dealCount;
                }
            } else {
                assert(false);
            }
        }
    } else {
        int err = GetLastError();
        if (err == WAIT_TIMEOUT) {
            // 等待超时，直接返回
        } else if (err == ERROR_ABANDONED_WAIT_0) {
            // If a call to GetQueuedCompletionStatusEx fails because the handle associated with it is closed, 
            // the function returns FALSE and GetLastError will return ERROR_ABANDONED_WAIT_0.

            // 关联的句柄被关闭但没有从IOCP移除
            BLOG_ERROR("iocp error! some handle been closed before remove from iocp!");
        } else {
            BLOG_ERROR("iocp error! err=%d", err);
            dealCount = -1;
        }
    }

    return dealCount;
}