

// 获取一个[0, UINT64_MAX]区间的随机数
BFX_API(uint64_t) BfxRandom64();

// 获取一个[minValue, maxValue]区间的随机数
BFX_API(int32_t) BfxRandomRange32(int32_t minValue, int32_t maxValue);

// 获取一个[0, maxValue)区间的随机数
BFX_API(uint64_t) BfxRandomRange64(uint64_t maxValue);


BFX_API(void) BfxRandomBytes(uint8_t* lpBuffer, size_t size);