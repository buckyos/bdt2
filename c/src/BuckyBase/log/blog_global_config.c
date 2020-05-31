#include "../BuckyBase.h"
#include "./blog_global_config.h"
#include "./blog_console.inl"
#include "./blog_file.h"
#include "./blog_thread.h"


static void _configAddTarget(_BLogGlobalConfig* config, int type, BfxLogTargetTargetOutput target, BfxUserData userData) {
    assert(config->targetCount <= sizeof(config->targets));
    if (config->targetCount > sizeof(config->targets)) {
        return;
    }

    const BLogTarget item = {
        .type = type,
        .output = target,
        .userData = userData
    };
    config->targets[config->targetCount++] = item;
}

//////////////////////////////////////////////////////////////////////////
// 添加默认的调试器/控制台输出
static int _defaultConsoleTarget(void* ud, const char* log, size_t len) {
    int type = (int)(intptr_t)ud;

    _blogConsoleOutput(type, log, len);

    return 0;
}

static void _enableDefaultConsoleTarget(_BLogGlobalConfig* config, int type) {
    assert(type != 0);

    int ctype = 0;
    if (type | BLogTargetType_Console) {
        ctype |= _BLogOutputType_Console;
    }
    if (type | BLogTargetType_Debugger) {
        ctype |= _BLogOutputType_Debugger;
    }

    BfxUserData udata = {
        .userData = (void*)(intptr_t)ctype,
    };
    _configAddTarget(config, type, _defaultConsoleTarget, udata);
}

//////////////////////////////////////////////////////////////////////////
// 全局配置文件

static bool _blogGlobalConfigFindConfigFile(BfxPathCharType* path, size_t size) {

    // TODO: 目前全局配置只支持exe目录的静态配置
    BfxPathCharType temp[BFX_PATH_MAX];
    int ret = BfxGetExecutablePath(temp, BFX_PATH_MAX);
    assert(ret > 0);

    BfxPathStringCat(temp, BFX_PATH_MAX, BFX_PATH_LITERAL(".blog.cfg"));

    if (!BfxPathFileExists(temp)) {
        return false;
    }

    assert(size == BFX_PATH_MAX);
    BfxPathJoin(path, temp, NULL);
    return true;
}

//////////////////////////////////////////////////////////////////////////
// file target

static int _defaultFileTarget(void* ud, const char* log, size_t len) {
    _BLogFile* file = (_BLogFile*)ud;

    return _blogFileOutput(file, log, len);
}

static int _defaultFileThreadTarget(void* ud, const char* log, size_t len) {
    _BLogThread* thread = (_BLogThread*)ud;

    return _blogThreadOutput(thread, log, len);
}

static void _addFileTarget(_BLogGlobalConfig* gconfig, const char* filepath, size_t maxCount, size_t maxSize, int64_t flushInterval) {

    const BfxPathCharType* param = NULL;
    BfxPathCharType temp[BFX_PATH_MAX];
    if (filepath) {
        size_t sourceLen = strlen(filepath);
        size_t destLen = BFX_ARRAYSIZE(temp);
        int ret = BfxTranscodeUTF8ToPath(filepath, &sourceLen, temp, &destLen);
        if (ret == 0) {
            temp[destLen] = BFX_PATH_LITERAL('\0');
            param = temp;
        }
    }

    _BLogFile* file = _newBLogFile(param, maxCount, maxSize);

    // TODO 对同步模式的直接支持

    _BLogThread* fileThread = _newBLogThread(file, flushInterval);
    BfxUserData udata = {
        .userData = fileThread,
    };
    _configAddTarget(gconfig, BLogTargetType_File, _defaultFileThreadTarget, udata);
}

static void _parseFileTarget(_BLogGlobalConfig* gconfig, cJSON* fileDoc) {
    size_t maxCount = 0;
    size_t maxSize = 0;
    int64_t flushInterval = 0;
    const char* filepath = NULL;

    bool on = true;
    const int size = cJSON_GetArraySize(fileDoc);
    for (int i = 0; i < size; ++i) {
        cJSON* item = cJSON_GetArrayItem(fileDoc, i);
        assert(item->string);
        if (item->string == NULL) {
            continue;
        }
        if (BfxStricmp(item->string, "on") == 0) {
            assert(item->type == cJSON_Number);
            on = !!item->valueint;
        } else if (BfxStricmp(item->string, "count") == 0) {
            assert(item->type == cJSON_Number);
            maxCount = (size_t)item->valueint;
        } else if (BfxStricmp(item->string, "size") == 0) {
            assert(item->type == cJSON_Number);
            maxSize = (size_t)item->valueint;
        } else if (BfxStricmp(item->string, "file") == 0) {
            assert(item->type == cJSON_String);
            filepath = item->valuestring;
        } else if (BfxStricmp(item->string, "flushInterval") == 0) {
            assert(item->type == cJSON_Number);
            flushInterval = item->valueint;
        }
    }
    
    if (on) {
        _addFileTarget(gconfig, filepath, maxCount, maxSize, flushInterval);
    }
}

// 加载全局配置文件
static bool _blogGlobalConfigLoad(_BLogGlobalConfig* gconfig) {
    BfxPathCharType configFile[BFX_PATH_MAX];
    bool find = _blogGlobalConfigFindConfigFile(configFile, BFX_PATH_MAX);
    if (!find) {
        return false;
    }

    // 加载json格式的配置文件
    cJSON* config = BfxLoadJson(configFile);
    if (!config) {
        return false;
    }

    int consoleTarget = BLogTargetType_Console | BLogTargetType_File;
    bool defaultFileTarget = true;
    const int size = cJSON_GetArraySize(config);
    for (int i = 0; i < size; ++i) {
        cJSON* item = cJSON_GetArrayItem(config, i);
        assert(item->string);
        if (item->string == NULL) {
            continue;
        }

        if (BfxStricmp(item->string, "debugger") == 0) {
            assert(item->type == cJSON_Number);
            if (!item->valueint) {
                consoleTarget &= ~BLogTargetType_Console;
            }

        } else if (BfxStricmp(item->string, "console") == 0) {
            assert(item->type == cJSON_Number);
            assert(item->type == cJSON_Number);
            if (!item->valueint) {
                consoleTarget &= ~BLogTargetType_File;
            }
        } else if (BfxStricmp(item->string, "file") == 0) {
            assert(item->type == cJSON_Object);
            _parseFileTarget(gconfig, item);
            defaultFileTarget = false;
        }
    }

    if (consoleTarget) {
        _enableDefaultConsoleTarget(gconfig, consoleTarget);
    }

    // 如果没有配置file字段，那么添加默认的filetarget
    if (defaultFileTarget) {
        _addFileTarget(gconfig, NULL, 0, 0, 0);
    }

    return true;
}

// 默认初始化
static void _blogGlobalConfigDefaultInit(_BLogGlobalConfig* gconfig) {

#ifdef _DEBUG
    int consoleType = _BLogOutputType_Console | _BLogOutputType_Debugger;
    _enableDefaultConsoleTarget(gconfig, consoleType);
#endif // _DEBUG

    _addFileTarget(gconfig, NULL, 0, 0, 0);
}

// 全局唯一
static _BLogGlobalConfig _blogGlobalConfigInstance;
static uv_once_t _blogGlobalConfigInstanceInit = UV_ONCE_INIT;

static void _initBlogGlobalConfigInstance(void) {
    
    memset(&_blogGlobalConfigInstance, 0, sizeof(_BLogGlobalConfig));

    // 加载配置文件
    bool ret = _blogGlobalConfigLoad(&_blogGlobalConfigInstance);
    if (!ret) {

        // 默认初始化
        _blogGlobalConfigDefaultInit(&_blogGlobalConfigInstance);
    }
}

// 获取唯一的全局配置
_BLogGlobalConfig* _blogGetGlobalConfig() {
    uv_once(&_blogGlobalConfigInstanceInit, _initBlogGlobalConfigInstance);
    return &_blogGlobalConfigInstance;
}

void _blogGlobalConfigOutputLine(_BLogGlobalConfig* config, int targetType, const char* value, size_t len) {
    for (int i = 0; i < config->targetCount; ++i) {
        const BLogTarget* target = &config->targets[i];
        if (target->type & targetType) {
            config->targets[i].output(target->userData.userData, value, len);
        }
    }
}

void BLogAddTarget(int type, BfxLogTargetTargetOutput target, BfxUserData userData) {
    assert(type != 0);
    assert(target);

    _configAddTarget(_blogGetGlobalConfig(), type, target, userData);
}