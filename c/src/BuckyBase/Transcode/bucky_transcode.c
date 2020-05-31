#include "../BuckyBase.h"
#include "./ConvertUTF.h"

static int _transCResult(ConversionResult cret) {
    int ret = BFX_RESULT_SUCCESS;

    switch (cret) {
    case sourceIllegal:
        ret = BFX_RESULT_INVALID_CHARS;
        break;
    case sourceExhausted:
        ret = BFX_RESULT_INCOMPLETE_CHARS;
        break;
    case targetExhausted:
        ret = BFX_RESULT_INSUFFICIENT_BUFFER;
        break;
    default:
        break;
    }

    return ret;
}


BFX_API(int) BfxTranscodeUTF16ToUTF8(const char16_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen && *sourceLen >= 0);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen && *destLen >= 0);

    char* destBuffer = dest;

    ConversionResult result = ConvertUTF16toUTF8(&source, source + *sourceLen
        , (UTF8**)&destBuffer, (UTF8*)dest + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

BFX_API(int) BfxTranscodeUTF8ToUTF16(const char8_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen);

    char16_t* destBuffer = dest;

    ConversionResult result = ConvertUTF8toUTF16((const UTF8**)&source, (const UTF8*)source + *sourceLen
        , &destBuffer, destBuffer + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

BFX_API(int) BfxTranscodeUTF16ToUTF32(const char16_t* source, size_t* sourceLen, char32_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen);

    char32_t* destBuffer = dest;

    ConversionResult result = ConvertUTF16toUTF32(&source, source + *sourceLen
        , &destBuffer, destBuffer + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

BFX_API(int) BfxTranscodeUTF32ToUTF16(const char32_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen);

    char16_t* destBuffer = dest;

    ConversionResult result = ConvertUTF32toUTF16(&source, source + *sourceLen
        , &destBuffer, destBuffer + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

BFX_API(int) BfxTranscodeUTF8ToUTF32(const char8_t* source, size_t* sourceLen, char32_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen);

    char32_t* destBuffer = dest;

    ConversionResult result = ConvertUTF8toUTF32((const UTF8**)&source, (const UTF8*)source + *sourceLen
        , &destBuffer, destBuffer + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

BFX_API(int) BfxTranscodeUTF32ToUTF8(const char32_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    BLOG_CHECK(source);
    BLOG_CHECK(sourceLen);
    BLOG_CHECK(dest);
    BLOG_CHECK(destLen);

    char8_t* destBuffer = dest;

    ConversionResult result = ConvertUTF32toUTF8(&source, source + *sourceLen
        , (UTF8**)&destBuffer, (UTF8*)destBuffer + *destLen, lenientConversion);

    *sourceLen = source - source;
    *destLen = destBuffer - dest;

    return _transCResult(result);
}

#define _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen)  \
    BLOG_CHECK(source); \
    BLOG_CHECK(sourceLen); \
    BLOG_CHECK(dest); \
    BLOG_CHECK(destLen); \
    size_t copyLen = min(*(sourceLen), *(destLen)); \
    memcpy(dest, source, sizeof(*source) * copyLen); \
    int ret = 0; \
    if (*sourceLen > copyLen) { \
        ret = BFX_RESULT_INSUFFICIENT_BUFFER; \
    } \
    *sourceLen = copyLen; \
    *destLen = copyLen; \
    return ret; \

#if defined(BFX_WCHAR_T_UTF16)

BFX_API(int) BfxTranscodeWCharToUTF16(const wchar_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF16ToWChar(const char16_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeWCharToUTF8(const wchar_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    return BfxTranscodeUTF16ToUTF8(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF8ToWChar(const char8_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    return BfxTranscodeUTF8ToUTF16(source, sourceLen, dest, destLen);
}

#elif defined(BFX_WCHAR_T_UTF8)

BFX_API(int) BfxTranscodeWCharToUTF16(const wchar_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen) {
    return BfxTranscodeUTF8ToUTF16(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF16ToWChar(const char16_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    return BfxTranscodeUTF16ToUTF8(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeWCharToUTF8(const wchar_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF8ToWChar(const char8_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}

#elif defined(BFX_WCHAR_T_UTF32)

BFX_API(int) BfxTranscodeWCharToUTF16(const wchar_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen) {
    return BfxTranscodeUTF32ToUTF16((const char32_t*)source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF16ToWChar(const char16_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    return BfxTranscodeUTF16ToUTF32(source, sourceLen, (char32_t*)dest, destLen);
}

BFX_API(int) BfxTranscodeWCharToUTF8(const wchar_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    return BfxTranscodeUTF32ToUTF8((const char32_t*)source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF8ToWChar(const char8_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen) {
    return BfxTranscodeUTF8ToUTF32(source, sourceLen, (char32_t*)dest, destLen);
}

#endif // BFX_WCHAR_T_UTF8

#if defined(BFX_OS_WIN)
BFX_API(int) BfxTranscodePathToUTF8(const BfxPathCharType* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    return BfxTranscodeUTF16ToUTF8(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF8ToPath(const char8_t* source, size_t* sourceLen, BfxPathCharType* dest, size_t* destLen) {
    return BfxTranscodeUTF8ToUTF16(source, sourceLen, dest, destLen);
}

#else // POSIX 
// posix下默认路径都是utf8编码
BFX_API(int) BfxTranscodePathToUTF8(const BfxPathCharType* source, size_t* sourceLen, char8_t* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}

BFX_API(int) BfxTranscodeUTF8ToPath(const char8_t* source, size_t* sourceLen, BfxPathCharType* dest, size_t* destLen) {
    _TRANSCODE_SRC_TO_DEST(source, sourceLen, dest, destLen);
}
#endif // POSIX


