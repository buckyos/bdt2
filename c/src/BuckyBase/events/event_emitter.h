#pragma once

BFX_DECLARE_HANDLE(BFX_EVENT_EMITTER);
BFX_DECLARE_HANDLE(BFX_SINGLE_EVENT_EMITTER);

// ÿ���¼������Ҽ���listener
#define BFX_EVENTEMITTER_MAX_LISTENERS 16

// eventname����󳤶�
#define BFX_EVENTEMITTER_EVENTNAME_MAX_LENGTH 32

typedef void (*BfxEventListener)(void* udata, ...);
typedef void (*BfxEventEmitter)(void* udata, BfxEventListener listener, void* listenerUserData);

// ���¼�������
BFX_API(BFX_SINGLE_EVENT_EMITTER) BfxCreateSingleEventEmitter(const char* name);
BFX_API(void) BfxDestroySingleEventEmitter(BFX_SINGLE_EVENT_EMITTER eventEmitter);

BFX_API(int32_t) BfxSingleEventEmitterOn(BFX_SINGLE_EVENT_EMITTER eventEmitter, BfxEventListener cb, BfxUserData udata);
BFX_API(bool) BfxSingleEventEmitterOff(BFX_SINGLE_EVENT_EMITTER eventEmitter, int32_t cookie);
BFX_API(bool) BfxSingleEventEmitterRemoveAllListeners(BFX_SINGLE_EVENT_EMITTER eventEmitter);

BFX_API(void) BfxSingleEventEmitterEmit(BFX_SINGLE_EVENT_EMITTER eventEmitter, BfxEventEmitter emitter, void* udata);


// ������node��EventEmiiter���¼������������Թ������¼�
BFX_API(BFX_EVENT_EMITTER) BfxCreateEventEmitter();
BFX_API(void) BfxDestroyEventEmitter(BFX_EVENT_EMITTER eventEmitter);

BFX_API(bool) BfxEventEmitterAddEvent(BFX_EVENT_EMITTER eventEmitter, const char* name);
BFX_API(bool) BfxEventEmitterRemoveEvent(BFX_EVENT_EMITTER eventEmitter, const char* name);

BFX_API(int32_t) BfxEventEmitterOn(BFX_EVENT_EMITTER eventEmitter, const char* name, BfxEventListener cb, BfxUserData udata);
BFX_API(bool) BfxEventEmitterOff(BFX_EVENT_EMITTER eventEmitter, const char* name, int32_t cookie);
BFX_API(bool) BfxEventEmitterRemoveAllListeners(BFX_EVENT_EMITTER eventEmitter, const char* name);

// ͬ�������¼�
BFX_API(void) BfxEventEmitterEmit(BFX_EVENT_EMITTER eventEmitter, const char* name, BfxEventEmitter emitter, void* udata);