#pragma once


#ifdef BFX_OS_WIN

typedef HANDLE BuckyNativeSocketType;
#define BFX_NATIVE_SOCKET_INVALID_HANDLE    INVALID_HANDLE_VALUE
#define BFX_NATIVE_SOCKET_IS_VALID(handle)  (handle != INVALID_HANDLE_VALUE)

#else // POSIX

typedef int BuckyNativeSocketType;
#define BFX_NATIVE_SOCKET_INVALID_HANDLE  -1
#define BFX_NATIVE_SOCKET_IS_VALID(fd)  (fd >= 0)

#endif // POSIX