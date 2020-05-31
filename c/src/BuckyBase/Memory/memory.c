#include "../BuckyBase.h"

#if defined(BFX_COMPILER_MSVC)
#else
#include <stdlib.h>
#include <alloca.h>
#endif //

BFX_API(void*) BfxMalloc(size_t size) {
    return malloc(size);
}

BFX_API(void*) BfxRealloc(void* data, size_t size) {
    return realloc(data, size);
}

BFX_API(void) BfxFree(void* data) {
    free(data);
}
