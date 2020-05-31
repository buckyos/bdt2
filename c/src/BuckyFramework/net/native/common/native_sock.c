#include "./native_sock.h"

void _initSocketCommon(_NativeSocketCommon* common, _BuckyFramework* framework, BfxUserData udata) {
    common->framework = framework;
    common->ref = 1;
    common->status = 0;
    common->socket = BFX_NATIVE_SOCKET_INVALID_HANDLE;

    common->thread = BfxGetCurrentThread();
    assert(common->thread);

    common->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }
}

_uninitSocketCommon(_NativeSocketCommon* common) {
    assert(common->ref == 0);

    if (common->udata.lpfnReleaseUserData) {
        common->udata.lpfnReleaseUserData(common->udata.userData);
    }

    BfxThreadRelease(common->thread);
    common->thread = NULL;

    common->framework = NULL;
}