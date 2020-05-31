
#include <stdlib.h>

typedef struct _BLogThreadBuffer {
    size_t size;
}_BLogThreadBuffer;

static BFX_THREAD_LOCAL _BLogThreadBuffer* _blogThreadBuffer;

static void _blogThreadAtExit(void) {
    if (_blogThreadBuffer) {
        BfxFree(_blogThreadBuffer);
        _blogThreadBuffer = NULL;
    }
}

static void* _blogGetThreadBuffer(size_t* size) {

    assert(*size > 0);

    _BLogThreadBuffer* cur = _blogThreadBuffer;
    if (cur == NULL) {
        cur = (_BLogThreadBuffer*)BfxMalloc(sizeof(_BLogThreadBuffer) + *size);
        if (cur == NULL) {
            return NULL;
        }

        cur->size = *size;
        _blogThreadBuffer = cur;

        atexit(_blogThreadAtExit);

    } else {
        if (cur->size < *size) {
            cur = (_BLogThreadBuffer*)BfxRealloc(cur, sizeof(_BLogThreadBuffer) + *size);
            if (cur == NULL) {
                return NULL;
            }

            cur->size = *size;
            _blogThreadBuffer = cur;
        }
    }

    *size = cur->size;
    return (cur + 1);
}
