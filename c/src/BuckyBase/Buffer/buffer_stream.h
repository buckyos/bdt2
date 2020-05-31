#pragma once

#include "../Global/basic_type.h"

typedef struct BfxBufferStream {
    uint8_t* buffer;
    size_t length;
    size_t pos;
} BfxBufferStream;

//BufferStream默认使用主机序

BFX_API(void) BfxBufferStreamInit(BfxBufferStream* stream, void* buffer, size_t length);
BFX_API(size_t) BfxBufferStreamGetPos(BfxBufferStream* stream);
BFX_API(size_t) BfxBufferStreamGetTailLength(BfxBufferStream* stream);
BFX_API(int) BfxBufferStreamSetPos(BfxBufferStream* stream, size_t newPos);

BFX_API(int) BfxBufferReadUInt8(BfxBufferStream* stream, uint8_t* outResult);
BFX_API(int) BfxBufferWriteUInt8(BfxBufferStream* stream, uint8_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadInt8(BfxBufferStream* stream, int8_t* outResult);
BFX_API(int) BfxBufferWriteInt8(BfxBufferStream* stream, int8_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadUInt16(BfxBufferStream* stream, uint16_t* outResult);
BFX_API(int) BfxBufferWriteUInt16(BfxBufferStream* stream, uint16_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadInt16(BfxBufferStream* stream, int16_t* outResult);
BFX_API(int) BfxBufferWriteInt16(BfxBufferStream* stream, int16_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadUInt32(BfxBufferStream* stream, uint32_t* outResult);
BFX_API(int) BfxBufferWriteUInt32(BfxBufferStream* stream, uint32_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadInt32(BfxBufferStream* stream, int32_t* outResult);
BFX_API(int) BfxBufferWriteInt32(BfxBufferStream* stream, int32_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadUInt48(BfxBufferStream* stream, uint64_t* outResult);
BFX_API(int) BfxBufferWriteUInt48(BfxBufferStream* stream, uint64_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadUInt64(BfxBufferStream* stream, uint64_t* outResult);
BFX_API(int) BfxBufferWriteUInt64(BfxBufferStream* stream, uint64_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadInt64(BfxBufferStream* stream, int64_t* outResult);
BFX_API(int) BfxBufferWriteInt64(BfxBufferStream* stream, int64_t v, size_t* pWriteBytes);

BFX_API(int) BfxBufferReadByteArray(BfxBufferStream* stream, uint8_t* out, size_t length);
BFX_API(int) BfxBufferWriteByteArray(BfxBufferStream* stream, const uint8_t* input, size_t length);

BFX_API(int) BfxBufferReadByteArrayBeginWithU16Length(BfxBufferStream* stream, BFX_BUFFER_HANDLE* hOutBuffer);
BFX_API(int) BfxBufferWriteByteArrayBeginWithU16Length(BfxBufferStream* stream, BFX_BUFFER_HANDLE hBuffer, size_t* pWriteBytes);