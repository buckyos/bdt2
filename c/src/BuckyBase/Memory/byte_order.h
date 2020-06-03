// Host 主机字节序，取决于当前平台
// LE   Littel Endian  小尾端字节序
// BE   Big Endian 大尾端字节序

// 反转字节序
BFX_API(uint16_t) BfxReverse16(uint16_t value);
BFX_API(uint32_t) BfxReverse32(uint32_t value);
BFX_API(uint64_t) BfxReverse64(uint64_t value);

#if defined(BFX_ARCH_CPU_LITTLE_ENDIAN)

#define BFX_HOST_TO_LE_16(value) (value)
#define BFX_HOST_TO_LE_32(value) (value)
#define BFX_HOST_TO_LE_64(value) (value)

#define BFX_LE_TO_HOST_16(value) (value)
#define BFX_LE_TO_HOST_32(value) (value)
#define BFX_LE_TO_HOST_64(value) (value)

#define BFX_HOST_TO_BE_16(value) (BfxReverse16(value))
#define BFX_HOST_TO_BE_32(value) (BfxReverse32(value))
#define BFX_HOST_TO_BE_64(value) (BfxReverse64(value))

#define BFX_BE_TO_HOST_16(value) (BfxReverse16(value))
#define BFX_BE_TO_HOST_32(value) (BfxReverse32(value))
#define BFX_BE_TO_HOST_64(value) (BfxReverse64(value))

#else // BFX_ARCH_CPU_BIG_ENDIAN

#define BFX_HOST_TO_LE_16(value) (BfxReverse16(value))
#define BFX_HOST_TO_LE_32(value) (BfxReverse32(value))
#define BFX_HOST_TO_LE_64(value) (BfxReverse64(value))

#define BFX_LE_TO_HOST_16(value) (BfxReverse16(value))
#define BFX_LE_TO_HOST_32(value) (BfxReverse32(value))
#define BFX_LE_TO_HOST_64(value) (BfxReverse64(value))

#define BFX_HOST_TO_BE_16(value) (value)
#define BFX_HOST_TO_BE_32(value) (value)
#define BFX_HOST_TO_BE_64(value) (value)

#define BFX_BE_TO_HOST_16(value) (value)
#define BFX_BE_TO_HOST_32(value) (value)
#define BFX_BE_TO_HOST_64(value) (value)

#endif // BFX_ARCH_CPU_BIG_ENDIAN