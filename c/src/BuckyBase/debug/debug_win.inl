static bool _beingDebugged() {
    return !!IsDebuggerPresent();
}

static void _debugBreak() {
    DebugBreak();
}

static int _getModulePath(const void* addr, BfxPathCharType* path, size_t size) {
    
    HMODULE mod = NULL;
    BOOL ret = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)addr, &mod);
    if (!ret) {
        BLOG_ERROR("call GetModuleHandleEx with address error!! addr=%p, err=%d", addr, GetLastError());
        return 0;
    }

    DWORD retSize = GetModuleFileName(mod, path, (DWORD)size);
    if (retSize == 0) {
        BLOG_ERROR("call GetModuleFileName with address failed!! module=%p, err=%d", mod, GetLastError());
        return 0;
    }

    return (int)retSize;
}

int _getExecutablePath(BfxPathCharType* path, size_t size) {

    DWORD ret = GetModuleFileName(NULL, path, (DWORD)size - 1);
    if (ret == 0) {
        BLOG_ERROR("call GetModuleFileName failed!");
        return 0;
    }

    path[ret] = BFX_PATH_LITERAL('\0');

    return (int)ret;
}