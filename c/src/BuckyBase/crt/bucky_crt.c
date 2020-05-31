#include "../BuckyBase.h"

#if defined(BFX_OS_WIN)
#include <stdlib.h>
#else
#include "./crt_raw.inl"
#endif


BFX_API(int) BfxStricmp(const char8_t* dst, const char8_t* src) {
    return _stricmp(dst, src);
}

BFX_API(int) BfxStrnicmp(const char8_t* dst, const char8_t* src, size_t count) {
    return _strnicmp(dst, src, count);
}