#include "../BuckyBase.h"
#include "./path.h"

#if defined(BFX_OS_WIN)
#pragma comment(lib, "Shlwapi.lib")
#endif // BFX_OS_WIN

// 判断一个字符是不是路径分割符，分严格和通用模式
#if defined(BFX_OS_WIN)
static bool _isPathSeparator(BfxPathCharType ch, bool strict) {
    if (strict) {
        return ch == BFX_PATH_WINDOW_NORMAL_SEPARATOR;
    }
    else {
        return ch == BFX_PATH_WINDOW_NORMAL_SEPARATOR
            || ch == BFX_PATH_POSIX_SEPARATOR;
    }
}
#else // posix
static bool _isPathSeparator(BfxPathCharType ch, bool strict) {
    return ch == BFX_PATH_POSIX_SEPARATOR;
}
#endif // posix



BFX_API(bool) BfxPathIsRelative(const BfxPathCharType* path) {
    if (!path || !*path) {
        // 空路径认为是相对路径
        return true;
    }

    if (_isPathSeparator(path[0], true)) {
        // 以路径分隔符开始，那么要区分是win还是posix
        // 1. 如果是以该平台的标准分隔符开始，那么都是非相对路径
        // 2. 如windows下 \node \\node 都认为是非相对路径; linux下 /node认为是非相对路径
        // 3. 如果是以和标准相反的分隔符开始，那么都认为是相对路径
        return false;
    }

    if (path[0] != '\0' && path[1] == BFX_PATH_LITERAL(':')) {
        // 如果以盘符+冒号开始， 如C:\\xxx， 那么是绝对路径
        return false;
    }

    return true;
}

BFX_API(bool) BfxPathFileExists(const BfxPathCharType* path) {
#if defined(BFX_OS_WIN)
    return !!PathFileExists(path);
#else // posix
    struct stat info = { 0 };
    return stat(path, &info) == 0;
#endif // posix
}

BFX_API(bool) BfxPathIsDirectory(const BfxPathCharType* path) {
#if defined(BFX_OS_WIN)
    return !!PathIsDirectory(path);
#else // posix
    struct stat info = { 0 };
    return (stat(path, &info) == 0) && (info.st_mode & S_IFDIR);
#endif // posix
}

BFX_API(const BfxPathCharType*) BfxPathFindFileName(const BfxPathCharType* path) {
    if (path == NULL) {
        return NULL;
    }

    const BfxPathCharType* name = path;
    for (; *path; ++path) {
        if ((_isPathSeparator(path[0], false) || path[0] == BFX_PATH_LITERAL(':'))
            && path[1] && !_isPathSeparator(path[1], false)) {
            name = path + 1;
        }
    }
    
    return name;
}

BFX_API(const BfxPathCharType*) BfxPathFindExtension(const BfxPathCharType* path) {
    const BfxPathCharType* dot = NULL;

    if (path != NULL) {
        for (; *path; ++path) {
            if (*path == BFX_PATH_LITERAL('.')) {
                dot = path;
            } else if (_isPathSeparator(*path, false) || *path == BFX_PATH_LITERAL(' ')) {
                dot = NULL;
            }
        }
    }

    return dot ? dot : path;
}

BFX_API(BfxPathCharType*) BfxPathStringCat(BfxPathCharType* dest, size_t destLen, const BfxPathCharType* src) {
    size_t len = BfxPathLength(dest);

    size_t i;
    for (i = len; i < destLen; ++i) {
        dest[i] = *src++;
        if (!*src) {
            break;
        }
    }

    ++i;
    dest[min(i, destLen - 1)] = BFX_PATH_LITERAL('\0');

    return dest;
}

BFX_API(int) BfxPathCompare(const BfxPathCharType* left, const BfxPathCharType* right) {
    if (!left) {
        left = BFX_PATH_LITERAL("");
    }
    if (!right) {
        right = BFX_PATH_LITERAL("");
    }

    BfxPathCharType f = 0, l = 0;

    do {
        f = *left++;
        l = *right++;

    } while ((f) && (f == l));

    return (int)(f - l);
}

BFX_API(size_t) BfxPathLength(const BfxPathCharType* path) {
    const BfxPathCharType* ptr = path;
    while (*ptr) {
        ++ptr;
    }

    return (ptr - path);
}

BfxPathCharType* _pathCopy(BfxPathCharType* dest, const BfxPathCharType* src, size_t srcLen) {
    return (BfxPathCharType*)memcpy(dest, src, sizeof(BfxPathCharType) * srcLen);
}

BFX_API(bool) BfxPathIsRoot(const BfxPathCharType* path) {
    if (path || !*path) {
        return false;
    }

    if (_isPathSeparator(*path, false)) {

        // \\ 和 / 是根路径
        if (!path[1]) {
            return true;
        } else if (_isPathSeparator(path[1], false)) {
            // 连续两个路径分隔符，那么判断是否是UNC Share根目录，形如\\server\share，
            // 但是\\server\, \\server\share\cc 则不属于
            int count = 0;
            path += 2;

            for (; *path; ++path) {
                if (_isPathSeparator(*path, false) && ++count > 1) {
                    return false;
                }
            }

            // 须排除以\结尾的情况
            --path;
            if (_isPathSeparator(*path, false)) {
                return false;
            }

            return true;
        }
    } else if (path[1] == BFX_PATH_LITERAL(':') && _isPathSeparator(path[2], false) && path[3] == BFX_PATH_LITERAL('\0')) {
        // 盘符开始的根路径，如 C:\\ 
        return true;
    }

    return false;
}

BFX_API(BfxPathCharType*) BfxPathAddBackslash(BfxPathCharType* path, size_t size){
    assert(path);

    size_t len = BfxPathLength(path);
    if (len >= size) {
        return NULL;
    }

    if (len > 0) {
        path += len;
        if (!_isPathSeparator(path[-1], false)) {
            *path++ = BFX_PATH_SEPARATOR;
            *path = BFX_PATH_LITERAL('\0');
        }
    }

    return path;
}

BFX_API(bool) BfxPathIsUNC(const BfxPathCharType* path) {
    return (path != NULL && _isPathSeparator(path[0], false) && _isPathSeparator(path[1], false));
}

BFX_API(bool) BfxPathIsUNCServerShare(const BfxPathCharType* path) {

    if (!path || !*path) {
        return false;
    }

     // 形如 //server/share
    if (_isPathSeparator(path[0], false) && _isPathSeparator(path[1], false)) {
        bool got = false;
        while (*path) {
            if (_isPathSeparator(*path, false)) {
                if (got) {
                    return false;
                }

                got = true;
            }
            path++;
        }

        return got;
    }

    return false;
}

BFX_API(bool) BfxPathStripToRoot(BfxPathCharType* path) {
    if (!path) {
        return true;
    }

    while (!BfxPathIsRoot(path)) {
        if (!BfxPathRemoveFileSpec(path)) {
            return false;
        }
    }

    return true;
}

BFX_API(BfxPathCharType*) BfxPathJoin(BfxPathCharType* dest, const BfxPathCharType* dirname, const BfxPathCharType* filename) {

    if (dest && (dirname || filename)) {
        BfxPathCharType szTemp[BFX_PATH_MAX] = { 0 };
        BfxPathCharType* ptr;

        size_t dirLen = dirname ? BfxPathLength(dirname) : 0;
        size_t fileLen = filename ? BfxPathLength(filename) : 0;

        if (!filename || *filename == BFX_PATH_LITERAL('\0')) {
            _pathCopy(szTemp, dirname, min(BFX_PATH_MAX, dirLen));
        } else if (dirname && *dirname && BfxPathIsRelative(filename)) {
            _pathCopy(szTemp, dirname, min(BFX_PATH_MAX, dirLen));
            ptr = BfxPathAddBackslash(szTemp, BFX_PATH_MAX);
            if (ptr != NULL) {
                size_t iLen = BfxPathLength(szTemp);

                if ((iLen + fileLen) < BFX_PATH_MAX) {
                    _pathCopy(ptr, filename, fileLen);
                } else {
                    return NULL;
                }
            } else {
                return NULL;
            }
        } else if (dirname && *dirname && _isPathSeparator(*filename, false) && !BfxPathIsUNC(filename)) {
            _pathCopy(szTemp, dirname, min(BFX_PATH_MAX, dirLen));
           
            BfxPathStripToRoot(szTemp);

            ptr = BfxPathAddBackslash(szTemp, BFX_PATH_MAX);
            if (ptr) {
           
                _pathCopy(ptr, filename + 1, min(BFX_PATH_MAX - 1 - (ptrdiff_t)(ptr - szTemp), (int)fileLen - 1));
            } else {
                return NULL; 
            }
        } else {
            _pathCopy(szTemp, filename, min(BFX_PATH_MAX, fileLen));
        }

        BfxPathCanonicalize(dest, szTemp);
    }

    return dest;
}

BFX_API(bool) BfxPathCanonicalize(BfxPathCharType* dest, const BfxPathCharType* path)
{
    BfxPathCharType* destPtr = dest;
    const BfxPathCharType* srcPtr = path;

    if (dest) {
        *destPtr = BFX_PATH_LITERAL('\0');
    }

    if (!dest || !path) {
        return false;
    }

    if (!*path) {
        *dest++ = BFX_PATH_SEPARATOR;
        *dest = BFX_PATH_LITERAL('\0');

        return true;
    }

    // 对根路径进行拷贝 C:\ \\ /等
    if (_isPathSeparator(*srcPtr, false)) {
        *destPtr++ = *srcPtr++;
    } else if (*srcPtr && srcPtr[1] == BFX_PATH_LITERAL(':')) {
        
        *destPtr++ = *srcPtr++;
        *destPtr++ = *srcPtr++;

        if (_isPathSeparator(*srcPtr, false)) {
            *destPtr++ = *srcPtr++;
        }
    }

    // 对剩余路径进行处理
    while (*srcPtr) {
        if (*srcPtr == BFX_PATH_LITERAL('.')) {
            if (_isPathSeparator(srcPtr[1], false) 
                && (srcPtr == path || _isPathSeparator(srcPtr[-1], false) || srcPtr[-1] == BFX_PATH_LITERAL(':'))
                ) {
                srcPtr += 2; /* Skip .\ */

            } else if (srcPtr[1] == BFX_PATH_LITERAL('.')
                && (destPtr == dest || _isPathSeparator(destPtr[-1], false))
                ) {
                
                if (destPtr != dest) {
                    *destPtr = BFX_PATH_LITERAL('\0');

                    if (destPtr > dest + 1 
                        && _isPathSeparator(destPtr[-1], false) 
                        &&(!_isPathSeparator(destPtr[-2], false) 
                            || destPtr > dest + 2)
                        ) {
                        if (destPtr[-2] == BFX_PATH_LITERAL(':') && (destPtr > dest + 3 || destPtr[-3] == BFX_PATH_LITERAL(':'))) {
                            destPtr -= 2;
                            while (destPtr > dest && !_isPathSeparator(*destPtr, false)) {
                                destPtr--;
                            }

                            if (_isPathSeparator(*destPtr, false)) {
                                destPtr++; /* Reset to last '\' */
                            } else {
                                destPtr = dest; /* Start path again from new root */
                            }
                        } else if (destPtr[-2] != BFX_PATH_LITERAL(':') && !BfxPathIsUNCServerShare(dest)) {
                            destPtr -= 2;
                        }
                    }

                    while (destPtr > dest && !_isPathSeparator(*destPtr, false)) {
                        destPtr--;
                    }

                    if (destPtr == dest) {
                        *destPtr++ = BFX_PATH_SEPARATOR;
                        srcPtr++;
                    }
                }

                srcPtr += 2; /* Skip .. in src path */
            } else {
                *destPtr++ = *srcPtr++;
            }
        } else {
            *destPtr++ = *srcPtr++;
        }
    }

    /* Append \ to naked drive specs */
    if (destPtr - dest == 2 && destPtr[-1] == BFX_PATH_LITERAL(':')) {
        *destPtr++ = BFX_PATH_SEPARATOR;
    }

    *destPtr++ = BFX_PATH_LITERAL('\0');

    return true;
}

BFX_API(bool) BfxPathRemoveFileSpec(BfxPathCharType* path) {

    if (!path) {
        return false;
    }


    BfxPathCharType* pos;
    BfxPathCharType* ptr = path;

    for (pos = ptr; *ptr; ++ptr) {
        // 以 \\ or / 结尾，那么只需要去掉这个分隔符
        if (_isPathSeparator(*ptr, false)) {
            pos = ptr;
        } else if (*ptr == BFX_PATH_LITERAL(':')) {

            // 需要跳过"C:\"
            if (_isPathSeparator(ptr[1], false)) {
                ++ptr;
            }
            pos = ptr + 1;
        }
    }

    if (!*pos) {
        // didn't strip anything
        return false;
    } 
    
    // \test \\test?
    if (((pos == path) && (_isPathSeparator(*pos, false))) 
        || ((pos == path + 1) && (_isPathSeparator(*pos, false) && _isPathSeparator(*path, false))))
    {
        // '\'?
        if (*(pos + 1) != BFX_PATH_LITERAL('\0')) {
            *(pos + 1) = BFX_PATH_LITERAL('\0');

            return true; 
        } else {
         
            return false;
        }
    } else {
        *pos = BFX_PATH_LITERAL('\0');
        return true;
    }
}