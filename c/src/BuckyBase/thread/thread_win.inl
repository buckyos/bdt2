#include "../BuckyBase.h"


BFX_API(BfxProcessID) BfxGetCurrentProcessID() {
	return GetCurrentProcessId();
}

BFX_API(BfxThreadID) BfxGetCurrentThreadID() {
	return GetCurrentThreadId();
}

BFX_API(void) BfxThreadSleep(int64_t duration) {
    int64_t now = BfxTimeGetNow(false);

    // ��Χ���
    if (duration < 0 || duration > INT64_MAX - now) {
        duration = INT64_MAX - now;
    }

    // Ϊ�˱���sleep������Ҫ���ѭ��ֱ��Ŀ�곤��
    int64_t end = now + duration;
    while ((now = BfxTimeGetNow(false)) < end) {

        int64_t left = end - now;
        int64_t delta = left / 1000;
        if (left % 1000) {
            ++delta;
        }

        Sleep((DWORD)delta);
    }
}