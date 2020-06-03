#include "../src/BuckyFramework/framework.h"


BFX_TIMER_HANDLE gTimer = NULL;
static void onTimer(void* args) {
    BfxClearTimer(gTimer);

    BfxPostQuit(NULL, 100);
}

static void atExit(void* args) {
    BLOG_INFO("thread exit!");
}

static void _onMessage(void* lpUserData) {
    BfxUserData ud = { 0 };
    gTimer = BfxSetInterval(1000 * 1000, onTimer, ud);

    BfxThreadAtExit(atExit, ud);

    BLOG_INFO("thread running! %d", 100);
}

void testThread() {
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