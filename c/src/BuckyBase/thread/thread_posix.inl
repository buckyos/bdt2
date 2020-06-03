#include "../BuckyBase.h"


BFX_API(BfxProcessID) BfxGetCurrentProcessID() {
	return getpid();
}

BFX_API(BfxThreadID) BfxGetCurrentThreadID() {
#if defined(BFX_OS_MACOSX)
	return pthread_mach_thread_np(pthread_self());
#elif defined(BFX_OS_ANDROID)
	return gettid();
#elif defined(BFX_OS_LINUX)
	return syscall(__NR_gettid);
#elif defined(BFX_OS_POSIX)
	return reinterpret_cast<int64_t>(pthread_self());
#endif
}

BFX_API(void) BfxThreadSleep(int64_t duration) {
    if (duration < 0) {
        duration = INT64_MAX;
    }

    struct timespec sleepTime, remaining;

    sleepTime.tv_sec = duration / (1000 * 1000);
    duration -= sleepTime.tv_sec * 1000 * 1000;
    sleepTime.tv_nsec = duration * 1000;

    // 需要处理由于信号机制触发的EINTR的错误，所以这里需要循环执行
    while (nanosleep(&sleepTime, &remaining) == -1 && errno == EINTR) {
        sleepTime = remaining;
    }
}