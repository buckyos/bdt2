

/*
ret = *ptr;
if (*ptr == oldValue) {
	*ptr = newValue;
}
return ret;
*/
BFX_API(int32_t) BfxAtomicCompareAndSwap32(volatile int32_t* ptr
	, int32_t oldValue
	, int32_t newValue);

/*
ret = *ptr;
*ptr = value;
return ret;
*/
BFX_API(int32_t) BfxAtomicExchange32(volatile int32_t* ptr, int32_t value);

/*
return *ptr += value;
*/
BFX_API(int32_t) BfxAtomicAdd32(volatile int32_t* ptr, int32_t value);

/*
return *ptr += 1;
*/
BFX_API(int32_t) BfxAtomicInc32(volatile int32_t* ptr);


/*
return *ptr -= 1;
*/
BFX_API(int32_t) BfxAtomicDec32(volatile int32_t* ptr);

/*
tmp = *ptr;
*prt op= value;
return tmp;
*/
BFX_API(int32_t) BfxAtomicAnd32(volatile int32_t* ptr, int32_t vlaue);
BFX_API(int32_t) BfxAtomicOr32(volatile int32_t* ptr, int32_t vlaue);
BFX_API(int32_t) BfxAtomicXor32(volatile int32_t* ptr, int32_t vlaue);


#if defined(BFX_ARCH_CPU_64_BITS)
BFX_API(int64_t) BfxAtomicCompareAndSwap64(volatile int64_t* ptr
	, int64_t oldValue
	, int64_t newValue);
BFX_API(int64_t) BfxAtomicExchange64(volatile int64_t* ptr, int64_t value);

BFX_API(int64_t) BfxAtomicAnd64(volatile int64_t* ptr, int64_t vlaue);
BFX_API(int64_t) BfxAtomicOr64(volatile int64_t* ptr, int64_t vlaue);
BFX_API(int64_t) BfxAtomicXor64(volatile int64_t* ptr, int64_t vlaue);

#endif // BFX_ARCH_CPU_64_BITS

typedef void* BfxAtomicPointerType;
BFX_API(BfxAtomicPointerType) BfxAtomicCompareAndSwapPointer(volatile BfxAtomicPointerType* ptr
	, BfxAtomicPointerType oldValue
	, BfxAtomicPointerType newValue);
BFX_API(BfxAtomicPointerType) BfxAtomicExchangePointer(volatile BfxAtomicPointerType* ptr
    , BfxAtomicPointerType value);