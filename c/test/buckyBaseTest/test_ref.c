#include "../../src/BuckyBase/BuckyBase.h"

typedef struct {
    BFX_DECLARE_MULTITHREAD_REF_MEMBER;
    int count;
}_TestRef;

static _TestRef* _newTestRef() {
    _TestRef* obj = (_TestRef*)BfxMalloc(sizeof(_TestRef));
    obj->ref = 1;
    obj->count = 1;

    return obj;
}

static void _deleteTestRef(_TestRef* obj) {
    assert(obj->ref == 0);

    obj->count = 0;
    BfxFree(obj);
}

BFX_DECLARE_MULTITHREAD_REF_FUNCTION(_TestRef, _deleteTestRef);

void testRef() {

    _TestRef* obj =  _newTestRef();

    _TestRefAddRef(obj);
    _TestRefRelease(obj);
    _TestRefRelease(obj);
}




    