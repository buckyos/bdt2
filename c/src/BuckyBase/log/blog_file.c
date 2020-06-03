
#include "../BuckyBase.h"

#if defined(BFX_OS_WIN)
#   define BFX_BLOG_DIR BFX_PATH_LITERAL("C:\\blog\\")
#else
#   if defined(XPF_OS_ANDROID)
#       define BFX_BLOG_DIR  BFX_PATH_LITERAL("/sdcard/tmp/blog/")
#   else // POSIX
#   define BFX_BLOG_DIR BFX_PATH_LITERAL("/tmp/blog/")
#   endif // POSIX
#endif


typedef enum {
    _BLogFileStatus_OK = 0,
    _BLogFileStatus_OpenFailed = 1,
} _BLogFileStatus;

typedef struct _BLogFile {
    BfxPathCharType filePath[BFX_PATH_MAX];
    size_t fileIndex;

    // 日志文件最大个数
    size_t maxFileCount;

    // 单个日志文件最大长度
    size_t maxFileSize;

    FILE* file;
    int status;

    // 当前文件输出的总长度
    size_t outputLen;

}_BLogFile;

// 获取index对应的文件路径
static void _genFilePath(_BLogFile* file, BfxPathCharType* out, int index) {
    assert(out);
    assert(index >= 0);

    if (index > 0) {
        char name[16];
        snprintf(name, sizeof(name), ".%02d.log", index);
        size_t sourceLen = strlen(name);

        BfxPathCharType wname[16];
        size_t wlen = BFX_ARRAYSIZE(wname);
        BfxTranscodeUTF8ToPath(name, &sourceLen, wname, &wlen);
        wname[wlen] = BFX_PATH_LITERAL('\0');
        BfxPathJoin(out, file->filePath, NULL);
        BfxPathStringCat(out, BFX_PATH_MAX, wname);
    } else {
        BfxPathJoin(out, file->filePath, NULL);
        BfxPathStringCat(out, BFX_PATH_MAX, BFX_PATH_LITERAL(".log"));
    }
}

static void _selectFileIndex(_BLogFile* file) {
 
    BfxPathCharType temp[BFX_PATH_MAX];
    time_t oldest = 0;
    for (size_t i = 0, cur = file->fileIndex; i < file->maxFileCount; ++i, ++cur) {
        cur %= file->maxFileCount;
        _genFilePath(file, temp, (int)cur);

        int ret;
#ifdef BFX_OS_WIN
        struct _stat64 fstat = { 0 };
        ret = _wstat64(temp, &fstat);
#else // POSIX
        struct stat fstat = { 0 };
        ret = stat(temp, &fstat);
#endif // POSIX

        if (ret == 0) {
            if (oldest == 0 || fstat.st_mtime < oldest) {
                oldest = fstat.st_mtime;
                file->fileIndex = cur;
            }
        } else {
            if (errno == ENOENT) {
                // 文件不存在，可以使用该名字
                file->fileIndex = cur;
                break;
            } else {
                // TODO 其余错误？
            }
        }
    }
}

#if defined(BFX_OS_WIN)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

static FILE* _openFile(const BfxPathCharType* filePath) {

    int fd = -1;
    int err = _wsopen_s(&fd, filePath, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _SH_DENYWR, _S_IREAD | _S_IWRITE);
    if (err != 0) {
        return NULL;
    }

    FILE* file = _wfdopen(fd, L"w");
    assert(file);
    if (file == NULL) {
        _close(fd);
        return NULL;
    }

    return file;
}
#else // POSIX

#include <fcntl.h>
#include <unistd.h>

static FILE* _openFile(const BfxPathCharType* filePath) {
    FILE* file = fopen(filePath, "w+");

    return file;
}
#endif // POSIX

static bool _blogFileCheckAndOpen(_BLogFile* file) {
    if (file->file) {
        return true;
    }

    // 如果已经打开失败了，那么不再尝试
    if (file->status == _BLogFileStatus_OpenFailed) {
        return false;
    }

    BfxPathCharType temp[BFX_PATH_MAX];
    _genFilePath(file, temp, (int)file->fileIndex);

    file->file = _openFile(temp);
    if (file->file == NULL) {

        // 检查一次目录
        BfxPathCharType dir[BFX_PATH_MAX];
        BfxPathJoin(dir, temp, NULL);
        BfxPathRemoveFileSpec(dir);
        int ret = BfxMakeDirNested(dir, S_IRWXU | S_IRWXG | S_IRWXO);
        if (ret == 0) {
            file->file = _openFile(temp);
        }
    }

    if (file->file == NULL) {
        file->status = _BLogFileStatus_OpenFailed;
        return false;
    }

    return true;
}

static void _blogFileClose(_BLogFile* file) {
    if (file->file) {
        fclose(file->file);
        file->file = NULL;
    }

    file->status = _BLogFileStatus_OK;
}

int _blogFileOutput(_BLogFile* file, const char* line, size_t len) {
    if (line == NULL || len == 0) {
        return 0;
    }

    if (!_blogFileCheckAndOpen(file)) {
        return -1;
    }

    assert(file->file);
    size_t ret = fwrite(line, sizeof(char), (int)len, file->file);
    assert(ret == len);

    file->outputLen += len;
    if (file->outputLen >= file->maxFileSize) {

        // 超出单个文件限制，那么向后滚动一个文件
        file->outputLen = 0;
        _blogFileClose(file);
        _selectFileIndex(file);
    }

    return 0;
}

#if defined(BFX_OS_POSIX)
static void _blogFileAccess(_BLogFile* file) {
    assert(file->file);

    BfxPathCharType temp[BFX_PATH_MAX];
    _genFilePath(file, temp, (int)file->fileIndex);

    if (access(temp, F_OK) != 0) {
        if (errno == ENOENT) {

            // linux下文件/目录可以被删除删除了，所以这种情况下需要尝试重新创建
            _blogFileClose(file);
            _blogFileCheckAndOpen(file);
        } else {
            // TODO
        }
    }
}
#endif // BFX_OS_POSIX

void _blogFileFlush(_BLogFile* file) {
    if (file->file && file->outputLen > 0) {

#ifdef BFX_OS_POSIX
        _blogFileAccess(file);
#endif // BFX_OS_POSIX

        int ret = fflush(file->file);
        assert(ret == 0);
    }
}

static void _blogInitFilePath(_BLogFile* file, const BfxPathCharType* filepath) {
    if (filepath) {
        BfxPathJoin(file->filePath, filepath, NULL);
    } else {

        // 使用默认路径和文件名
        BfxPathCharType temp[BFX_PATH_MAX];
        int ret = BfxGetExecutablePath(temp, BFX_PATH_MAX);
        assert(ret > 0);

        const BfxPathCharType* exeName = BfxPathFindFileName(temp);
        const BfxPathCharType* ext = BfxPathFindExtension(exeName);
        if (ext && ext != exeName) {
            *(BfxPathCharType*)ext = BFX_PATH_LITERAL('\0');
        }

        BfxPathJoin(file->filePath, BFX_BLOG_DIR, exeName);

        // 使用exeName作为日志文件名
        BfxPathJoin(file->filePath, file->filePath, exeName);
    }

    _selectFileIndex(file);
}

_BLogFile* _newBLogFile(const BfxPathCharType* filepath, size_t maxCount, size_t maxSize) {
    _BLogFile* file = (_BLogFile*)BfxMalloc(sizeof(_BLogFile));
    memset(file, 0, sizeof(_BLogFile));

    if (maxCount > 0) {
        file->maxFileCount = (int)maxCount;
    } else {
        file->maxFileCount = 20;
    }
    
    if (maxSize > 0) {
        file->maxFileSize = (int)maxSize;
    } else {
        file->maxFileSize = 1024 * 1024 * 10;
    }

    _blogInitFilePath(file, filepath);

    return file;
}