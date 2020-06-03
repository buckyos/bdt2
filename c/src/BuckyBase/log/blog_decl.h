#pragma once
#include "./blog_config.h"
#include "./blog_print.h"
#include "./blog_global_config.h"


#define BLOG_DECLARE_ISOLATE(isolate)                                           \
typedef struct _BLogIsolateConfig  _BLogIsolateConfig;                          \
_BLogIsolateConfig* _blogGet ## isolate ## Config();                            \


#define BLOG_IMPL_ISOLATE(isolate)                                              \
static _BLogIsolateConfig _blog ## isolate ## ConfigInstance = { 0 };           \
static void _initBLog ## isolate ## Config(void) {                              \
    _blogConfigInit(&_blog ## isolate ## ConfigInstance, #isolate);             \
}                                                                               \
_BLogIsolateConfig* _blogGet ## isolate ## Config() {                           \
    static volatile int32_t s_init = 0;                                         \
    if (s_init == 2) {                                                          \
        return & _blog ## isolate ## ConfigInstance;                            \
    }                                                                           \
    if (BfxAtomicCompareAndSwap32(&s_init, 0, 1) == 0) {                        \
        _blogConfigInit(&_blog ## isolate ## ConfigInstance, #isolate);         \
        s_init = 2;                                                             \
        return & _blog ## isolate ## ConfigInstance;                            \
    } else {                                                                    \
        return NULL;                                                            \
    }                                                                           \
}                                                                               \

#define _BLOG_LOG_IMPL(isolate, cond, level, func, file, line, ...)                     \
do {                                                                                    \
    _BLogPos __blog_pos__;                                                              \
    _blogPosInit(&__blog_pos__, file, line, func, level);                               \
    _BLogIsolateConfig* __blog_config__ = _blogGet ## isolate ## Config();              \
    if (__blog_config__ && _blogConfigCheckSwitch(__blog_config__, level) && (cond)) {  \
        _blogPrintf(__blog_config__, level, &__blog_pos__, __VA_ARGS__);                \
    }                                                                                   \
} while (0);                                                                            \

#define _BLOG_LOG_EX(isolate, cond, level, ...)     \
    _BLOG_LOG_IMPL(isolate, cond, level,  __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

#define _BLOG_CHECK(isolate, cond)                  \
    _BLOG_LOG_EX(isolate, !(cond), BLogLevel_Check, "CHECK FAILED: %s", #cond); \

#define _BLOG_CHECK_EX(isolate, cond, ...)          \
    _BLOG_LOG_EX(isolate, !(cond), BLogLevel_Check, ##__VA_ARGS__);             \


#define _BLOG_HEX_IMPL(isolate, cond, level, func, file, line, address, length, ...)    \
do {                                                                                    \
    _BLogPos pos;                                                                       \
    _blogPosInit(&pos, file, line, func, level);                                        \
    _BLogIsolateConfig* config = _blogGet ## isolate ## Config();                       \
    if (config && _blogConfigCheckSwitch(config, level) && (cond)) {                    \
        _blogHexPrintf(config, level, &pos, address, length, __VA_ARGS__);              \
    }                                                                                   \
} while (0);                                                                            \

#define _BLOG_HEX_EX(isolate, cond, level, address, length, ...)     \
    _BLOG_HEX_IMPL(isolate, cond, level, __FUNCTION__, __FILE__, __LINE__, address, length, ##__VA_ARGS__)


#if defined(_MSC_VER)
#   define BLOG_NOOP   __noop
#else // GNUC
#   define BLOG_NOOP  ((void)0)
#endif // GNUC