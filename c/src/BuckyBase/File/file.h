
#pragma once
#include "../Global/basic_type.h"
#include "../Path/path.h"
#include "../Buffer/Buffer.h"
#include "./cJSON.h"

// ����һ���ļ���buffer�����asString=true��ô����ĩβ����\0
BFX_API(BFX_BUFFER_HANDLE) BfxLoadFile(const BfxPathCharType* path, bool asString);

// ����һ��json�ļ�
BFX_API(cJSON*) BfxLoadJson(const BfxPathCharType* path);

#ifndef BFX_OS_POSIX

// �������û���Ȩ��
#define S_IRWXU 00700  // ��/д/ִ��
#define S_IRUSR 00400  // ��
#define S_IWUSR 00200  // д
#define S_IXUSR 00100  // ִ��

// �û���Ȩ��
#define S_IRWXG 00070  // ��/д/ִ��
#define S_IRGRP 00040  // ��
#define S_IWGRP 00020  // д
#define S_IXGRP 00010  // ִ��

// �����û�Ȩ��
#define S_IRWXO 00007  // ��/д/ִ��
#define S_IROTH 00004  // ��
#define S_IWOTH 00002  // д
#define S_IXOTH 00001  // ִ��

#endif // !BFX_OS_POSIX


BFX_API(int) BfxMakeDir(const BfxPathCharType* filepath, int pmode);
BFX_API(int) BfxMakeDirNested(const BfxPathCharType* filepath, int pmode);