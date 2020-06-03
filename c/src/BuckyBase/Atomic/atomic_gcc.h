

BFX_API(int32_t) BfxAtomicCompareAndSwap32(volatile int32_t* ptr
	, int32_t oldValue
	, int32_t newValue) {

	__atomic_compare_exchange_n(ptr, &oldValue, newValue
		, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);

	return oldValue;
}


BFX_API(int32_t) BfxAtomicInc32(volatile int32_t* ptr) {
	return BfxAtomicAdd32(ptr, 1);
}

BFX_API(int32_t) BfxAtomicAdd32(volatile int32_t* ptr, int32_t value) {
	return value + __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int32_t) BfxAtomicDec32(volatile int32_t* ptr) {
	return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST) - 1;
}

BFX_API(int32_t) BfxAtomicExchange32(volatile int32_t* ptr, int32_t value) {
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int32_t) BfxAtomicAnd32(volatile int32_t* ptr, int32_t value) {
	return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int32_t) BfxAtomicOr32(volatile int32_t* ptr, int32_t value) {
	return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int32_t) BfxAtomicXor32(volatile int32_t* ptr, int32_t value) {
	return __atomic_fetch_xor(ptr, value, __ATOMIC_SEQ_CST);
}

#if defined(BFX_ARCH_CPU_64_BITS)
BFX_API(int64_t) BfxAtomicCompareAndSwap64(volatile int64_t* ptr
	, int64_t oldValue
	, int64_t newValue) {

	__atomic_compare_exchange_n(ptr, &oldValue, newValue
		, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);

	return oldValue;
}

BFX_API(int64_t) BfxAtomicExchange64(volatile int64_t* ptr, int64_t value) {
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int64_t) BfxAtomicAnd64(volatile int64_t* ptr, int64_t value) {
	return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int64_t) BfxAtomicOr64(volatile int64_t* ptr, int64_t value) {
	return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

BFX_API(int64_t) BfxAtomicXor64(volatile int64_t* ptr, int64_t value) {
	return __atomic_fetch_xor(ptr, value, __ATOMIC_SEQ_CST);
}

#endif // BFX_ARCH_CPU_64_BITS

