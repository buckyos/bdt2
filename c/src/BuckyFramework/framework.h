#pragma once

#include <BdtSystemFramework.h>
#include "./thread/bucky_thread.h"

typedef struct _BuckyFramework _BuckyFramework;

/*
����Ϊ0��ʹ�����õ�Ĭ��ֵ��
default {
tcpThreadCount: 4,
udpThreadCount: 4,
timerThreadCount: 2,
dispatchThreadCount: 4,
}
*/
typedef struct BuckyFrameworkOptions {
    size_t tcpThreadCount;
    size_t udpThreadCount;
    size_t timerThreadCount;
    size_t dispatchThreadCount;

    size_t size;
} BuckyFrameworkOptions;

BFX_API(BdtSystemFramework*) createBuckyFramework(const BdtSystemFrameworkEvents* events, const BuckyFrameworkOptions* options);

// ��ȡuserData��������ʹ�������Ҫ�ͷ�
BFX_API(void) buckyFrameworkSocketGetUserData(void* sock, BfxUserData* userData);


