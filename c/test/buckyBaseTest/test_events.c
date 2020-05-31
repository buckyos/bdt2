#include "../../src/BuckyBase/BuckyBase.h"

static void onAddEvent(void* udata, int a, int b, int* ret) {
    int r = (int)(intptr_t)udata;
    r += a + b;

    *ret = r;
}

// add�¼��������Ĳ���
typedef struct {
    int a;
    int b;
    int result;
}_TestParams;

// add�¼�������������add�¼�ʱ��ͬ������
static void addEmitter(void *udata, BfxEventListener listener, void* userData) {
    _TestParams* params = (_TestParams*)udata;

    // ������Ҫ��֤ע���listener��ǩ���淶����add��Ҫ��
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