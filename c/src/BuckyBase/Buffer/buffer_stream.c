#include "../BuckyBase.h"
#include "./buffer_stream.h"


BFX_API(void) BfxBufferStreamInit(BfxBufferStream* stream, void* buffer, size_t length)
{
    memset(stream, 0, sizeof(BfxBufferStream));
    stream->buffer = buffer;
    stream->length = length;
}

BFX_API(size_t) BfxBufferStreamGetPos(BfxBufferStream* stream)
{
    assert(stream);
    return stream->pos;
}

BFX_API(size_t) BfxBufferStreamGetTailLength(BfxBufferStream* stream)
{
    assert(stream);
    assert(stream->length >= stream->pos);

    return stream->length - stream->pos;
}

BFX_API(int) BfxBufferStreamSetPos(BfxBufferStream* stream, size_t newPos)
{
    assert(stream);
    assert(newPos <= stream->length);
    stream->pos = newPos;
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadUInt8(BfxBufferStream* self, uint8_t* outResult)
{
    assert(self);
    assert(self->pos + 1 <= self->length);
    *outResult = *(uint8_t*)(self->buffer + self->pos);
    self->pos += 1;
    return 1; //read 1 bytes
}

BFX_API(int) BfxBufferWriteUInt8(BfxBufferStream* self, uint8_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 1 <= self->length);
    *(uint8_t*)(self->buffer + self->pos) = v;
    self->pos += 1;
    *pWriteBytes = 1; //write 1 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadInt8(BfxBufferStream* self, int8_t* outResult)
{
    assert(self);
    assert(self->pos + 1 <= self->length);
    *outResult = *(int8_t*)(self->buffer + self->pos);
    self->pos += 1;
    return 1; //read 1 bytes
}


BFX_API(int) BfxBufferWriteInt8(BfxBufferStream* self, int8_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 1 <= self->length);
    *(int8_t*)(self->buffer + self->pos) = v;
    self->pos += 1;
    *pWriteBytes = 1; //write 1 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadByteArray(BfxBufferStream* self, uint8_t* outResult, size_t length)
{
    assert(self);
    assert(self->pos + length <= self->length);
    memcpy(outResult, self->buffer + self->pos, length);
    self->pos += length;
    return (int)length;
}

BFX_API(int) BfxBufferWriteByteArray(BfxBufferStream* self, const uint8_t* input, size_t length)
{
    assert(self);
    assert(self->pos + length <= self->length);
    memcpy(self->buffer + self->pos, input, length);
    self->pos += length;

    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadByteArrayBeginWithU16Length(BfxBufferStream* self, BFX_BUFFER_HANDLE* hOutBuffer)
{
    assert(self);
    assert(self->pos + 2 <= self->length);
    uint16_t len;
    len = *(uint16_t*)(self->buffer + self->pos);
    self->pos += 2;
    *hOutBuffer = NULL;

    if (len == 0)
    {
        return 2;
    }
    if (self->pos + len > self->length)
    {
        return -1;
    }

    *hOutBuffer = BfxCreateBuffer(len);
    size_t bufferLen;
    uint8_t* out = BfxBufferGetData(*hOutBuffer, &bufferLen);

    memcpy(out, self->buffer + self->pos, len);
    self->pos += len;
    return 2 + len;
}

BFX_API(int) BfxBufferWriteByteArrayBeginWithU16Length(BfxBufferStream* self, BFX_BUFFER_HANDLE hBuffer, size_t* pWriteBytes)
{
    assert(self);
    size_t length;
    uint8_t* input = BfxBufferGetData(hBuffer, &length);
    assert(length < 0xffff);
    assert(self->pos + length + 2 <= self->length);

    *(uint16_t*)(self->buffer + self->pos) = (uint16_t)length;
    self->pos += 2;
    if (length > 0)
    {
        memcpy(self->buffer + self->pos, input, length);
        self->pos += length;
    }
    *pWriteBytes = 2 + length;
    return BFX_RESULT_SUCCESS;
}
//TODO：下列函数在MIPS架构下的实现未完成
BFX_API(int) BfxBufferReadUInt16(BfxBufferStream* self, uint16_t* outResult)
{
    assert(self);
    assert(self->pos + 2 <= self->length);
    *outResult = *(uint16_t*)(self->buffer + self->pos);
    self->pos += 2;
    return 2; //read 2 bytes
}

BFX_API(int) BfxBufferWriteUInt16(BfxBufferStream* self, uint16_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 2 <= self->length);
    *(uint16_t*)(self->buffer + self->pos) = v;
    self->pos += 2;
    *pWriteBytes = 2; //write 2 byte
    return BFX_RESULT_SUCCESS;
}


BFX_API(int) BfxBufferReadInt16(BfxBufferStream* self, int16_t* outResult)
{
    assert(self);
    assert(self->pos + 2 <= self->length);
    *outResult = *(int16_t*)(self->buffer + self->pos);
    self->pos += 2;
    return 2;
}

BFX_API(int) BfxBufferWriteInt16(BfxBufferStream* self, int16_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 2 <= self->length);
    *(int16_t*)(self->buffer + self->pos) = v;
    self->pos += 2;
    *pWriteBytes = 2; //write 2 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadUInt32(BfxBufferStream* self, uint32_t* outResult)
{
    assert(self);
    assert(self->pos + 4 <= self->length);
    *outResult = *(uint32_t*)(self->buffer + self->pos);
    self->pos += 4;
    return 4;
}

BFX_API(int) BfxBufferWriteUInt32(BfxBufferStream* self, uint32_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 4 <= self->length);
    *(uint32_t*)(self->buffer + self->pos) = v;
    self->pos += 4;
    *pWriteBytes = 4; //write 4 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadInt32(BfxBufferStream* self, int32_t* outResult)
{
    assert(self);
    assert(self->pos + 4 <= self->length);
    *outResult = *(int32_t*)(self->buffer + self->pos);
    self->pos += 4;
    return 4;
}

BFX_API(int) BfxBufferWriteInt32(BfxBufferStream* self, int32_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 4 <= self->length);
    *(int32_t*)(self->buffer + self->pos) = v;
    self->pos += 4;
    *pWriteBytes = 4; //write 4 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadUInt48(BfxBufferStream* self, uint64_t* outResult)
{
	assert(self);
	assert(self->pos + 6 <= self->length);
	
	*outResult = *(uint64_t*)(self->buffer + self->pos) & 0x0000ffffffffffff;
	self->pos += 6;
	return 6;
}

BFX_API(int) BfxBufferWriteUInt48(BfxBufferStream* self, uint64_t v, size_t* pWriteBytes)
{
	assert(self);
	assert(self->pos + 6 <= self->length);
	*(uint32_t*)(self->buffer + self->pos) = v & 0x00000000ffffffff;
	*(uint16_t*)(self->buffer + self->pos + 4) = (v >> 32) & 0x0000ffff;
	self->pos += 6;
	*pWriteBytes = 6; //write 6 byte
	return BFX_RESULT_SUCCESS;
}


BFX_API(int) BfxBufferReadUInt64(BfxBufferStream* self, uint64_t* outResult)
{
    assert(self);
    assert(self->pos + 8 <= self->length);
    *outResult = *(uint64_t*)(self->buffer + self->pos);
    self->pos += 8;
    return 8;
}

BFX_API(int) BfxBufferWriteUInt64(BfxBufferStream* self, uint64_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 8 <= self->length);
    *(uint64_t*)(self->buffer + self->pos) = v;
    self->pos += 8;
    *pWriteBytes = 8; //write 8 byte
    return BFX_RESULT_SUCCESS;
}

BFX_API(int) BfxBufferReadInt64(BfxBufferStream* self, int64_t* outResult)
{
    assert(self);
    assert(self->pos + 8 <= self->length);
    *outResult = *(int64_t*)(self->buffer + self->pos);
    self->pos += 8;
    return 8;
}

BFX_API(int) BfxBufferWriteInt64(BfxBufferStream* self, int64_t v, size_t* pWriteBytes)
{
    assert(self);
    assert(self->pos + 8 <= self->length);
    *(int64_t*)(self->buffer + self->pos) = v;
    self->pos += 8;
    *pWriteBytes = 8; //write 8 byte
    return BFX_RESULT_SUCCESS;
}