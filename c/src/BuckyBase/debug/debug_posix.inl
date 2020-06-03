#include "../BuckyBase.h"

#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>

#if defined(BFX_OS_LINUX) || defined(BFX_OS_ANDROID)

#include <stdlib.h>

/*
Name:   cat
Umask:  0022
State:  R (running)
Tgid:   20675
Ngid:   0
Pid:    20675
PPid:   20594
TracerPid:      1214
Uid:    0       0       0       0
Gid:    0       0       0       0
*/
static bool _checkTracerPid(const char* val) {
    bool valid = false;

    int count = 0;
    sds* ret = sdssplitargs(val, &count);
    for (int i = 0; i < count; ++i) {
        if (strcmp(ret[i], "TracerPid") == 0) {
            i += 2;
            if (i >= count) {
                break;
            }

            valid = strcmp(ret[i], "0") != 0;
            break;
        }
    }

    sdsfreesplitres(ret, count);

    return valid;
}

static bool _beingDebugged() {
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd < 0) {
        return false;
    }

    // 只读取前1024个字节就足够了
    char buf[1024];

    size_t readCount = BFX_HANDLE_EINTR(read(fd, buf, sizeof(buf) - 1));
    if (BFX_HANDLE_EINTR(close(fd)) < 0) {
        return false;
    }

    if (readCount <= 0) {
        return false;
    }

    buf[readCount] = '\0';
    return _checkTracerPid(buf);
}
#endif

#if defined(BFX_ARCH_CPU_ARM_FAMILY)
#   if defined(BFX_ARCH_CPU_32_BITS)
#       define DEBUG_BREAK_ASM() asm("bkpt 0")
#   else // 64bits
#       define DEBUG_BREAK_ASM() asm("brk 0")
#   endif // 64bits
#elif defined(BFX_ARCH_CPU_MIPS_FAMILY)
#   define DEBUG_BREAK_ASM() asm("break 2")
#elif defined(BFX_ARCH_CPU_X86_FAMILY)
#   define DEBUG_BREAK_ASM() asm("int3")
#else
#   error "unsupport cpu"
#endif // cpu arch

static void _debugBreak() {
#if defined(NDEBUG) && !defined(BFX_OS_MACOSX) && !defined(BFX_OS_ANDROID)
    abort();
#elif !defined(BFX_OS_MACOSX)
    if (!_beingDebugged()) {
        abort();
    } else {
        DEBUG_BREAK_ASM();
    }
#else // other
    DEBUG_BREAK_ASM();
#endif // other
}

static int _getModulePath(const void* addr, BfxPathCharType* path, size_t size) {
    
    Dl_info info;
    int ret = dladdr(addr, &info);
    BLOG_CHECK(ret != 0);
    if (ret == 0) {
        BLOG_ERROR("dladdr faield!");
        return 0;
    }

    const size_t len = strlen(info.dli_fname);
    size_t retLen = min(size - 1, len);
    strncpy(path, info.dli_fname, retLen);

    path[retLen] = BFX_PATH_LITERAL('\0');

    return retLen;
}

#if defined(BFX_OS_MACOSX)
#include <mach/mach.h>
#include <mach-o/dyld.h>

int _getExecutablePath(BfxPathCharType* path, size_t size) {
    uint32_t len = (uint32_t)size - 1;
    int ret = _NSGetExecutablePath(path, &len);
    if (ret <= 0) {
        BLOG_ERROR("call _NSGetExecutablePath failed! ret=%d", ret);
        return ret;
    }

    BLOG_CHECK(len > 0);
    path[len] = BFX_PATH_LITERAL('\0');

    return len;
}
#elif defined(BFX_OS_ANDROID)

int _getExecutablePath(BfxPathCharType* path, size_t size) {
    int ret = readlink("/proc/self/exe", path, size - 1);
    if (ret <= 0) {
        BLOG_ERROR("failed to resolve proc exe path!");
        return ret;
    }

    path[ret] = BFX_PATH_LITERAL('\0');

    return ret;
}

#elif defined(BFX_OS_LINUX)

int _getExecutablePath(BfxPathCharType* path, size_t size) {
    ssize_t ret = readlink("/proc/self/exe", path, size - 1);
    if (ret <= 0) {
        BLOG_ERROR("failed to resolve proc exe path!");
        return ret;
    }

    path[ret] = BFX_PATH_LITERAL('\0');
    
    return ret;
}

#elif defined(BFX_OS_FREEBSD)

int _getExecutablePath(BfxPathCharType* path, size_t size) {

    const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };

    // Upon return, |size| is the number of bytes written to |path|
    // including the string terminator.
    size = size - 1;
    int error = sysctl(name, 4, path, &size, NULL, 0);
    if (error < 0 || size <= 1) {
        BLOG_ERROR("failed to resolve proc exe path!!!");
        return 0;
    }

    path[size] = BFX_PATH_LITERAL('\0');

    return size;
}

#else

#error "_getExecutablePath not impl!!!"

#endif // XPF_OS_FREEBSD