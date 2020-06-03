#include "../BuckyBase.h"
#include "./blog_print.h"
#include "./blog_stage.h"
#include <ctype.h>
#include <stdarg.h>

void _blogPosInit(_BLogPos* pos, const char* file, int line, const char* func, int level) {
    pos->file = file;
    pos->line = line;
    pos->functionName = func;

    if (level >= BLogLevel_Warn) {
        pos->cerr = errno;
#if defined(BFX_OS_WIN)
        pos->lastError = GetLastError();
#endif // XPF_OS_WIN
    } else {
        pos->cerr = 0;
#if defined(BFX_OS_WIN)
        pos->lastError = 0;
#endif // XPF_OS_WIN
    }
}

void _blogPrintf(_BLogIsolateConfig* config, int level, const _BLogPos* pos, const char* format, ...) {
    _BLogStage stage;

    stage.globalConfig = _blogGetGlobalConfig();
    stage.isolateConfig = config;
    _blogStageInit(&stage, level, pos);

    va_list args;
    va_start(args, format);
    _blogOutputStagePrintList(&stage, format, args);
    va_end(args);

    _blogStageFlushEnd(&stage);

    /* 如果发生了错误，并且在调试，那么触发中断 */
    if (stage.status && BfxIsBeingDebugged()) {
        BFX_DEBUGBREAK();
    }
}

static char _blogHexTransByte(int ch) {
    return isprint(ch) ? ch : '.';
}

extern const char* _blogLineBreak;

static void _blogHexPrintfLine(_BLogStage* stage, const char* address) {
    const uint8_t* buffer = (uint8_t*)address;

    _blogOutputStagePrintVA(stage, "[0x%p]"
        "\t%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
        " %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
        "%3s%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s",
        address,
        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],
        buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15],
        buffer[16], buffer[17], buffer[18], buffer[19], buffer[20], buffer[21], buffer[22], buffer[23],
        buffer[24], buffer[25], buffer[26], buffer[27], buffer[28], buffer[29], buffer[30], buffer[31],
        "",
        _blogHexTransByte(buffer[0]), _blogHexTransByte(buffer[1]), _blogHexTransByte(buffer[2]), _blogHexTransByte(buffer[3]),
        _blogHexTransByte(buffer[4]), _blogHexTransByte(buffer[5]), _blogHexTransByte(buffer[6]), _blogHexTransByte(buffer[7]),
        _blogHexTransByte(buffer[8]), _blogHexTransByte(buffer[9]), _blogHexTransByte(buffer[10]), _blogHexTransByte(buffer[11]),
        _blogHexTransByte(buffer[12]), _blogHexTransByte(buffer[13]), _blogHexTransByte(buffer[14]), _blogHexTransByte(buffer[15]),
        _blogHexTransByte(buffer[16]), _blogHexTransByte(buffer[17]), _blogHexTransByte(buffer[18]), _blogHexTransByte(buffer[19]),
        _blogHexTransByte(buffer[20]), _blogHexTransByte(buffer[21]), _blogHexTransByte(buffer[22]), _blogHexTransByte(buffer[23]),
        _blogHexTransByte(buffer[24]), _blogHexTransByte(buffer[25]), _blogHexTransByte(buffer[26]), _blogHexTransByte(buffer[27]),
        _blogHexTransByte(buffer[28]), _blogHexTransByte(buffer[29]), _blogHexTransByte(buffer[30]), _blogHexTransByte(buffer[31]),
        _blogLineBreak
    );
}

static void _blogHexPrintfSeg(_BLogStage* stage, const char* address, size_t len) {
    assert(len < 32);
    const uint8_t* buffer = (uint8_t*)address;


    _blogOutputStagePrintVA(stage, "[0x%p]\t", address);

    for (size_t i = 0; i < len; ++i) {
        _blogOutputStagePrintVA(stage, "%02x ", buffer[i]);
    }

    if (len > 0) {
        char padding[32];
        snprintf(padding, 32, "%%%ds", (32 - (int)len) * 3 + 2);
        _blogOutputStagePrintVA(stage, padding, "");
    }

    for (size_t i = 0; i < len; ++i) {
        _blogOutputStagePrintVA(stage, "%c", _blogHexTransByte(buffer[i]));
    }

    _blogOutputStagePrintVA(stage, "%s", _blogLineBreak);
}

static void _blogHexPrintfBlock(_BLogStage* stage, const char* address, size_t len) {
    size_t fullLine = len / 32;
    for (size_t i = 0; i < fullLine; ++i) {
        _blogHexPrintfLine(stage, address + i * 32);
    }

    _blogHexPrintfSeg(stage, address + fullLine * 32, len % 32);
}

void _blogHexPrintf(_BLogIsolateConfig* config, int level, const _BLogPos* pos,
    const void* addr, size_t bytes, const char* format, ...) {

    _BLogStage stage;
    stage.globalConfig = _blogGetGlobalConfig();
    stage.isolateConfig = config;
    _blogStageInit(&stage, level, pos);

    // 尝试输出自定义内容
    va_list args;
    va_start(args, format);
    _blogOutputStagePrintList(&stage, format, args);
    va_end(args);

    // 输出一条开始语句
    _blogOutputStagePrintVA(&stage, "%s>>>>>>>>>>>>>>>> will dump memory, address=%p, length=%d%s", _blogLineBreak, addr, bytes, _blogLineBreak);

    // 输出内存
    _blogHexPrintfBlock(&stage, addr, bytes);

    _blogOutputStagePrintVA(&stage, "<<<<<<<<<<<<<<<< end dump memory, address=%p, length=%d", addr, bytes);

    _blogStageFlushEnd(&stage);
}