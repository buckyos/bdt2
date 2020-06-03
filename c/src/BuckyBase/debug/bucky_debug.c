#include "../BuckyBase.h"
#include "./debug.h"

#if defined(BFX_OS_WIN)
#include "./debug_win.inl"
#else // posix
#include "./debug_posix.inl"
#endif // posix

BFX_API(bool) BfxIsBeingDebugged() {
    return _beingDebugged();
}

BFX_API(void) BfxDebugBreak() {
    _debugBreak();
}


BFX_API(int) BfxGetModulePath(const void* addrInModule, BfxPathCharType* path, size_t size) {
    return _getModulePath(addrInModule, path, size);
}


BFX_API(int) BfxGetExecutablePath(BfxPathCharType* path, size_t size) {
    return _getExecutablePath(path, size);
}