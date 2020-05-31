#ifndef _BDT_AUTO_TEST_DUMP_H_
#define _BDT_AUTO_TEST_DUMP_H_

#include <windows.h>  
#include <DbgHelp.h>  
#include <stdlib.h>  
#pragma comment(lib, "dbghelp.lib")  
#pragma comment(lib, "shlwapi.lib")

//#ifndef _M_IX86  
//#error "The following code only works for x86!"  
//#endif  

wchar_t g_wszDumpSavePath[256] = { 0 };

inline BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
{
	if (pModuleName == 0)
	{
		return FALSE;
	}

	WCHAR szFileName[_MAX_FNAME] = L"";
	_wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

	if (wcsicmp(szFileName, L"ntdll") == 0)
		return TRUE;

	return FALSE;
}

inline BOOL CALLBACK MiniDumpCallback(PVOID                            pParam,
	const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT        pOutput)
{
	if (pInput == 0 || pOutput == 0)
		return FALSE;

	switch (pInput->CallbackType)
	{
	case ModuleCallback:
		if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
			if (!IsDataSectionNeeded(pInput->Module.FullPath))
				pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
	case IncludeModuleCallback:
	case IncludeThreadCallback:
	case ThreadCallback:
	case ThreadExCallback:
		return TRUE;
	default:;
	}

	return FALSE;
}

inline void CreateMiniDump(PEXCEPTION_POINTERS pep, LPCWSTR strFileName)
{
	HANDLE hFile = CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//MessageBox(0, strFileName, 0, 0);
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = FALSE;

		MINIDUMP_CALLBACK_INFORMATION mci;
		mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
		mci.CallbackParam = 0;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, (pep != 0) ? &mdei : 0, NULL, &mci);

		CloseHandle(hFile);
	}
}

LONG __stdcall MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	wchar_t wszFile[MAX_ZONE_PATH] = { 0 };
	GetModuleFileName(NULL, wszFile, MAX_ZONE_PATH);
	
	wchar_t wszFileName[MAX_ZONE_PATH] = { 0 };
	wsprintf(wszFileName, L"%s\\%s.dmp", g_wszDumpSavePath, PathFindFileName(wszFile));
	CreateMiniDump(pExceptionInfo, wszFileName);

	Sleep(5 * 1000);

	return EXCEPTION_EXECUTE_HANDLER;
}

// 此函数一旦成功调用，之后对 SetUnhandledExceptionFilter 的调用将无效  
void DisableSetUnhandledExceptionFilter()
{
	void* addr = (void*)GetProcAddress(LoadLibrary(L"kernel32.dll"), "SetUnhandledExceptionFilter");

	if (addr)
	{
		unsigned char code[16];
		int size = 0;

		code[size++] = 0x33;
		code[size++] = 0xC0;
		code[size++] = 0xC2;
		code[size++] = 0x04;
		code[size++] = 0x00;

		DWORD dwOldFlag, dwTempFlag;
		VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
		VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
	}
}

void InitMinDump(const char* savePath)
{
	size_t dstLen = 256;
	size_t srcLen = strlen(savePath);
	BfxTranscodeUTF8ToWChar(savePath, &srcLen, g_wszDumpSavePath, &dstLen);
	//注册异常处理函数  
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	//使SetUnhandledExceptionFilter  
	DisableSetUnhandledExceptionFilter();
}

#endif