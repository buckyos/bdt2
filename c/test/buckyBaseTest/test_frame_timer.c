#include <BdtCore.h>
#include "../../src/BuckyFramework/framework.h"

static uv_sem_t sig;

static volatile int32_t g_total = 0;

typedef struct BdtStack {
    BdtSystemFramework* framework;
} BdtStack;

typedef struct {
    BFX_DECLARE_MULTITHREAD_REF_MEMBER;
    BdtSystemFramework* framework;
    char* name;
} _TimerData;

static _TimerData* _newTimerData(BdtSystemFramework* framework, int index) {
    char* name = BfxMalloc(32);
    snprintf(name, 32, "timer: %d\n", index);

    _TimerData* data = (_TimerData*)BfxMalloc(sizeof(_TimerData));
    data->name = name;
    data->framework = framework;
    data->ref = 1;

    return data;
}

static void _deleteTimerData(_TimerData* data) {

    BLOG_INFO("release timer data, name=%s", data->name);

    BfxFree(data->name);
    data->name = NULL;
    BfxFree(data);
}

BFX_DECLARE_MULTITHREAD_REF_FUNCTION(_TimerData, _deleteTimerData);

static void BFX_STDCALL _bdtSystemFrameworkTimeoutEvent(BDT_SYSTEM_TIMER timer, void* userData) {
    _TimerData* data = (_TimerData*)userData;

    BLOG_INFO("ontimer, name=%s", data->name);
    data->framework->destroyTimeout(timer);

    int ret = BfxAtomicDec32(&g_total);
    if (ret == 0) {
        uv_sem_post(&sig);
    }
}

static void BFX_STDCALL _bdtSystemFrameworkAsyncEvent(void* userData) {
    _TimerData* data = (_TimerData*)userData;

    BLOG_INFO("onasync, name=%s", data->name);

    int ret = BfxAtomicDec32(&g_total);
    if (ret == 0) {
        uv_sem_post(&sig);
    }
}

static void BFX_STDCALL addRefTimerUserData(void* userData) {
    _TimerDataAddRef((_TimerData*)userData);
}

static void BFX_STDCALL releaseTimerUserData(void* userData) {
    _TimerDataRelease((_TimerData*)userData);
}

static BDT_SYSTEM_TIMER _startOneTimer(BdtSystemFramework* framework, int index) {
    
    _TimerData* data = _newTimerData(framework, index);

    const BfxUserData udata = { 
        .userData = data, 
        .lpfnAddRefUserData = addRefTimerUserData,
        .lpfnReleaseUserData = releaseTimerUserData
    };

    BDT_SYSTEM_TIMER handle = framework->createTimeout(framework, _bdtSystemFrameworkTimeoutEvent, &udata, 0);
    assert(handle);

    _TimerDataRelease(data);

    return handle;
}

static void _testAsync(BdtSystemFramework* framework, int index) {

    _TimerData* data = _newTimerData(framework, index);

    const BfxUserData udata = {
        .userData = data,
        .lpfnAddRefUserData = addRefTimerUserData,
        .lpfnReleaseUserData = releaseTimerUserData
    };

    framework->immediate(framework, _bdtSystemFrameworkAsyncEvent, &udata);
}

static void startTimer(BdtSystemFramework* framework) {
    
    BfxAtomicAdd32(&g_total, 50);
    for (int i = 0; i < 100; ++i) {
        BDT_SYSTEM_TIMER timer = _startOneTimer(framework, i);
        if (i % 2) {
            framework->destroyTimeout(timer);
        }
    }
}

static void startAsync(BdtSystemFramework* framework) {

    BfxAtomicAdd32(&g_total, 100);

    for (int i = 0; i < 100; ++i) {
        _testAsync(framework, i);
    }
}

void testFrameworkTimer() {
    BuckyFrameworkOptions options = {
        .size = sizeof(BuckyFrameworkOptions),
        .tcpThreadCount = 4,
        .udpThreadCount = 4,
        .timerThreadCount = 4,
        .dispatchThreadCount = 4
    };

    BdtSystemFrameworkEvents events = {
        .bdtPushTcpSocketEvent = NULL,
        .bdtPushTcpSocketData = NULL,
        .bdtPushUdpSocketData = NULL,
    };

    // 为了测试需要，构造一个假的BdtStack
    BdtStack* dummy = (BdtStack*)BfxMalloc(sizeof(BdtStack));
    BdtSystemFramework* framework = createBuckyFramework(&events, &options);
    framework->stack = dummy;
    dummy->framework = framework;

    uv_sem_init(&sig, 0);

    startTimer(framework);

    startAsync(framework);

    uv_sem_wait(&sig);
}
