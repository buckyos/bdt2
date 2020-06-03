#include "../src/BuckyFramework/framework.h"


static BFX_THREAD_LOCAL void* s_buffer;

static void _onMessage(void* lpUserData) {

    void* data = BfxThreadBufferMallocStatic(&s_buffer, 100);

    void *data2 = BfxThreadBufferMallocStatic(&s_buffer, 32);

    BfxThreadBufferFreeStatic(&s_buffer, data2);
    BfxThreadBufferFreeStatic(&s_buffer, data);

    BfxPostQuit(NULL, 0);

    BLOG_INFO("thread running! %d", 100);
}

void testThreadBuffer() {
    void* data = BfxThreadBufferMallocStatic(&s_buffer, 32);
    memset(data, 0, 32);

    BfxThreadOptions opt = {
        .name = "test1",
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    BFX_THREAD_HANDLE thandle;
    BfxCreateIOThread(opt, &thandle);

    BfxUserData ud = { 0 };
    BfxPostMessage(thandle, _onMessage, ud);

    BfxThreadJoin(thandle);
}

static BfxTlsKey s_buffer2;

static void _onMessage2(void* lpUserData) {

    void* data = BfxThreadBufferMallocDynamic(&s_buffer2, 100);

    void* data2 = BfxThreadBufferMallocDynamic(&s_buffer2, 32);

    BfxThreadBufferFreeDynamic(&s_buffer2, data2);
    BfxThreadBufferFreeDynamic(&s_buffer2, data);

    BfxPostQuit(NULL, 0);

    BLOG_INFO("thread2 running! %d", 100);
}

void testThreadBuffer2() {
    BfxTlsCreate(&s_buffer2);

    void* data = BfxThreadBufferMallocDynamic(&s_buffer2, 32);
    memset(data, 0, 32);

    BfxThreadOptions opt = {
        .name = "test1",
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    BFX_THREAD_HANDLE thandle;
    BfxCreateIOThread(opt, &thandle);

    BfxUserData ud = { 0 };
    BfxPostMessage(thandle, _onMessage2, ud);

    BfxThreadJoin(thandle);

    BfxTlsDelete(&s_buffer2);
}