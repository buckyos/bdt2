#include "../../src/BuckyBase/BuckyBase.h"

void testByteOrder() {
    int64_t v = INT64_C(0x1122334455667788);
    int64_t ret = BfxReverse64(v);
    int64_t ret2 = BFX_HOST_TO_BE_64(v);
    assert(ret2 == ret);

    ret = BFX_HOST_TO_LE_64(v);
    assert(ret == v);
    
}