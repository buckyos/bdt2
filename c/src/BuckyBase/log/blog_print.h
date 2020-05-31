#pragma once

typedef struct _BLogPos {

    const char* functionName;
    const char* file;
    int line;

    // 保存的错误信息
    int cerr;

#if defined(BFX_OS_WIN)
    DWORD lastError;
#endif // BFX_OS_WIN

}_BLogPos;

void _blogPosInit(_BLogPos* pos, const char* file, int line, const char* func, int level);

void _blogPrintf(_BLogIsolateConfig* config, int level, const _BLogPos* pos, const char* format, ...);

void _blogHexPrintf(_BLogIsolateConfig* config, int level, const _BLogPos* pos,
    const void* addr, size_t bytes, const char* format, ...);