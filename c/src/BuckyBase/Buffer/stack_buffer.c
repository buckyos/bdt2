#include "../BuckyBase.h"
#include "./Buffer.h"
#include "./stack_buffer.h"


BFX_API(BFX_STACK_BUFFER_HANDLE) BfxCreateBindStackBuffer(void* data, size_t size) {
    // TODO 这里是否需要严格检测data是栈区指针？

    return (BFX_STACK_BUFFER_HANDLE)BfxCreateBindBuffer(data, size);
}

BFX_API(void) BfxFinishStackBuffer(BFX_STACK_BUFFER_HANDLE buffer) {
    int32_t ret = BfxBufferRelease((BFX_BUFFER_HANDLE)buffer);
    BLOG_CHECK(ret == 0);
}

BFX_API(size_t) BfxStackBufferGetLength(BFX_STACK_BUFFER_HANDLE buffer) {
    return BfxBufferGetLength((BFX_BUFFER_HANDLE)buffer);
}

BFX_API(uint8_t*) BfxStackBufferGetData(BFX_STACK_BUFFER_HANDLE buffer, size_t* outBufferLength) {
    return BfxBufferGetData((BFX_BUFFER_HANDLE)buffer, outBufferLength);
}

// buffer[pos, pos + min(readLen, bufferSize - pos)) -> dest[destPos, destPos + min(readLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferRead(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t readLen, void* dest, size_t destPos) {
    return BfxBufferRead((BFX_BUFFER_HANDLE)buffer, pos, readLen, dest, destPos);
}

// src[srcPos, srcPos + min(writeLen, bufferSize - pos)] -> buffer[pos, pos + min(writeLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferWrite(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t writeLen, const void* src, size_t srcPos) {
    return BfxBufferWrite((BFX_BUFFER_HANDLE)buffer, pos, writeLen, src, srcPos);
}