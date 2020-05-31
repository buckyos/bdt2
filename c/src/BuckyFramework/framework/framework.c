#include "../framework.h"
#include "../internal.h"
#include "../net/net_framework.h"
#include "./thread_pool.h"
#include "./timer_manager.h"


static _BuckyFramework* _newBuckyFramework() {
    _BuckyFramework* framework = (_BuckyFramework*)BfxMalloc(sizeof(_BuckyFramework));
    memset(framework, 0, sizeof(_BuckyFramework));

    framework->state = 0;

    return framework;
}

static void _uninitBuckyFramework(_BuckyFramework* framework) {
    _uninitThreadContainer(&framework->threadPool);
}

static void _deleteBuckyFramework(_BuckyFramework* framework) {

    int ret = BfxAtomicCompareAndSwap32(&framework->state, 0, 1);
    if (ret == 0) {
        _uninitBuckyFramework(framework);
    }

    BfxFree(framework);
}

static void BFX_STDCALL _destroySelf(struct BdtSystemFramework* framework) {
    _BuckyFramework* bframe = (_BuckyFramework*)framework;

    _deleteBuckyFramework(bframe);
}

BFX_API(BdtSystemFramework*) createBuckyFramework(const BdtSystemFrameworkEvents* events, const BuckyFrameworkOptions* options) {
    _BuckyFramework* framework = _newBuckyFramework();
    framework->events = *events;

    framework->base.destroySelf = _destroySelf;

    _fillNetFramework(framework);

    _fillTimerFramework(framework);

    _initThreadContainer(&framework->threadPool, options);

    _initPackageDispatchManager(framework, &framework->dispatchManager, options);

    return (BdtSystemFramework*)framework;
}