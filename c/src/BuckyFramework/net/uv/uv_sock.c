#include "./uv_sock.h"

void _initSocketCommon(_UVSocketCommon* common, _BuckyFramework* framework, BfxUserData udata) {
    common->framework = framework;
    common->ref = 1;
    common->status = 0;

    common->thread = BfxGetCurrentThread();
    assert(common->thread);

    common->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }
}

void _uninitSocketCommon(_UVSocketCommon* common) {
    assert(common->ref == 0);

    if (common->udata.lpfnReleaseUserData) {
        common->udata.lpfnReleaseUserData(common->udata.userData);
    }

    BfxThreadRelease(common->thread);
    common->thread = NULL;

    common->framework = NULL;
}

void _uvSocketInitUserData(_UVSocketCommon* socket, BfxUserData udata) {
    assert(socket->udata.userData == NULL);

    socket->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }
}

void _uvSocketGetUserData(_UVSocketCommon* socket, BfxUserData* udata) {

    *udata = socket->udata;
    if (socket->udata.lpfnAddRefUserData) {
        socket->udata.lpfnAddRefUserData(socket->udata.userData);
    }
}

