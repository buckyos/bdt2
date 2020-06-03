#include "../BuckyBase.h"
#include "./blog_config.h"
#include "./blog_global_config.h"
#include "./blog_stage.h"


const char* _blogConfigGetLevelTag(_BLogIsolateConfig* config, int8_t level) {
    assert(level >= BLogLevel_Verbose && level <= BLogLevel_Off);

    return config->levelTags[level];
}

static int _blogConfigGetLevel(_BLogIsolateConfig* config, const char* levelTag) {
    for (int i = 0; i < BFX_ARRAYSIZE(config->levelTags); ++i) {
        if (BfxStricmp(config->levelTags[i], levelTag) == 0) {
            return i;
        }
    }

    return 0;
}

bool _blogConfigGetErrno(_BLogIsolateConfig* config, int level) {
    return (config->err && (level >= BLogLevel_Warn && level < BLogLevel_Off));
}

bool _blogConfigGetFileLine(_BLogIsolateConfig* config, int level) {
    return (config->fileline);
}

bool _blogConfigCheckSwitch(_BLogIsolateConfig* config, int level) {
    if (!config->on) {
        return false;
    }

    if (!config->levels[level]) {
        return false;
    }

    return true;
}

static void _blogConfigInitLevelTags(_BLogIsolateConfig* config) {
    config->levelTags[BLogLevel_Verbose] = "verbose";

    config->levelTags[BLogLevel_Trace] = "trace";
    config->levelTags[BLogLevel_Trace + 1] = "trace#2";
    config->levelTags[BLogLevel_Trace + 2] = "trace#3";

    config->levelTags[BLogLevel_Debug] = "debug";
    config->levelTags[BLogLevel_Debug + 1] = "debug#2";
    config->levelTags[BLogLevel_Debug + 2] = "debug#3";

    config->levelTags[BLogLevel_Info] = "info";
    config->levelTags[BLogLevel_Info + 1] = "info#2";
    config->levelTags[BLogLevel_Info + 2] = "info#3";

    config->levelTags[BLogLevel_Scope] = "scope";
    config->levelTags[BLogLevel_Scope + 1] = "scope#2";
    config->levelTags[BLogLevel_Scope + 2] = "scope#3";

    config->levelTags[BLogLevel_Warn] = "warn";
    config->levelTags[BLogLevel_Warn + 1] = "warn#2";
    config->levelTags[BLogLevel_Warn + 2] = "warn#3";

    config->levelTags[BLogLevel_Check] = "check";
    config->levelTags[BLogLevel_Error] = "error";
    config->levelTags[BLogLevel_Fatal] = "fatal";
}


static void _blogConfigSetLevel(_BLogIsolateConfig* config, int8_t level) {
    assert(level >= BLogLevel_Verbose && level <= BLogLevel_Off);

    for (int8_t i = BLogLevel_Verbose; i <= BLogLevel_Off; ++i) {
        config->levels[i] = (level <= i);
    }
}

static bool _blogConfigFindConfigFile(_BLogIsolateConfig* config, BfxPathCharType* path, size_t size) {
    
    // TODO: 目前只支持exe目录对应name的静态配置
    BfxPathCharType temp[BFX_PATH_MAX];
    int ret = BfxGetExecutablePath(temp, BFX_PATH_MAX);
    assert(ret > 0);

    BfxPathRemoveFileSpec(temp);
    
    size_t destLen = sizeof(config->name) * 2 + 1;
    BfxPathCharType name[sizeof(config->name) * 2 + 1];

    size_t srcLen = strlen(config->name);
    BfxTranscodeUTF8ToPath(config->name, &srcLen, name, &destLen);
    name[destLen] = BFX_PATH_LITERAL('\0');

    BfxPathJoin(temp, temp, name);
    BfxPathStringCat(temp, BFX_PATH_MAX, BFX_PATH_LITERAL(".blog.cfg"));

    if (!BfxPathFileExists(temp)) {
        return false;
    }

    BfxPathJoin(path, temp, NULL);
    return true;
}

// 加载全局配置文件
static void _blogConfigLoad(_BLogIsolateConfig* config) {
    BfxPathCharType configFile[BFX_PATH_MAX];
    bool find = _blogConfigFindConfigFile(config, configFile, BFX_PATH_MAX);
    if (!find) {
        return;
    }

    // 加载json格式的配置文件
    cJSON* doc = BfxLoadJson(configFile);
    assert(doc);
    if (!doc) {
        return;
    }

    const int size = cJSON_GetArraySize(doc);
    for (int i = 0; i < size; ++i) {
        cJSON* item = cJSON_GetArrayItem(doc, i);
        assert(item->string);
        if (item->string == NULL) {
            continue;
        }

        if (BfxStricmp(item->string, "on") == 0) {
            assert(item->type == cJSON_Number);
            config->on = !!item->valueint;
        } else if (BfxStricmp(item->string, "console") == 0) {
            assert(item->type == cJSON_Number);
            if (item->valueint) {
                config->targets |= BLogTargetType_Console;
            } else {
                config->targets &= ~BLogTargetType_Console;
            }
        } else if (BfxStricmp(item->string, "debugger") == 0) {
            assert(item->type == cJSON_Number);
            if (item->valueint) {
                config->targets |= BLogTargetType_Debugger;
            } else {
                config->targets &= ~BLogTargetType_Debugger;
            }
        } else if (BfxStricmp(item->string, "file") == 0) {
            assert(item->type == cJSON_Number);
            if (item->valueint) {
                config->targets |= BLogTargetType_File;
            } else {
                config->targets &= ~BLogTargetType_File;
            }
        } else if (BfxStricmp(item->string, "level") == 0) {
            assert(item->type == cJSON_String);

            _blogConfigSetLevel(config, _blogConfigGetLevel(config, item->valuestring));
        } else if (BfxStricmp(item->string, "errno") == 0) {
            assert(item->type == cJSON_Number);
            config->err = !!item->valueint;
        } else if (BfxStricmp(item->string, "fileline") == 0) {
            assert(item->type == cJSON_Number);
            config->fileline = !!item->valueint;
        }
    }
}

static void _blogConfigInitDefault(_BLogIsolateConfig* config, const char* name) {
    strncpy(config->name, name, BFX_ARRAYSIZE(config->name));
    config->on = true;
    config->fileline = true;
    config->err = true;
    
    _blogConfigInitLevelTags(config);

    _blogConfigSetLevel(config, BLogLevel_Debug);

    // TODO 根据编译版本来确定默认的targets
    config->targets = BLogTargetType_Console | BLogTargetType_Debugger | BLogTargetType_File;
}

void _blogConfigInit(_BLogIsolateConfig* config, const char* name) {
    memset(config, 0, sizeof(_BLogIsolateConfig));

    _blogConfigInitDefault(config, name);

    _blogConfigLoad(config);
}