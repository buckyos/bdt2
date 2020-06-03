#pragma once

typedef struct _BuckyMessageLoop _BuckyMessageLoop;

#define _BUCKY_MESSAGE_PUMP_COMMON_FILEDS \
_BuckyMessageLoop* loop;    \
/* active事件，多次调用至少会触发一次*/ \
void (*onProxyActive)(_BuckyMessageLoop* loop); \
/* 是否处于active周期 */ \
volatile int32_t duringActive; \

#ifdef BFX_OS_WIN
typedef struct {
    _BUCKY_MESSAGE_PUMP_COMMON_FILEDS

    // 该线程持有的完成端口
    HANDLE iocp;
}_BuckyMessagePump;

typedef struct _BuckyMessagePumpIORequest {
    OVERLAPPED overlapped;

    void (*onIOComplete)(struct _BuckyMessagePumpIORequest* request);
    size_t bytesTransfered;

}_BuckyMessagePumpIORequest;

#else // POSIX

#include "./activer.h"

typedef struct {
    _BUCKY_MESSAGE_PUMP_COMMON_FILEDS;

    // 该线程持有的epoll
    int epoll;
    _BuckyMessagePumpActiver activer;
}_BuckyMessagePump;

typedef struct _BuckyMessagePumpIOFile {
    typedef void (*onIOEvent)(struct _BuckyMessagePumpIOFile* file, int events);
    int fd;
    int events;

}_BuckyMessagePumpIOFile;

#endif // POSIX


int _initBuckyMessagePump(_BuckyMessagePump* pump);
int _uninitBuckyMessagePump(_BuckyMessagePump* pump);

// 唤醒线程，该函数是线程安全的
int _buckyMessagePumpActive(_BuckyMessagePump* pump);

// 核心操作，检查线程上的pending事件并处理
int _buckyMessagePumpPoll(_BuckyMessagePump* pump, int64_t timeout);

#ifdef BFX_OS_WIN
int _buckyMessagePumpBindFile(_BuckyMessagePump* pump, HANDLE obj, void* key);
#else // POSIX
int _buckyMessagePumpBindFile(_BuckyMessagePump* pump, _BuckyMessagePumpIOFile* file);
int _buckyMessagePumpUnbindFile(_BuckyMessagePump* pump, _BuckyMessagePumpIOFile* file);
#endif // POSIX