#ifndef __BUCKY_BASE_BUFFER_H__
#define __BUCKY_BASE_BUFFER_H__


//BUFFER,目前设计为一个简单的，带有引用计数和size_t的uint8_t*封装
BFX_DECLARE_HANDLE(BFX_BUFFER_HANDLE);

// 创建一个指定大小的buffer
BFX_API(BFX_BUFFER_HANDLE) BfxCreateBuffer(size_t bufferSize);

// 创建基于指定内存的buffer，必须保证内存生命周期覆盖Buffer
BFX_API(BFX_BUFFER_HANDLE) BfxCreateBindBuffer(void* data, size_t size);

// 基于指定buffer创建一个子buffer，子buffer会持有源Buffer
BFX_API(BFX_BUFFER_HANDLE) BfxClipBuffer(BFX_BUFFER_HANDLE buffer, size_t pos, size_t length);


// 计算buffer对象需要的占位大小
BFX_API(size_t) BfxBufferCalcPlacedSize(size_t size);

// 直接在mem上创建buffer对象，确保mem长度>=BfxBufferCalcPlacedSize(size)
// place需要满足对象粒度的对齐
BFX_API(BFX_BUFFER_HANDLE) BfxCreatePlacedBuffer(void* place, size_t placeSize, size_t size);
BFX_API(BFX_BUFFER_HANDLE) BfxCreatePlacedBindBuffer(void* place, size_t placeSize, void* data, size_t size);
BFX_API(BFX_BUFFER_HANDLE) BfxPlacedClipBuffer(void* place, size_t placeSize, BFX_BUFFER_HANDLE handle, size_t pos, size_t length);


BFX_API(int32_t) BfxBufferAddRef(BFX_BUFFER_HANDLE buffer);
BFX_API(int32_t) BfxBufferRelease(BFX_BUFFER_HANDLE buffer);

// 直接获取buffer的内存，需要小心操作不要越界！ [data, data+outBufferLength)
BFX_API(uint8_t*) BfxBufferGetData(BFX_BUFFER_HANDLE buffer, size_t* outBufferLength);

// 获取buffer的长度
BFX_API(size_t) BfxBufferGetLength(BFX_BUFFER_HANDLE buffer);

// buffer[pos, pos + min(readLen, bufferSize - pos)) -> dest[destPos, destPos + min(readLen, bufferSize - pos)]
BFX_API(size_t) BfxBufferRead(BFX_BUFFER_HANDLE buffer, size_t pos, size_t readLen, void* dest, size_t destPos);

// src[srcPos, srcPos + min(writeLen, bufferSize - pos)] -> buffer[pos, pos + min(writeLen, bufferSize - pos)]
BFX_API(size_t) BfxBufferWrite(BFX_BUFFER_HANDLE buffer, size_t pos, size_t writeLen, const void* src, size_t srcPos);

BFX_API(bool) BfxBufferIsEqual(BFX_BUFFER_HANDLE lhs, BFX_BUFFER_HANDLE rhs);

#endif  //__BUCKY_BASE_BUFFER_H__