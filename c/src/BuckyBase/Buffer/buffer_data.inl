#include "../BuckyBase.h"

typedef struct {
    BFX_DECLARE_MULTITHREAD_REF_MEMBER;

    bool ownData;

    size_t size;
    uint8_t* data;

} _BufferData;

static _BufferData*  _newBufferData(void* data, size_t size) {
    BLOG_CHECK(size >= 0);

    bool ownData = false;
    if (data == NULL) {
        data = BfxMalloc(size);
        if (data == NULL) {
            BLOG_ERROR("alloc buffer data failed! size=%zd", size);
            return NULL;
        }

        ownData = true;
    }

    _BufferData* bufData = BFX_MALLOC_OBJ(_BufferData);
    bufData->ref = 1;
    bufData->ownData = ownData;
    bufData->size = size;
    bufData->data = data;

    return bufData;
}

static void _deleteBufferData(_BufferData* data) {
    if (data->ownData) {
        BfxFree(data->data);
    }

    data->data = NULL;

    BFX_FREE(data);
}

BFX_DECLARE_MULTITHREAD_REF_FUNCTION(_BufferData, _deleteBufferData);