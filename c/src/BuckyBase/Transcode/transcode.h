#pragma once
#include "../Path/path.h"

// source和sourceLen: 源字符串所在缓冲区和长度(不包括字符串结尾的'\0')，source不能为NULL且必须有效，此缓冲在在转码过程中不会被修改
// dest和*destLen：目标缓冲区和目标缓冲区的最大长度，转码结束后destLen指向已转码的目标长度(不包括字符串结尾的'\0'), sourceLen指向已转码的源长度
// 注意: 转换完成后，dest不会在末尾自动添加结束符'\0'，如果需要的话要自己添加，需要注意长度不要越界！

// utf16 <-> utf8
BFX_API(int) BfxTranscodeUTF16ToUTF8(const char16_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF8ToUTF16(const char8_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen);

// utf32 <-> utf8
BFX_API(int) BfxTranscodeUTF32ToUTF8(const char32_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF8ToUTF32(const char8_t* source, size_t* sourceLen, char32_t* dest, size_t* destLen);

// utf32 <-> utf16
BFX_API(int) BfxTranscodeUTF32ToUTF16(const char32_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF16ToUTF32(const char16_t* source, size_t* sourceLen, char32_t* dest, size_t* destLen);

// wchar_t <-> utf16
BFX_API(int) BfxTranscodeWCharToUTF16(const wchar_t* source, size_t* sourceLen, char16_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF16ToWChar(const char16_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen);

// wchar_t <-> utf8
BFX_API(int) BfxTranscodeWCharToUTF8(const wchar_t* source, size_t* sourceLen, char8_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF8ToWChar(const char8_t* source, size_t* sourceLen, wchar_t* dest, size_t* destLen);

// path <-> utf8
BFX_API(int) BfxTranscodePathToUTF8(const BfxPathCharType* source, size_t* sourceLen, char8_t* dest, size_t* destLen);
BFX_API(int) BfxTranscodeUTF8ToPath(const char8_t* source, size_t* sourceLen, BfxPathCharType* dest, size_t* destLen);
