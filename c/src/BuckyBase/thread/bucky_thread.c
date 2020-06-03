#include "../BuckyBase.h"
#include "./thread.h"

#if defined(BFX_OS_WIN)
#include "./thread_win.inl"
#else
#include "./thread_posix.inl"
#endif