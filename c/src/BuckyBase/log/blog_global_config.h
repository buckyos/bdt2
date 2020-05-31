#pragma once

/*
isolate config file
{
    "debugger": 0|1,
    "console": 0|1,
    "file": {
        "on": 0|1,  // ���Թر��ļ������Ĭ��1
        "count": {int},
        "size": {int},
        "file": "C:\\blog\\test\\filename"
    }
}
*/

// output target define
typedef enum {
    BLogTargetType_Console = 1 << 1,
    BLogTargetType_Debugger = 1 << 2,
    BLogTargetType_File = 1 << 4,
} BLogTargetType;

typedef int (*BfxLogTargetTargetOutput)(void* ud, const char* log, size_t len);

typedef struct {
    int type;
    BfxUserData userData;
    BfxLogTargetTargetOutput output;
}BLogTarget;


typedef struct {

    // ��־���Ŀ��
    BLogTarget targets[8];
    int8_t targetCount;

}_BLogGlobalConfig;

_BLogGlobalConfig* _blogGetGlobalConfig(); 

void _blogGlobalConfigOutputLine(_BLogGlobalConfig* config, int targetType, const char* value, size_t len);

// ���һ���Զ���Target
void BLogAddTarget(int type, BfxLogTargetTargetOutput target, BfxUserData userData);