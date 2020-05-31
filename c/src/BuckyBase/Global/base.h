#pragma once

#define BFX_DECLARE_MULTITHREAD_REF_MEMBER \
    volatile int32_t ref

#define BFX_DECLARE_MULTITHREAD_REF_FUNCTION(type, delFunc) \
static int32_t BFX_MACRO_CONCAT(type, AddRef)(type* data) { \
    assert(data->ref > 0); \
    return BfxAtomicInc32(&data->ref); \
} \
static int32_t BFX_MACRO_CONCAT(type, Release)(type* data) { \
    assert(data->ref > 0); \
    int32_t ref = BfxAtomicDec32(&data->ref); \
    if (ref == 0) {\
        delFunc(data); \
    }\
    return ref; \
}

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif // !max

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif // !min