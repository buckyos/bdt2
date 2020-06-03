#pragma once

BFX_DECLARE_HANDLE(BFX_STACK_BUFFER_HANDLE);

// ��������stack�ڴ��buffer��ֻ�����ڵ�ǰ������ʹ�ã����÷���ǰ��Ҫ����BfxFinishStackBuffer����ʹ��


// data��buffer������ջ�Ϸ���
#define BFX_CREATE_STACK_BUFFER(size) \
	(BFX_STACK_BUFFER_HANDLE)BfxCreatePlacedBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(size)), BfxBufferCalcPlacedSize(size), size);

// buffer������ջ�Ϸ��䣬ֻ���ڵ�ǰ������ʹ��
#define BFX_CLIP_STACK_BUFFER(buffer, pos, length) \
	(BFX_STACK_BUFFER_HANDLE)BfxPlacedClipBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(0)), BfxBufferCalcPlacedSize(0), (BFX_BUFFER_HANDLE)buffer, pos, length);

// buffer������ջ�Ϸ��䣬ֻ���ڵ�ǰ������ʹ��
#define BFX_CREATE_STACK_BIND_BUFFER(data, size) \
	(BFX_STACK_BUFFER_HANDLE)BfxCreatePlacedBindBuffer(BFX_ALLOCA(BfxBufferCalcPlacedSize(0)), BfxBufferCalcPlacedSize(0), data, size);

// ʹ����Ϻ���ã���Ҫ�ʹ���������ͬһ��ջ�����
// ��Ȼstack_buffer�������ͷŵģ�����Ϊ����ȷ�������ڣ�����Ҫ�ԳƵ���
BFX_API(void) BfxFinishStackBuffer(BFX_STACK_BUFFER_HANDLE buffer);


BFX_API(size_t) BfxStackBufferGetLength(BFX_STACK_BUFFER_HANDLE buffer);

BFX_API(uint8_t*) BfxStackBufferGetData(BFX_STACK_BUFFER_HANDLE buffer, size_t* outBufferLength);

// buffer[pos, pos + min(readLen, bufferSize - pos)) -> dest[destPos, destPos + min(readLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferRead(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t readLen, void* dest, size_t destPos);

// src[srcPos, srcPos + min(writeLen, bufferSize - pos)] -> buffer[pos, pos + min(writeLen, bufferSize - pos)]
BFX_API(size_t) BfxStackBufferWrite(BFX_STACK_BUFFER_HANDLE buffer, size_t pos, size_t writeLen, const void* src, size_t srcPos);


