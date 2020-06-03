#pragma once
#include "./blog_config.h"
#include "./blog_global_config.h"


typedef struct _BLogStage {

    // 日志的配置
    bool on;
    int level;

    const _BLogPos* logPos;

    // 状态码
    int status;

    // 缓冲区
    char* buffer;
    size_t bufferLength;

    size_t pos;

    // 关联的配置
    _BLogIsolateConfig* isolateConfig;
    _BLogGlobalConfig* globalConfig;
} _BLogStage;

void _blogStageInit(_BLogStage* stage, int level, const _BLogPos* pos);

void _blogOutputStagePrintList(_BLogStage* stage, const char* format, va_list args);

void _blogOutputStagePrintVA(_BLogStage* stage, const char* format, ...);

// 刷新缓冲区到target
void _blogStageFlushOutput(_BLogStage* stage);

// 刷新并终结本次输出
void _blogStageFlushEnd(_BLogStage* state);