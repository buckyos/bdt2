#include "../BuckyBase.h"
#include "./blog_stage.h"
#include "./blog_format.inl"
#include "./blog_thread_buffer.inl"

#include <stdarg.h>

static bool _blogStageInitBuffer(_BLogStage* stage) {
    assert(stage->bufferLength > 0);

    stage->buffer = (char*)_blogGetThreadBuffer(&stage->bufferLength);
    return (stage->buffer != NULL);
}

static bool _blogStageGrowBuffer(_BLogStage* stage) {

    size_t len = stage->bufferLength * 2;
    char* newBuffer = _blogGetThreadBuffer(&len);
    if (newBuffer == NULL) {
        return false;
    }

    stage->buffer = newBuffer;
    stage->bufferLength = len;
 
    return true;
}

static bool _blogStageIsOutOfRange(int ret, size_t bufferLen) {
    bool outRange = false;
#ifdef BFX_COMPILER_MSVC
    // msvc运行库的snprintf基于_s版本实现，所以count使用了_TRUNCATE
    // 这种情况下输出阶段后返回-1但是errno不被修改
    if (ret == -1 && ((errno == ERANGE) || (errno == 0))) {
        outRange = true;
    }
#else // GCC
    // Thus, a return value of size or more means that the output was truncated. 
    // If an output error is encountered, a negative value is returned.
    if (ret >= (int)bufferLen) {
        outRange = true;
    }
#endif // GCC

    return outRange;
}

// 获取当前buffer，用以循环输出
static char* _blogStageCalcBuffer(_BLogStage* stage, size_t* bufferLen) {
    assert(stage->pos < stage->bufferLength);

    if (stage->buffer == NULL) {
        _blogStageGrowBuffer(stage);
        if (stage->buffer == NULL) {
            return NULL;
        }
    }

    // 预留一个尾字符位置
    *bufferLen = stage->bufferLength - stage->pos - 1;
    return (stage->buffer + stage->pos);
}


void _blogOutputStagePrintList(_BLogStage* stage, const char* format, va_list args) {
    if (!stage->on) {
        return;
    }

    for (;;) {
        size_t bufferLen = 0;
        char* buffer = _blogStageCalcBuffer(stage, &bufferLen);
        if (buffer == NULL) {
            break;
        }

        va_list ap;
        va_copy(ap, args);

#ifdef BFX_COMPILER_MSVC
        errno = 0;
#endif // BFX_COMPILER_MSVC
        int ret = vsnprintf(buffer, bufferLen, format, ap);
        va_end(ap);

        if (ret >= 0 && ret < (int)bufferLen) {
            stage->pos += ret;
            break;
        }

        buffer[0] = 0;
        if (!_blogStageIsOutOfRange(ret, bufferLen)) {
            break;
        }

        if (!_blogStageGrowBuffer(stage)) {
            break;
        }
    }
}

void _blogOutputStagePrintVA(_BLogStage* stage, const char* format, ...) {
    if (!stage->on) {
        return;
    }

    va_list list;
    va_start(list, format);

    _blogOutputStagePrintList(stage, format, list);

    va_end(list);
}

static void _blogStageAddHeaders(_BLogStage* stage) {
    
    BfxSystemTime local;
    BfxTimeToSystemTime(BfxTimeGetNow(false), &local, true);

    _blogOutputStagePrintVA(stage, "[%s],[%s],[%04d-%02d-%02d %02d:%02d:%02d.%03d],[%d:%d],[%s] ", 
        stage->isolateConfig->name,
        _blogConfigGetLevelTag(stage->isolateConfig, stage->level),
        local.year, local.month, local.dayOfMonth, local.hour, local.minute, local.second, local.millisecond,
        BfxGetCurrentProcessID(), BfxGetCurrentThreadID(),
        stage->logPos->functionName);
}

extern char* _threadTranscodeWCharToUTF8(const wchar_t* source, size_t len, size_t* outLen);

static void _blogStageAddErrno(_BLogStage* stage) {
    if (stage->logPos->cerr != 0) {
        // C的errno
        _blogOutputStagePrintVA(stage, ", errno=%d(%s) ", stage->logPos->cerr, strerror(stage->logPos->cerr));
    }
    
#if defined(BFX_OS_WIN)
    if (stage->logPos->lastError != 0) {
        wchar_t* winErr = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, stage->logPos->lastError, 0, (LPWSTR)&winErr, 0, NULL);
        if (winErr) {
            size_t len = wcslen(winErr);
            while (len > 0 && (winErr[len - 1] == L'\r' || winErr[len - 1] == L'\n')) {
                --len;
            }

            size_t errLen = 0;
            const char* err = _threadTranscodeWCharToUTF8(winErr, len, &errLen);
            _blogOutputStagePrintVA(stage, ", lasterror=%d(%s) ", stage->logPos->lastError, err);

            LocalFree(winErr);
        }
    }
#endif // BFX_OS_WIN
}

static void _blogStageAddFileline(_BLogStage* stage) { 
    _blogOutputStagePrintVA(stage, ", %s(%d)", stage->logPos->file, stage->logPos->line);
}

static void _blogStageAddTails(_BLogStage* stage) {

    if (_blogConfigGetErrno(stage->isolateConfig, stage->level)) {
        _blogStageAddErrno(stage);
    }

    if (_blogConfigGetFileLine(stage->isolateConfig, stage->level)) {
        _blogStageAddFileline(stage);
    }

    // 添加换行符
    _blogOutputStagePrintVA(stage, "%s", _blogLineBreak);
}

// 刷新缓冲区到target
void _blogStageFlushOutput(_BLogStage* stage) {
    assert(stage->pos <= stage->bufferLength);
    if (stage->pos > 0) {
        stage->buffer[stage->pos] = '\0';
        _blogGlobalConfigOutputLine(_blogGetGlobalConfig(), stage->isolateConfig->targets, stage->buffer, stage->pos);
    }

    stage->pos = 0;
    stage->buffer[0] = '\0';
}

// 刷新并终结本次输出
void _blogStageFlushEnd(_BLogStage* stage) {
    _blogStageAddTails(stage);

    _blogStageFlushOutput(stage);
}

void _blogStageInit(_BLogStage* stage, int level, const _BLogPos* pos) {

    stage->on = true;
    stage->level = level;
    stage->logPos = pos;

    stage->status = 0;

    stage->bufferLength = 4096;
    stage->buffer = NULL;
    stage->pos = 0;

    _blogStageInitBuffer(stage);

    _blogStageAddHeaders(stage);
}