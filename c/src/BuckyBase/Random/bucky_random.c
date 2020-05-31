#include "../BuckyBase.h"

#if defined(BFX_OS_WIN)
#include "./random_win.inl"
#else
#include "./random_posix.inl"
#endif

BFX_API(uint64_t) BfxRandom64(uint64_t maxValue) {

    // Ϊ��ȡģʱ��ȷ������[0, maxValue)֮���������Ǿ��ȵ�
    const uint64_t most = (UINT64_MAX / maxValue) * maxValue - 1;

    uint64_t value;
    do {
        value = _random64();
    } while (value > most);

    return (value % maxValue);
}


// ��ȡһ��[begin, end]����������
BFX_API(int32_t) BfxRandomRange32(int32_t begin, int32_t end) {
    BLOG_CHECK(begin <= end);
    if (begin == end) {
        return begin;
    }

    int32_t result = begin + (int32_t)BfxRandom64((uint64_t)end - begin + 1);

    BLOG_CHECK(result >= begin);
    BLOG_CHECK(result <= end);

    return result;
}

#ifdef BFX_OS_WIN
BFX_API(void) BfxRandomBytes(uint8_t* lpBuffer, size_t size) {
    size_t filled = 0;
    while (filled < size) {
        uint64_t value = _random64();
        memcpy(lpBuffer + filled, &value, min(sizeof(value), size - filled));
        filled += sizeof(value);
    }
}
#endif // BFX_OS_WIN