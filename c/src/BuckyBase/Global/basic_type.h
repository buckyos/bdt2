
#ifndef __BUCKYBASE_BASICTYPE_H__
#define __BUCKYBASE_BASICTYPE_H__

#include "./platform_specific.h"
#include "./compiler_specific.h"
#include "./global_config.h"


// C99的统一跨平台数据定义文件，编译器环境需要有该头文件的支持
#if !defined(__STDC_CONSTANT_MACROS)
#define __STDC_CONSTANT_MACROS
#endif // !__STDC_CONSTANT_MACROS

#if !defined(__STDC_LIMIT_MACROS)
#define __STDC_LIMIT_MACROS
#endif // !__STDC_LIMIT_MACROS

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif // !__STDC_FORMAT_MACROS

#if defined(__cplusplus)
#include <cstddef>
#else // C
#include <stddef.h>
#endif // C

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#if defined(__OBJC__)
#include <objc/objc.h>
#endif //objc

// size_t类型常量宏
#if defined(BFX_ARCH_CPU_32_BITS)
    #define SIZE_T_C(val) UINT32_C(val)
#elif defined(BFX_ARCH_CPU_64_BITS)
    #define SIZE_T_C(val) UINT64_C(val)
#else // unknown arch
    #error("CPU arch not support!!")
#endif // arch

// 平台相关字节长度类型的prinf和scanf支持，比如size_t和ptrdiff_t
#if defined(BFX_ARCH_CPU_32_BITS)

    #define PRIdVT PRId32
    #define PRIiVT PRIi32
    #define PRIoVT PRIo32
    #define PRIuVT PRIu32
    #define PRIxVT PRIx32
    #define PRIXVT PRIX32

    #define SCNdVT SCNd32
    #define SCNiVT SCNi32
    #define SCNoVT SCNo32
    #define SCNuVT SCNu32
    #define SCNxVT SCNx32

#else // BFX_ARCH_CPU_64_BITS

    #define PRIdVT PRId64
    #define PRIiVT PRIi64
    #define PRIoVT PRIo64
    #define PRIuVT PRIu64
    #define PRIxVT PRIx64
    #define PRIXVT PRIX64

    #define SCNdVT SCNd64
    #define SCNiVT SCNi64
    #define SCNoVT SCNo64
    #define SCNuVT SCNu64
    #define SCNxVT SCNx64

#endif // BFX_ARCH_CPU_64_BITS


#if !defined(BFX_OS_WIN)
// 接口层的BOOL类型定义
#if !defined(BOOL)

	#if !defined(__OBJC__)
		typedef int BOOL;

		#ifndef FALSE
			#define FALSE               0
		#endif

		#ifndef TRUE
			#define TRUE                1
		#endif

	#else // __OBJC__

		#ifndef FALSE
			#define FALSE               NO
		#endif

		#ifndef TRUE
			#define TRUE                YES
		#endif
	#endif // __OBJC__

#endif	// !BOOL
#endif // !BFX_OS_WIN

// C++环境下BOOL和bool的类型转换
#if defined(__cplusplus)
	#define BOOLTobool(bv) (!!(bv))
	#define boolToBOOL(bv) ((bv)? TRUE : FALSE)
#endif // __cplusplus

// 字符类型的定义
#if defined(BFX_COMPILER_CLANG) && defined(__cplusplus)
    typedef char        char8_t;

    // clang编译器里面引入了char16_t和char32_t的定义
#ifndef _LIBCPP_VERSION
    typedef uint16_t	char16_t;
    typedef uint32_t	char32_t;
#endif // !_LIBCPP_VERSION

#elif defined(BFX_COMPILER_GCC) && defined(__GXX_EXPERIMENTAL_CXX0X__)
	// gcc 在指定了c++0x标准以后，会内置charX_t类型
    typedef char		char8_t;
#elif defined(BFX_COMPILER_MSVC) && (_MSC_VER >= 1600) && defined(__cplusplus)
    // vs2010及以后版本支持c++0x
    typedef char		char8_t;
#else

	#if defined(BFX_COMPILER_MSVC)
		#define _CHAR16T // VS2010头文件yval.h里面定义了char16_t和char32_t
	#endif // BFX_COMPILER_MSVC

	typedef char		char8_t;
	typedef uint16_t	char16_t;
	typedef uint32_t	char32_t;

#endif // BFX_COMPILER_GCC && __GXX_EXPERIMENTAL_CXX0X__

// 定义不同平台下wchar_t的对等内部类型
#if defined(BFX_WCHAR_T_UTF16)
	typedef char16_t	wchar_t_equal_type;
#elif defined(BFX_WCHAR_T_UTF32)
	typedef char32_t	wchar_t_equal_type;
#elif defined(BFX_WCHAR_T_UTF8)
	typedef char8_t		wchar_t_equal_type;
#else
	#error "wchar_t type not defined!"
#endif // BFX_WCHAR_T_UTF16

#if defined(BFX_WCHAR_T_UNSIGNED)
    #if defined(BFX_WCHAR_T_UTF16)
        typedef uint16_t    wchar_t_int_type;
    #elif defined(BFX_WCHAR_T_UTF32)
        typedef uint32_t    wchar_t_int_type;
    #elif defined(BFX_WCHAR_T_UTF8)
        typedef uint8_t     wchar_t_int_type;
    #else
	    #error "wchar_t type not defined!"
    #endif // BFX_WCHAR_T_UTF16
#elif defined(BFX_WCHAR_T_SIGNED)
    #if defined(BFX_WCHAR_T_UTF16)
        typedef int16_t     wchar_t_int_type;
    #elif defined(BFX_WCHAR_T_UTF32)
        typedef int32_t     wchar_t_int_type;
    #elif defined(BFX_WCHAR_T_UTF8)
        typedef int8_t      wchar_t_int_type;
    #else
	    #error "wchar_t type not defined!"
    #endif // BFX_WCHAR_T_UTF16
#else
    #error "wchar_t signed type not specified!"
#endif 

// 不同平台的换行符
#if defined(BFX_OS_WIN)
    #define BFX_LINE_END        "\r\n"
    #define BFX_LINE_END_W      L"\r\n"
#elif defined(BFX_OS_MACOSX)
    #define BFX_LINE_END        "\r"
    #define BFX_LINE_END_W      L"\r"
#else // others
    #define BFX_LINE_END        "\n"
    #define BFX_LINE_END_W      L"\n"
#endif // others

// 字符串的一些posix兼容宏
#ifdef BFX_COMPILER_MSVC
    #ifndef strcasecmp
        #define strcasecmp _stricmp
    #endif // !strcasecmp
    #ifndef strncasecmp
        #define strncasecmp _strnicmp
    #endif // !strncasecmp
#endif //BFX_COMPILER_MSVC


//使用指南：
//1. 框架内部的字符串优先使用UTF8或者ASCII编码的char/char8_t字符串
//2. 框架内部的宽字符一律使用char16_t表示，不得使用char32_t
//3. 平台宽字符wchar_t，只有在和相关的系统接口交互的时候，才需要使用，使用接口和char8_t/char16_t字符串相互转换而来

/*
不同平台和编译器，对指定的宽字符字符串有不同程度的定制和优化，如果使用下面的方式定义，可以一定程度上优化宽字符字符串的支持和转换，
但是带来的问题就是过于复杂，并且wchar_t和char16_t或者char32_t类型可能一致，也可能不一致，会导致很多基于字符类型的模板难于设计和开发，
故而我们不使用下面的类型定义，而是使用上面的最简单的定义，除了一些系统api明确需要wchar_t的地方外，其余地方应该都使用char16_t来代替

#if defined(BFX_WCHAR_T_UTF16)
	#if defined(BFX_COMPILER_MSVC)
		#define _CHAR16T // VS2010头文件yval.h里面定义了char16_t和char32_t
	#endif // BFX_COMPILER_MSVC

	typedef char		char8_t;
	typedef wchar_t		char16_t;
	typedef int32_t	    char32_t;
#elif defined(BFX_WCHAR_T_UTF32)
	typedef char		char8_t;
	typedef int16_t	    char16_t;
	typedef wchar_t		char32_t;
#elif defined(BFX_WCHAR_T_UTF8)
	typedef int8_t		char8_t;
	typedef int16_t	    char16_t;
	typedef int32_t	    char32_t;
#else
	#error "wchar_t type not defined!
#endif // BFX_WCHAR_T_UTF16

*/


// 接口层接收的统一的UserData
// userData在传给接口时候，如果需要对userData进行生命周期管理，那么需要指定lpfnAddRefUserData和lpfnReleaseUserData
// 1. 如果lpfnAddRefUserData不为空，那么userData在接口内部需要持有的时候，会调用lpfnAddRefUserData增持，如果多处持有会多次调用；
// 2. 如果lpfnReleaseUserData不为空，那么说明该userData在接口内部需要释放的时候，会调用lpfnReleaseUserData来释放，和lpfnAddRefUserData调用是对称的
// 3. 如果一个UserData不需要声明周期管理，那么需要将lpfnAddRefUserData和lpfnReleaseUserData置空，这种情况下接口内部会直接持有UserData，外部必须保证UserData的有效性
// 4. lpfnAddRefUserData和lpfnReleaseUserData可以一个置空，这两种需要根据具体情况来决定：
//		a. 如果lpfnAddRefUserData!=NULL,而lpfnReleaseUserData==NULL，那么内部只会增持不会释放
//		b. 如果lpfnAddRefUserData==NULL,而lpfnReleaseUserData!=NULL，那么内部只会释放而不会增持

typedef struct BfxUserData
{
	void *userData;
	void (BFX_STDCALL *lpfnAddRefUserData)(void *userData);
	void (BFX_STDCALL *lpfnReleaseUserData)(void *userData);

} BfxUserData;

typedef void (BFX_STDCALL *fnUserDataFunc) (void* userData);

// 句柄类型的相关定义
#define BFX_DECLARE_HANDLE(name) struct __BFX_SAFE_HANDLE_##name { int unused; }; typedef struct __BFX_SAFE_HANDLE_##name *name;

#ifdef __cplusplus
#define BFX_DECLARE_HANDLE_EX(name, base) struct __BFX_SAFE_HANDLE_##name : public __BFX_SAFE_HANDLE_##base { int n##name; }; typedef __BFX_SAFE_HANDLE_##name *name;
#else
#define BFX_DECLARE_HANDLE_EX(name, base) BFX_DECLARE_HANDLE(name)
#endif //__cplusplus 

#endif // __BUCKYBASE_BASICTYPE_H__