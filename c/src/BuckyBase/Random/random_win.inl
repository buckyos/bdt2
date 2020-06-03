#define _CRT_RAND_S
#include <stdlib.h>

#include "../BuckyBase.h"


static uint32_t _random32() {
    uint32_t value;
    BLOG_VERIFY_ERROR_CODE(rand_s(&value));

    return value;
}

static uint64_t _random64() {
    return BFX_MAKEUINT64(_random32(), _random32());
}