#pragma once

// slot一般是tls类型的变量，如下
// static BFX_THREAD_LOCAL void* slot;

BFX_API(void*) BfxThreadBufferMallocStatic(void** slot, size_t size);
BFX_API(void) BfxThreadBufferFreeStatic(void** slot, void* data);

// 基于显式TLS
BFX_API(void*) BfxThreadBufferMallocDynamic(BfxTlsKey* key, size_t size);
BFX_API(void) BfxThreadBufferFreeDynamic(BfxTlsKey* key, void* data);
