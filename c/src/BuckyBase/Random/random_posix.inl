#include "../BuckyBase.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct
{
    int fd;
}_PosixRandomFile;

static int _randomFD = -1;
static int _randomFileOpen() {

    // 从/dev/urandom文件读取
    int fd = open("/dev/urandom", O_RDONLY);
    BLOG_CHECK(fd >= 0);
    if (fd < 0)
    {
        BLOG_ERROR("open /dev/urandom failed!!");
    }

    return fd;
}

static uv_once_t randomFileInstance = UV_ONCE_INIT;

static void _randomFileInit(void) {
    BLOG_CHECK(_randomFD == -1);
    _randomFD = _randomFileOpen();
}

static int _getRandomFile() {
    uv_once(&randomFileInstance, _randomFileInit);
    assert(_randomFD > 0);

    return _randomFD;
}

BFX_API(void) BfxRandomBytes(uint8_t* dest, size_t size) {
    assert(dest && size > 0);

    int fd = _getRandomFile();
    BLOG_CHECK(fd > 0);

    // 可能需要连续读取多次
    size_t readLen = 0;
    while (readLen < size) {
        ssize_t ret = BFX_HANDLE_EINTR(read(fd, dest + readLen, size - readLen));
        if (ret <= 0) {
            break;
        }

        readLen += ret;
    }
}

static uint64_t _random64() {
    uint64_t result = 0;
    BfxRandomBytes((uint8_t*)&result, sizeof(result));

    return result;
}