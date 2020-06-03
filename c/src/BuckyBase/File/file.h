
#pragma once
#include "../Global/basic_type.h"
#include "../Path/path.h"
#include "../Buffer/Buffer.h"
#include "./cJSON.h"

// 加载一个文件到buffer，如果asString=true那么会在末尾增加\0
BFX_API(BFX_BUFFER_HANDLE) BfxLoadFile(const BfxPathCharType* path, bool asString);

// 加载一个json文件
BFX_API(cJSON*) BfxLoadJson(const BfxPathCharType* path);

#ifndef BFX_OS_POSIX

// 所有者用户的权限
#define S_IRWXU 00700  // 读/写/执行
#define S_IRUSR 00400  // 读
#define S_IWUSR 00200  // 写
#define S_IXUSR 00100  // 执行

// 用户组权限
#define S_IRWXG 00070  // 读/写/执行
#define S_IRGRP 00040  // 读
#define S_IWGRP 00020  // 写
#define S_IXGRP 00010  // 执行

// 其它用户权限
#define S_IRWXO 00007  // 读/写/执行
#define S_IROTH 00004  // 读
#define S_IWOTH 00002  // 写
#define S_IXOTH 00001  // 执行

#endif // !BFX_OS_POSIX


BFX_API(int) BfxMakeDir(const BfxPathCharType* filepath, int pmode);
BFX_API(int) BfxMakeDirNested(const BfxPathCharType* filepath, int pmode);