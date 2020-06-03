#pragma once
#include "./blog_thread_buffer.inl"

// android��ʹ��Loggingϵͳ���������̨ 
#if defined(BFX_OS_ANDROID)
#include <android/log.h>
#endif // BFX_OS_ANDROID

wchar_t* _threadTranscodeUTF8ToWChar(const char8_t* source, size_t len, size_t* outLen) {

    // utf8ת������룬���ȶ��������� 
    size_t bufLen = (len + 1) * sizeof(wchar_t);
    wchar_t* dest = (wchar_t*)_blogGetThreadBuffer(&bufLen);

    size_t destLen = bufLen / sizeof(wchar_t);
    size_t sourceLen = len;
    int ret = BfxTranscodeUTF8ToWChar(source, &sourceLen, dest, &destLen);
    assert(ret == 0);
    assert(sourceLen <= len);

    dest[destLen] = '\0';
    if (outLen) {
        *outLen = destLen;
    }

    return dest;
}

char* _threadTranscodeWCharToUTF8(const wchar_t* source, size_t len, size_t* outLen) {

    size_t bufLen = (len + 1) * 3 * sizeof(char);
    char* dest = (char*)_blogGetThreadBuffer(&bufLen);

    size_t destLen = bufLen / sizeof(char);
    size_t sourceLen = len;
    int ret = BfxTranscodeWCharToUTF8(source, &sourceLen, dest, &destLen);
    assert(ret == 0);
    assert(sourceLen <= len);

    dest[destLen] = '\0';
    if (outLen) {
        *outLen = destLen;
    }

    return dest;
}

typedef enum {
    _BLogOutputType_Console = 1 << 1,
    _BLogOutputType_Debugger = 1 << 2,
}_BLogOutputType;

static void _blogConsoleOutput(int type, const char* value, size_t len) {
#if defined(BFX_OS_WIN)

    if (len == 0) {
        len = strlen(value);
    }

    size_t destLen = 0;
    wchar_t* dest = _threadTranscodeUTF8ToWChar(value, len, &destLen);

    if (type | _BLogOutputType_Console) {
        HANDLE hHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hHandle != NULL && hHandle != INVALID_HANDLE_VALUE)
        {
            WriteConsole(hHandle, dest, (DWORD)destLen, NULL, NULL);
        }
    }

    if (type | _BLogOutputType_Debugger) {
        OutputDebugString(dest);
    }
#elif defined(BFX_OS_ANDROID)
    if (type | _BLogOutputType_Console) {
        __android_log_print(ANDROID_LOG_INFO, "blog", "%*.*s", 0, (int)len, value);
    }
#else // posix, others
    if (type | _BLogOutputType_Console) {
        printf("%*.*s", 0, (int)len, value);
    }
    else {
        // ����ƽ̨�ݲ�֧�������������
    }
#endif // platform
}
