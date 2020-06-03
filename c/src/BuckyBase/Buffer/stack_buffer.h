#pragma once

BFX_DECLARE_HANDLE(BFX_STACK_BUFFER_HANDLE);

// 创建基于stack内存的buffer，只可以在当前调用桢使用，调用返回前需要调用BfxFinishStackBuffer结束使用


// data和buffer对象都在栈上分配
#define BFX_CREATE_STACK_BUFFER(size) \
	(BFX_STACK_BUFFER_HANDLE)BfxCreatePlacedBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(size)), BfxBufferCalcPlacedSize(size), size);

// buffer对象在栈上分配，只可在当前调用桢使用
#define BFX_CLIP_STACK_BUFFER(buffer, pos, length) \
	(BFX_STACK_BUFFER_HANDLE)BfxPlacedClipBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(0)), BfxBufferCalcPlacedSize(0), (BFX_BUFFER_HANDLE)buffer, pos, length);

// buffer对象在栈上分配，只可在当前调用桢使用
#define BFX_CREATE_STACK_BIND_BUFFER(data, size) \
	(BFX_STACK_BUFFER_HANDLE)BfxCreatePlacedBindBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(0)), BfxBufferCalcPlacedSize(0), data, size);

// 使用完毕后调用，需要和创建函数在同一个栈桢调用
// 虽然stack_buffer是无需释放的，但是为了明确生命周期，还是要对称调用
BFX_API(void) BfxFinishStackBuffer(BFX_STACK_BUFFER_HANDLE buffer);


BFX_API(size_t) BfxStackBufferGetLength(BFX_STACK_BUFFER_HANDLE buffer);

BFX_API(uint8_t*) BfxStackBufferGetData(BFX_STACK_BUFFER_HANDLE buffer, size_t* outBufferLength);

// buffer[pos, pos + min(readLen, bufferSize - pos)) -> dest[destPos, destPos + min(readLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferRead(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t readLen, void* dest, size_t destPos);

// src[srcPos, srcPos + min(writeLen, bufferSize - pos)] -> buffer[pos, pos + min(writeLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferWrite(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t writeLen, const void* src, size_t srcPos);


