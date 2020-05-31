
#pragma once

/*
isolate config file
{
    "on": 0|1,
    "console": 0|1,
    "debugger": 0|1,
    "file", 0|1,
    ""

}
*/

// ��־������
typedef enum BLogLevel {
    BLogLevel_Verbose = 0,
    BLogLevel_Trace = 1,
    BLogLevel_Debug = 4,
    BLogLevel_Info = 7,
    BLogLevel_Scope = 10,
    BLogLevel_Warn = 13,
    BLogLevel_Check = 16,
    BLogLevel_Error = 17,
    BLogLevel_Fatal = 18,

    BLogLevel_Off = 19,
} BLogLevel;


typedef struct _BLogIsolateConfig {

    // isolate name
    char name[16];

    // ��־�ܿ���
    bool on;

    // ����
    bool levels[BLogLevel_Off];
    const char* levelTags[BLogLevel_Off];

    // ���Ŀ��
    int targets;

    // �Ƿ����errno��lasterror
    bool err;

    // �Ƿ�����ļ����к�
    bool fileline;

} _BLogIsolateConfig;

const char* _blogConfigGetLevelTag(_BLogIsolateConfig* config, int8_t level);
bool _blogConfigGetErrno(_BLogIsolateConfig* config, int level);
bool _blogConfigGetFileLine(_BLogIsolateConfig* config, int level);

void _blogConfigInit(_BLogIsolateConfig* config, const char* name);

bool _blogConfigCheckSwitch(_BLogIsolateConfig* config, int level);