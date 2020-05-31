#pragma once

// slotһ����tls���͵ı���������
// static BFX_THREAD_LOCAL void* slot;

BFX_API(void*) BfxThreadBufferMallocStatic(void** slot, size_t size);
BFX_API(void) BfxThreadBufferFreeStatic(void** slot, void* data);

// ������ʽTLS
BFX_API(void*) BfxThreadBufferMallocDynamic(BfxTlsKey* key, size_t size);
BFX_API(void) BfxThreadBufferFreeDynamic(BfxTlsKey* key, void* data);
