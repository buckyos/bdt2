

// ��ȡһ��[0, UINT64_MAX]����������
BFX_API(uint64_t) BfxRandom64();

// ��ȡһ��[minValue, maxValue]����������
BFX_API(int32_t) BfxRandomRange32(int32_t minValue, int32_t maxValue);

// ��ȡһ��[0, maxValue)����������
BFX_API(uint64_t) BfxRandomRange64(uint64_t maxValue);


BFX_API(void) BfxRandomBytes(uint8_t* lpBuffer, size_t size);