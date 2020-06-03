#include "../BuckyBase.h"
#include "./file.h"


BFX_API(BFX_BUFFER_HANDLE) BfxLoadFile(const BfxPathCharType* path, bool asString) {
    FILE* file = NULL;

#if defined(BFX_OS_WIN)
    int fd = -1;
    int err = _wsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
    if (err == 0)
    {
        file = _wfdopen(fd, L"rb");
    }
    //_wfopen_s(&lpFile, lpFilePath, L"rb");
#else // BFX_OS_POSIX
    file = fopen(path, "rb");
#endif // BFX_OS_POSIX

    if (file == NULL) {
        BLOG_ERROR("open file failed! path=%" BFX_PATH_LOG_FORMAT, path);
        return NULL;
    }

    BFX_BUFFER_HANDLE buffer = NULL;
    bool success = false;
    do
    {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        if (size == 0) {
            break;
        }

        buffer = BfxCreateBuffer(asString ? size + 1 : size);
        if (buffer == NULL) {
            //BLOG_ERROR("alloc buffer for file failed! file=%"BFX_PATH_LOG_FORMAT, ", size=%d", path, size);
            break;
        }

        void* data = BfxBufferGetData(buffer, NULL);
        size_t rb = fread(data, 1, size, file);
        if (rb != size) {
            BLOG_ERROR("fread error, rb=%d, size=%d", rb, size);
            break;
        }

        if (asString) {
            ((char*)data)[size] = '\0';
        }

        success = true;
    } while (0);

    if (file) {
        fclose(file);
    }

    if (!success) {
        if (buffer) {
            BfxBufferRelease(buffer);
        }

        return NULL;
    }

    return buffer;
}

BFX_API(cJSON*) BfxLoadJson(const BfxPathCharType* path) {
    BFX_BUFFER_HANDLE buffer = BfxLoadFile(path, true);
    if (buffer == NULL) {
        return NULL;
    }

    const char* data = (char*)BfxBufferGetData(buffer, NULL);
    BLOG_CHECK(data);

    cJSON* json = cJSON_Parse(data);
    if (json == NULL)
    {
        BLOG_ERROR("load json file failed!! err=%s", cJSON_GetErrorPtr());
    }

    BfxBufferRelease(buffer);

    return json;
}

#if defined(BFX_OS_WIN)

BFX_API(int) BfxMakeDir(const BfxPathCharType* filepath, int pmode) {
    // win下暂时忽略pmode

    int err = 0;
    assert(filepath);
    BOOL ret = CreateDirectory(filepath, NULL);
    if (!ret && (GetLastError() != ERROR_ALREADY_EXISTS)) {
        err = BFX_RESULT_FAILED;
        BLOG_ERROR("call CreateDirectory failed, path=%ls", filepath);
    }

    return err;
}

#else // POSIX

BFX_API(int) BfxMakeDir(const BfxPathCharType* filepath, int pmode) {
    assert(filepath);

    struct stat  st;
    int err = 0;

    if (stat(filepath, &st) != 0) {
        // 目录不存在则尝试创建，失败后需要考虑是不是同时已经被创建了
        if (mkdir(filepath, pmode) != 0 && errno != EEXIST) {
            err = BFX_RESULT_FAILED;
            BLOG_WARN("call mkdir failed, path=%s", filepath);
        }
    } else if (!S_ISDIR(st.st_mode)) {
        // 已经存在同名的文件，导致无法创建目录了
        err = BFX_RESULT_FAILED;
    }

    return err;
}
#endif // POSIX


BFX_API(int) BfxMakeDirNested(const BfxPathCharType* filepath, int pmode) {
    assert(filepath);
    if (BfxPathIsDirectory(filepath)) {
        return 0;
    }

    // 检查并递归创建子目录
    BfxPathCharType temp[BFX_PATH_MAX];
    BfxPathJoin(temp, filepath, NULL);
    if (!BfxPathRemoveFileSpec(temp)) {
        return BFX_RESULT_FAILED;
    }

    if (!BfxPathIsDirectory(temp)) {
        int ret = BfxMakeDirNested(temp, pmode);
        if (ret != 0) {
            return ret;
        }
    }

    // 子目录创建成功，尝试创建当前目录
    return BfxMakeDir(filepath, pmode);
}