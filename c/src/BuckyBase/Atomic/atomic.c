#include "../BuckyBase.h"

#if defined(BFX_OS_WIN)
#include "./atomic_msvc.h"
#else
#include "./atomic_gcc.h"
#endif

#if defined(BFX_ARCH_CPU_32_BITS)

BFX_API(BfxAtomicPointerType) BfxAtomicCompareAndSwapPointer(volatile BfxAtomicPointerType* ptr
	, BfxAtomicPointerType oldValue
	, BfxAtomicPointerType newValue) {
	return (BfxAtomicPointerType)BfxAtomicCompareAndSwap32((volatile int32_t*)ptr,
		(int32_t)oldValue,
		(int32_t)newValue);
}

BFX_API(BfxAtomicPointerType) BfxAtomicExchangePointer(volatile BfxAtomicPointerType* ptr, BfxAtomicPointerType value) {
	return (BfxAtomicPointerType)BfxAtomicExchange32((volatile int32_t*)ptr,
		(int32_t)value);
}

#else // BFX_ARCH_CPU_64_BITS 

BFX_API(BfxAtomicPointerType) BfxAtomicCompareAndSwapPointer(volatile BfxAtomicPointerType* ptr
	, BfxAtomicPointerType oldValue
	, BfxAtomicPointerType newValue) {
	return (BfxAtomicPointerType)BfxAtomicCompareAndSwap64((volatile int64_t*)ptr,
		(int64_t)oldValue,
		(int64_t)newValue);
}

BFX_API(BfxAtomicPointerType) BfxAtomicExchangePointer(volatile BfxAtomicPointerType* ptr, BfxAtomicPointerType value) {
	return (BfxAtomicPointerType)BfxAtomicExchange64((volatile int64_t*)ptr,
		(int64_t)value);
}

#endif // BFX_ARCH_CPU_64_BITS