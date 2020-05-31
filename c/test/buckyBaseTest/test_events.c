#include "../../src/BuckyBase/BuckyBase.h"

static void onAddEvent(void* udata, int a, int b, int* ret) {
    int r = (int)(intptr_t)udata;
    r += a + b;

    *ret = r;
}

// add事件触发器的参数
typedef struct {
    int a;
    int b;
    int result;
}_TestParams;

// add事件触发器，触发add事件时候被同步调用
static void addEmitter(void *udata, BfxEventListener listener, void* userData) {
    _TestParams* params = (_TestParams*)udata;

    // 这里需要保证注册的listener的签名规范符合add的要求
    listener(userData, params->a, params->b, &params->result);
}

void testSingleEvent() {
    BFX_SINGLE_EVENT_EMITTER emitter = BfxCreateSingleEventEmitter("add1");

    {
        BfxUserData udata = { .userData = (void*)(intptr_t)100 };
        BfxSingleEventEmitterOn(emitter, onAddEvent, udata);
    }

    {
        BfxUserData udata = { .userData = (void*)(intptr_t)110 };
        BfxSingleEventEmitterOn(emitter, onAddEvent, udata);
    }

    {
        _TestParams params = { .a = 1,.b = 2,.result = 0 };
        BfxSingleEventEmitterEmit(emitter, addEmitter, &params);
    }

    int32_t cookie;
    {
        BfxUserData udata = { .userData = (void*)(intptr_t)110 };
        cookie = BfxSingleEventEmitterOn(emitter, onAddEvent, udata);
        bool ret = BfxSingleEventEmitterOff(emitter, cookie);
        assert(ret);
    }

    {
        bool ret = BfxSingleEventEmitterOff(emitter, cookie);
        assert(!ret);
    }


    BfxSingleEventEmitterRemoveAllListeners(emitter);

    BfxDestroySingleEventEmitter(emitter);
}

void testEvents() {
    testSingleEvent();

    BFX_EVENT_EMITTER emitter = BfxCreateEventEmitter();
    BfxEventEmitterAddEvent(emitter, "add");
    BfxEventEmitterAddEvent(emitter, "sub");

    {
        BfxUserData udata = { .userData = (void*)(intptr_t)100 };
        BfxEventEmitterOn(emitter, "add", onAddEvent, udata);
    }

    {
        BfxUserData udata = { .userData = (void*)(intptr_t)110 };
        BfxEventEmitterOn(emitter, "add", onAddEvent, udata);
    }

    {
        _TestParams params = { .a = 1,.b = 2,.result = 0 };
        BfxEventEmitterEmit(emitter, "add", addEmitter, &params);
    }

    int32_t cookie;
    {
        BfxUserData udata = { .userData = (void*)(intptr_t)110 };
        cookie = BfxEventEmitterOn(emitter, "add", onAddEvent, udata);
        bool ret = BfxEventEmitterOff(emitter, "add", cookie);
        assert(ret);
    }

    {
        bool ret = BfxEventEmitterOff(emitter, "add", cookie);
        assert(!ret);
    }



    BfxEventEmitterRemoveAllListeners(emitter, "add");

    BfxDestroyEventEmitter(emitter);
}