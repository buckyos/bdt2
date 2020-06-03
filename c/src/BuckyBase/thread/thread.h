#pragma once

#if defined(BFX_OS_WIN)
typedef DWORD BfxProcessID;
typedef DWORD BfxThreadID;
#elif defined(BFX_OS_POSIX)
typedef pid_t BfxProcessID;
typedef pid_t BfxThreadID;
#endif // XPF_OS_WIN

BFX_API(BfxProcessID) BfxGetCurrentProcessID();
BFX_API(BfxThreadID) BfxGetCurrentThreadID();

// �߳�˯��ָ��ʱ�䣬duration΢��
BFX_API(void) BfxThreadSleep(int64_t duration);