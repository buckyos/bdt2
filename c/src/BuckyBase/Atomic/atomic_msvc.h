

BFX_API(int32_t) BfxAtomicCompareAndSwap32(volatile int32_t* ptr
	, int32_t oldValue
	, int32_t newValue) {

	LONG result = InterlockedCompareExchange(
		(volatile LONG*)ptr,
		(LONG)newValue,
		(LONG)oldValue);

	return (int32_t)(result);
}

BFX_API(int32_t) BfxAtomicInc32(volatile int32_t* ptr) {
	LONG ret = InterlockedIncrement((volatile LONG*)ptr);

	return (int32_t)ret;
}

BFX_API(int32_t) BfxAtomicAdd32(volatile int32_t* ptr, int32_t value) {
	LONG ret = value + InterlockedExchangeAdd((volatile LONG*)ptr, (LONG)value);

	return (int32_t)ret;
}

BFX_API(int32_t) BfxAtomicDec32(volatile int32_t* ptr) {
	LONG ret = InterlockedDecrement((volatile LONG*)ptr);

	return (int32_t)ret;
}


BFX_API(int32_t) BfxAtomicExchange32(volatile int32_t* ptr, int32_t value) {
	LONG result = InterlockedExchange(
		(volatile LONG*)ptr,
		(LONG)value);

	return (int32_t)result;
}

BFX_API(int32_t) BfxAtomicAnd32(volatile int32_t* ptr, int32_t value) {
	return (int32_t)InterlockedAnd((volatile LONG*)ptr, (LONG)value);
}

BFX_API(int32_t) BfxAtomicOr32(volatile int32_t* ptr, int32_t value) {
	return (int32_t)InterlockedOr((volatile LONG*)ptr, (LONG)value);
}

BFX_API(int32_t) BfxAtomicXor32(volatile int32_t* ptr, int32_t value) {
	return (int32_t)InterlockedXor((volatile LONG*)ptr, (LONG)value);
}

#if defined(BFX_ARCH_CPU_64_BITS)
BFX_API(int64_t) BfxAtomicCompareAndSwap64(volatile int64_t* ptr
	, int64_t oldValue
	, int64_t newValue) {

	LONG64 result = InterlockedCompareExchange64(
		(volatile LONG64*)ptr,
		(LONG64)newValue,
		(LONG64)oldValue);

	return (int64_t)(result);
}

BFX_API(int64_t) BfxAtomicExchange64(volatile int64_t* ptr, int64_t value) {
	LONG64 result = InterlockedExchange64(
		(volatile LONG64*)ptr,
		(LONG64)value);

	return (int64_t)result;
}

BFX_API(int64_t) BfxAtomicAnd64(volatile int64_t* ptr, int64_t value) {
	return (int64_t)InterlockedAnd64((volatile LONG64*)ptr, (LONG64)value);
}

BFX_API(int64_t) BfxAtomicOr64(volatile int64_t* ptr, int64_t value) {
	return (int64_t)InterlockedOr64((volatile LONG64*)ptr, (LONG64)value);
}

BFX_API(int64_t) BfxAtomicXor64(volatile int64_t* ptr, int64_t value) {
	return (int64_t)InterlockedXor64((volatile LONG64*)ptr, (LONG64)value);
}
#endif // BFX_ARCH_CPU_64_BITS