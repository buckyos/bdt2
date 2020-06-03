#include "../framework.h"
#include "./timer_manager.h"
#include "./thread_pool.h"
#include "../internal.h"


typedef struct {
    BFX_DECLARE_MULTITHREAD_REF_MEMBER;

    void (*onTimeout)();
    BfxUserData udata;
    int32_t timeout;

    BFX_TIMER_HANDLE timer;
    BFX_THREAD_HANDLE thread;

    int type;
    volatile int state;
}_BuckyFrameworkTimer;


typedef enum {
    _BuckyFrameworkTimerState_INIT = 0,
    _BuckyFrameworkTimerState_CANCELED = 1,
    _BuckyFrameworkTimerState_FIRED = 2,
} _BuckyFrameworkTimerState;


typedef enum {
    _BuckyFrameworkTimerType_Timeout = 0,
    _BuckyFrameworkTimerType_Immediate = 1,
} _BuckyFrameworkTimerType;

static _BuckyFrameworkTimer* _newTimer(_BuckyFrameworkTimerType type, void (*onTimeout)(), BfxUserData udata, int32_t timeout) {
    _BuckyFrameworkTimer* timer = (_BuckyFrameworkTimer*)BfxMalloc(sizeof(_BuckyFrameworkTimer));
    memset(timer, 0, sizeof(_BuckyFrameworkTimer));

    timer->ref = 1;
    timer->type = type;
    timer->state = _BuckyFrameworkTimerState_INIT;
    timer->timeout = timeout;
    timer->onTimeout = onTimeout;
    timer->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }

    return timer;
}

static void _deleteTimer(_BuckyFrameworkTimer* timer) {
    assert(timer->timer == NULL);
    if (timer->thread) {
        BfxThreadRelease(timer->thread);
        timer->thread = NULL;
    }

    if (timer->udata.lpfnReleaseUserData) {
        timer->udata.lpfnReleaseUserData(timer->udata.userData);
    }

    BfxFree(timer);
}

BFX_DECLARE_MULTITHREAD_REF_FUNCTION(_BuckyFrameworkTimer, _deleteTimer);

static void _baseTimerCallback(void* udata) {
    _BuckyFrameworkTimer* timer = (_BuckyFrameworkTimer*)udata;
    assert(timer->timer);
    timer->timer = NULL;

    if (timer->state == _BuckyFrameworkTimerState_INIT) {
        timer->state = _BuckyFrameworkTimerState_FIRED;

        if (timer->type == _BuckyFrameworkTimerType_Timeout) {
            ((bdtSystemFrameworkTimeoutEvent)timer->onTimeout)((BDT_SYSTEM_TIMER)timer, timer->udata.userData);
        } else if (timer->type == _BuckyFrameworkTimerType_Immediate) {
            ((bdtSystemFrameworkImmediateEvent)timer->onTimeout)(timer->udata.userData);
        } else {
            assert(0);
        }
    }
    
    _BuckyFrameworkTimerRelease(timer);
}

// ��timerĿ���߳��������
static int _setTimerOnTargetThread(_BuckyFrameworkTimer* timer) {
    assert(timer->timer == NULL);

    int ret = 0;
    if (timer->state == _BuckyFrameworkTimerState_INIT) {
        BfxUserData udata = { .userData = timer };
        timer->timer = BfxSetTimeout((int64_t)timer->timeout * 1000, _baseTimerCallback, udata);
        if (timer->timer == NULL) {
            ret = BFX_RESULT_FAILED;
        }
    } else {
        // Ͷ�ݹ����б�cancel��
        ret = BFX_RESULT_CANCELED;
    }

    if (ret != 0) {
        // ����timerʧ��/��ȡ������ô������Ҫ�ͷ�
        _BuckyFrameworkTimerRelease(timer);
    } else {
        // ��timer�ص������ͷ�
    }

    return ret;
}

// ���������̵߳���
static _BuckyFrameworkTimer* _setTimer(_BuckyFrameworkTimerType type, _BuckyFramework* framework,  void (*cb)(),
    const BfxUserData* userData, int32_t timeout) {

    BFX_THREAD_HANDLE thread = _threadContainerSelectThread(&framework->threadPool, _BuckyThreadTarget_TIMER);
    assert(thread);
    if (thread == NULL) {
        return NULL;
    }

    _BuckyFrameworkTimer* timer = _newTimer(type, cb, *userData, timeout);
    timer->thread = thread;

    // ����timer֮ǰ����Ҫ����һ�μ���!
    _BuckyFrameworkTimerAddRef(timer);

    int ret;
    if (BfxIsCurrentThread(thread)) {
        ret = _setTimerOnTargetThread(timer);
    } else {
        BfxUserData udata = { .userData = timer };
        ret = BfxPostMessage(thread, (BfxOnMessage)_setTimerOnTargetThread, udata);
        if (ret != 0) {
            BLOG_ERROR("post setTimer failed! err=%d", ret);

            _BuckyFrameworkTimerRelease(timer);
        }
    }

    // ����ʧ�ܣ���Ҫ���timer
    if (ret != 0) {
        timer = NULL;
        _BuckyFrameworkTimerRelease(timer);
    }

    return timer;
}

// ��timerĿ���߳��������
static void _killTimerOnTargetThread(_BuckyFrameworkTimer* timer) {
    assert(timer->state == _BuckyFrameworkTimerState_CANCELED);

    if (timer->timer) {
        BFX_TIMER_HANDLE handle = timer->timer;
        timer->timer = NULL;

        BfxClearTimer(handle);
    }

    // ������Ҫ�ͷ�һ������
    _BuckyFrameworkTimerRelease(timer);
}

// �����������̵߳��ã�����ֻ�ܵ���һ�Σ���Ϊ���ͷ����ü���
static void _destroyTimer(_BuckyFrameworkTimer* timer) {
    int ret = BfxAtomicCompareAndSwap32(&timer->state, _BuckyFrameworkTimerState_INIT, _BuckyFrameworkTimerState_CANCELED);
    if (ret == 0) {
        assert(timer->thread);

        if (BfxIsCurrentThread(timer->thread)) {

            _killTimerOnTargetThread(timer);

        } else {

            BfxUserData udata = { .userData = timer };
            int ret = BfxPostMessage(timer->thread, (BfxOnMessage)_killTimerOnTargetThread, udata);
            if (ret != 0) {
                BLOG_ERROR("post killTimer failed! err=%d", ret);

                _BuckyFrameworkTimerRelease(timer);
                timer = NULL;
            } else {
                // �ص������ͷ�����
            }
        }
    } else {
        // �ظ�cancel������timer�Ѿ���������ôֻ��Ҫ�ͷ�����
        _BuckyFrameworkTimerRelease(timer);
    } 
}

//////////////////////////////////////////////////////////////////////////
// framework impl

static BDT_SYSTEM_TIMER BFX_STDCALL _createTimeout(struct BdtSystemFramework* framework,
    bdtSystemFrameworkTimeoutEvent cb,
    const BfxUserData* userData,
    int32_t timeout) {


    return (BDT_SYSTEM_TIMER)_setTimer(_BuckyFrameworkTimerType_Timeout, (_BuckyFramework*)framework, cb, userData, timeout);
}

static void BFX_STDCALL _destroyTimeout(BDT_SYSTEM_TIMER timer) {
    _destroyTimer((_BuckyFrameworkTimer*)timer);
}

static void BFX_STDCALL _immediate(struct BdtSystemFramework* framework, bdtSystemFrameworkImmediateEvent cb, const BfxUserData* userData) {
    _BuckyFrameworkTimer* timer = _setTimer(_BuckyFrameworkTimerType_Immediate, (_BuckyFramework*)framework, cb, userData, 0);
    if (timer) {
        _BuckyFrameworkTimerRelease(timer);
    }
}


void _fillTimerFramework(_BuckyFramework* framework) {
    BdtSystemFramework* bdtFrame = &framework->base;

    bdtFrame->createTimeout = _createTimeout;
    bdtFrame->destroyTimeout = _destroyTimeout;
    bdtFrame->immediate = _immediate;
}

