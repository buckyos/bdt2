#include "../BuckyBase.h"
#include "./Buffer.h"

#include <assert.h>
#include <string.h>

typedef enum {

    _BfxBufferType_Own = 0,
    _BfxBufferType_Out = 1,
    _BfxBufferType_Clip = 2,

} _BfxBufferType;

typedef struct _tagBfxBuffer {
    volatile int32_t ref;
    _BfxBufferType type;
    bool placed;

    size_t size;

    union {
        struct {
            struct _tagBfxBuffer* ptr;
            size_t pos;
        } other;
        uint8_t* out;
        uint8_t data[1];
    } data;
} _BfxBuffer;


static int32_t _bufferAddRef(_BfxBuffer* buffer);
static int32_t _bufferRelease(_BfxBuffer* buffer);

// 计算整个buffer对象占用内存大小
BFX_API(size_t) BfxBufferCalcPlacedSize(size_t size) {

    return sizeof(_BfxBuffer) + size;
}

// 如果指定了mem，需要确保mem大小足够
static _BfxBuffer* _newBuffer(void* mem, void* data, size_t size) {
    BLOG_CHECK(size >= 0);

    _BfxBuffer* buffer;
    if (data == NULL) {
        if (mem) {
            buffer = (_BfxBuffer*)mem;
        } else {
            buffer = (_BfxBuffer*)BfxMalloc(sizeof(_BfxBuffer) + size);
            if (buffer == NULL) {
                BLOG_ERROR("alloc buffer data failed! size=%zd", size);
                return NULL;
            }
        }

        buffer->ref = 1;
        buffer->size = size;
        buffer->type = _BfxBufferType_Own;
        buffer->placed = !!mem;
    } else {
        BLOG_CHECK(data);

        if (mem) {
            buffer = (_BfxBuffer*)mem;
        } else {
            buffer = (_BfxBuffer*)BfxMalloc(sizeof(_BfxBuffer));
            if (buffer == NULL) {
                BLOG_ERROR("alloc buffer data failed! size=%zd", size);
                return NULL;
            }
        }
        
        buffer->ref = 1;
        buffer->data.out = data;
        buffer->size = size;
        buffer->type = _BfxBufferType_Out;
        buffer->placed = !!mem;
    }

    return buffer;
}

static _BfxBuffer* _newClipBuffer(void* mem, _BfxBuffer* other, size_t pos, size_t size) {

    _BfxBuffer* buffer;
    if (mem) {
        buffer = (_BfxBuffer*)mem;
    } else {
        buffer = (_BfxBuffer*)BfxMalloc(sizeof(_BfxBuffer));
        if (buffer == NULL) {
            BLOG_ERROR("alloc buffer failed! size=%zd", size);
            return NULL;
        }
    }

    buffer->ref = 1;
    buffer->size = size;
    buffer->type = _BfxBufferType_Clip;
    buffer->placed = !!mem;
    buffer->data.other.ptr = other;
    buffer->data.other.pos = pos;

    // 需要持有此buffer
    _bufferAddRef(other);

    return buffer;
}

static void _deleteBuffer(_BfxBuffer* buffer) {
    if (buffer->type == _BfxBufferType_Clip) {
        _bufferRelease(buffer->data.other.ptr);
    }

    if (!buffer->placed) {
        BfxFree(buffer);
    }
}

static int32_t _bufferAddRef(_BfxBuffer* buffer) {
    assert(buffer);

    BLOG_CHECK(buffer->ref > 0);
    return BfxAtomicInc32(&buffer->ref);
}

static int32_t _bufferRelease(_BfxBuffer* buffer) {
    assert(buffer);

    BLOG_CHECK(buffer->ref > 0);
    int32_t ret = BfxAtomicDec32(&buffer->ref);
    if (ret == 0) {
        _deleteBuffer(buffer);
    }

    return ret;
}

static uint8_t* _bufferGetData(_BfxBuffer* buffer) {
    uint8_t* data;
    if (buffer->type == _BfxBufferType_Own) {
        data = buffer->data.data;
    }
    else if (buffer->type == _BfxBufferType_Clip) {
        data = _bufferGetData(buffer->data.other.ptr) + buffer->data.other.pos;
    }
    else {
        data = buffer->data.out;
    }

    return data;
}

BFX_API(BFX_BUFFER_HANDLE) BfxCreateBuffer(size_t bufferSize) {
    _BfxBuffer* result = _newBuffer(NULL, NULL, bufferSize);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(BFX_BUFFER_HANDLE) BfxCreateBindBuffer(void* data, size_t size) {
    assert(data);
    assert(size >= 0);

    _BfxBuffer* result = _newBuffer(NULL, data, size);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(BFX_BUFFER_HANDLE) BfxClipBuffer(BFX_BUFFER_HANDLE handle, size_t pos, size_t length) {
    _BfxBuffer* buffer = (_BfxBuffer*)(handle);
    assert(buffer);

    if (pos >= buffer->size || pos + length > buffer->size) {
        BLOG_ERROR("invalid clip params! srcSize=%zd, pos=%zd, length=%zd", buffer->size, pos, length);
        return NULL;
    }

    _BfxBuffer* result = _newClipBuffer(NULL, buffer, pos, length);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(BFX_BUFFER_HANDLE) BfxPlacedClipBuffer(void* place, size_t placeSize, BFX_BUFFER_HANDLE handle, size_t pos, size_t length) {
    _BfxBuffer* buffer = (_BfxBuffer*)(handle);
    assert(buffer);
    assert(place);
    assert(placeSize >= BfxBufferCalcPlacedSize(0));

    if (pos >= buffer->size || pos + length > buffer->size) {
        BLOG_ERROR("invalid clip params! srcSize=%zd, pos=%zd, length=%zd", buffer->size, pos, length);
        return NULL;
    }

    _BfxBuffer* result = _newClipBuffer(place, buffer, pos, length);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(BFX_BUFFER_HANDLE) BfxCreatePlacedBuffer(void* place, size_t placeSize, size_t size) {
    assert(place && placeSize >= BfxBufferCalcPlacedSize(size));
    assert(size >= 0);

    _BfxBuffer* result = _newBuffer(place, NULL, size);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(BFX_BUFFER_HANDLE) BfxCreatePlacedBindBuffer(void* place, size_t placeSize, void* data, size_t size) {
    assert(place && placeSize >= BfxBufferCalcPlacedSize(0));
    assert(data);
    assert(size >= 0);

    _BfxBuffer* result = _newBuffer(place, data, size);
    return (BFX_BUFFER_HANDLE)result;
}

BFX_API(uint8_t*) BfxBufferGetData(BFX_BUFFER_HANDLE handle, size_t* outBufferLength)
{
	_BfxBuffer* buffer = (_BfxBuffer*)(handle);
	assert(buffer);

	if (outBufferLength) {
		*outBufferLength = buffer->size;
	}

    return _bufferGetData(buffer);
}


BFX_API(size_t) BfxBufferGetLength(BFX_BUFFER_HANDLE buffer)
{
    if (BFX_UNLIKELY(buffer == NULL)) {
        return 0;
    }

    return ((_BfxBuffer*)buffer)->size;
}

BFX_API(int32_t) BfxBufferAddRef(BFX_BUFFER_HANDLE handle)
{
    return _bufferAddRef((_BfxBuffer*)handle);
}

BFX_API(int32_t) BfxBufferRelease(BFX_BUFFER_HANDLE handle)
{
    return _bufferRelease((_BfxBuffer*)handle);
}

BFX_API(size_t) BfxBufferRead(BFX_BUFFER_HANDLE handle, size_t pos, size_t readLen, void* dest, size_t destPos) {
    if (readLen == 0) {
        return 0;
    }

    BLOG_CHECK(dest);

    _BfxBuffer* buffer = (_BfxBuffer*)(handle);
    if (pos >= buffer->size) {
        BLOG_ERROR("read buffer overhead!! pos=%zd, buffersize=%zd", pos, buffer->size);
        return 0;
    }

    BLOG_CHECK(pos + readLen <= buffer->size);

    // 确定可读取的最大长度
    readLen = min(buffer->size - pos, readLen);

    memcpy((uint8_t*)dest + destPos, _bufferGetData(buffer) + pos, readLen);

    return readLen;
}

BFX_API(size_t) BfxBufferWrite(BFX_BUFFER_HANDLE handle, size_t pos, size_t writeLen, const void* src, size_t srcPos) {
    if (writeLen == 0) {
        return 0;
    }

    BLOG_CHECK(src);

    _BfxBuffer* buffer = (_BfxBuffer*)(handle);
    if (pos >= buffer->size) {
        BLOG_ERROR("write buffer overhead!! pos=%zd, size=%zd, writeLen=%zd", pos, buffer->size, writeLen);
        return 0;
    }

    BLOG_CHECK(pos + writeLen <= buffer->size);

    // 确定可写入的最大长度
    writeLen = min(buffer->size - pos, writeLen);

    memcpy(_bufferGetData(buffer) + pos, (uint8_t*)src + srcPos, writeLen);

    return writeLen;
}

BFX_API(bool) BfxBufferIsEqual(BFX_BUFFER_HANDLE lhs, BFX_BUFFER_HANDLE rhs)
{
	_BfxBuffer* lbuffer = (_BfxBuffer*)(lhs);
	_BfxBuffer* rbuffer = (_BfxBuffer*)(rhs);
	if (lbuffer == NULL && rbuffer == NULL)
	{
		return true;
	}

	if (lbuffer == NULL || rbuffer == NULL)
	{
		return false;
	}


	if (lbuffer->size != rbuffer->size)
	{
		return false;
	}

	uint8_t* lp = _bufferGetData(lbuffer);
	uint8_t* rp = _bufferGetData(rbuffer);
	return memcmp(lp, rp, lbuffer->size) == 0;

}