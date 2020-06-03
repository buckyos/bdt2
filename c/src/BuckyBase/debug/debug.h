#pragma once

BFX_API(bool) BfxIsBeingDebugged();

BFX_API(void) BfxDebugBreak();

// 给定地址，获取所在模块的路径，返回实际获取到的长度，path确保以\0结束，所以最多返回size-1的有效内容
BFX_API(int) BfxGetModulePath(const void* addrInModule, BfxPathCharType *path, size_t size);

// 获取exe路径
BFX_API(int) BfxGetExecutablePath(BfxPathCharType* path, size_t size);