
#ifndef __BUCKYBASE_PLATFORMHEADERS_H__
#define __BUCKYBASE_PLATFORMHEADERS_H__

#if defined(BFX_OS_WIN)

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// windows版本，最小版本支持
// Windows Server 2008, Windows vista
#if !defined(WINVER)
	#define WINVER			0x0600
#else
	#if WINVER < 0x0600
		#error "unsupport window version!"
	#endif // WINVER < 0x0501
#endif // WINVER

#if !defined(_WIN32_WINNT)
	#define _WIN32_WINNT			0x0600
#else
	#if _WIN32_WINNT < 0x0600
		#error "unsupport window version!"
	#endif // _WIN32_WINNT < 0x0501
#endif // _WIN32_WINNT


// windows的标准头文件 
#include <Windows.h>

#include <sys/types.h>
#include <sys/stat.h>

// 由于stdint.h和wchar_t版本不一致，导致WCHAR_MIN重复定义，所以这里disable掉这个waring
#pragma warning(push)
#pragma warning(disable:4005)
#include <wchar.h>
#pragma warning(pop)

#define _CRT_RAND_S

#include <WinUser.h>
#include <WinDef.h>
#include <Shlwapi.h>
#include <Sddl.h>
#include <shellapi.h>

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <Mswsock.h>
#include <ws2ipdef.h>

#include <tchar.h>
#include <process.h>

#elif defined(BFX_OS_LINUX)

#include <linux/unistd.h>
#include <sys/syscall.h>

#elif defined(BFX_OS_MACOSX)

#include <CoreFoundation/CoreFoundation.h>

#endif // BFX_OS_MACOSX


// posix标准系统的头文件
#if defined(BFX_OS_POSIX)

#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>

#endif // BFX_OS_POSIX

#if defined(BFX_OS_ANDROID) && defined(BFX_ARCH_CPU_32_BITS)
#include <time64.h>
#endif

// BuckyBase.h依赖的标准C头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#endif // __BUCKYBASE_PLATFORMHEADERS_H__