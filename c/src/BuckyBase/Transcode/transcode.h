#pragma once
#include "../Path/path.h"

// source��sourceLen: Դ�ַ������ڻ������ͳ���(�������ַ�����β��'\0')��source����ΪNULL�ұ�����Ч���˻�������ת������в��ᱻ�޸�
// dest��*destLen��Ŀ�껺������Ŀ�껺��������󳤶ȣ�ת�������destLenָ����ת���Ŀ�곤��(�������ַ�����β��'\0'), sourceLenָ����ת���Դ����
// ע��: ת����ɺ�dest������ĩβ�Զ���ӽ�����'\0'�������Ҫ�Ļ�Ҫ�Լ���ӣ���Ҫע�ⳤ�Ȳ�ҪԽ�磡

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
