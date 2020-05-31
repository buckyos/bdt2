#pragma once

// 内存分配
BFX_API(void*) BfxMalloc(size_t size);
BFX_API(void*) BfxRealloc(void* data, size_t size);
BFX_API(void) BfxFree(void* data);

// 在栈上分配内存
#if defined(BFX_COMPILER_MSVC)
#define BFX_ALLOCA(size) \
	 __pragma(warning(suppress: 6255)) _alloca(size)
#else // GCC
#define BFX_ALLOCA(size) alloca(size)
#endif // GCC



// 申请指定类型大小的内存
#define BFX_MALLOC(size)                    BfxMalloc(size)
#define BFX_REALLOC(data, size)             BfxRealloc(data, size)
#define BFX_MALLOC_OBJ(type)                ((type*)BfxMalloc(sizeof(type)))
#define BFX_MALLOC_ARRAY(type, length)      ((type*)BfxMalloc(sizeof(type) * (length)))
#define BFX_FREE(p)                         (BfxFree(p))


