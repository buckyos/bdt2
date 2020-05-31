#include "../../src/BuckyBase/BuckyBase.h"

void testRandom() {

    uint8_t* data;
    int n = sizeof(*data);

    int32_t v = BfxRandomRange32(0, 1000);

    int64_t v6 = BfxRandom64(100);
}