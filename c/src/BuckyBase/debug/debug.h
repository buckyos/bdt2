#pragma once

BFX_API(bool) BfxIsBeingDebugged();

BFX_API(void) BfxDebugBreak();

// ������ַ����ȡ����ģ���·��������ʵ�ʻ�ȡ���ĳ��ȣ�pathȷ����\0������������෵��size-1����Ч����
BFX_API(int) BfxGetModulePath(const void* addrInModule, BfxPathCharType *path, size_t size);

// ��ȡexe·��
BFX_API(int) BfxGetExecutablePath(BfxPathCharType* path, size_t size);