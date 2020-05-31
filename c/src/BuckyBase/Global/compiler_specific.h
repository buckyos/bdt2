
#ifndef __BUCKYBASE_COMPILERSPECIFIC_H__
#define __BUCKYBASE_COMPILERSPECIFIC_H__

#include "./platform_specific.h"
#include "./result.h"

// GCC版本宏
#if defined(BFX_COMPILER_GCC)
#define BFX_GCC_VERSION (__GNUC__ * 10000 \
    + __GNUC_MINOR__ * 100 \
    + __GNUC_PATCHLEVEL__)
#endif // BFX_COMPILER_GCC

// __COUNTER__的兼容宏
// GCC4.3 release才增加了__COUNTER__预定义宏的支持，所以在老的版本使用__LINE__替代，
// 所以在老版本gcc下使用者不要在同一行使用超过一个带__COUNTER__的检测宏！
#ifndef __COUNTER__
    #define __COUNTER__ __LINE__
#endif // !__COUNTER__

// size_t和ptrdiff_t是否是编译器内置类型
#if defined(BFX_OS_MACOSX) && defined(BFX_ARCH_CPU_64_BITS)
    #define BFX_SIZE_T_INNERTYPE        1
    #define BFX_PTRDIFF_T_INNERTYPE     1
#endif // BFX_OS_MACOSX


// 不同编译器下的stdcall调用规范定义
#ifndef BFX_STDCALL
	#if defined(BFX_COMPILER_MSVC)
		#define BFX_STDCALL __stdcall
	#elif defined(BFX_COMPILER_GCC)
        // https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#index-force_005falign_005farg_005fpointer-function-attribute_002c-x86
        // gcc 下，__stdcall__选项只在x86-32下起作用，64bits下关闭这个选项
        #if defined(BFX_ARCH_CPU_X86)
		    #define BFX_STDCALL __attribute__((__stdcall__))
        #else
            #define BFX_STDCALL 
        #endif
	#else
		#error "unknown complier!!"
	#endif // BFX_COMPILER_MSVC
#endif // BFX_STDCALL

// 工程里面使用extern "C"的统一宏
#ifndef BFX_EXTERN_C
	#ifdef __cplusplus	
		#define BFX_EXTERN_C extern "C"
        #define BFX_BEGIN_EXTERN_C extern "C"{
        #define BFX_END_EXTERN_C }
	#else
		#define BFX_EXTERN_C 
        #define BFX_BEGIN_EXTERN_C
        #define BFX_END_EXTERN_C
	#endif // __cplusplus
#endif // BFX_EXTERN_C


// 导出接口的相关定义
#if defined(BFX_COMPILER_MSVC)
    #if defined(BFX_STATIC)
            #define BFX_API(x) BFX_EXTERN_C x
	#elif defined(BFX_EXPORTS)
			#define BFX_API(x) BFX_EXTERN_C __declspec(dllexport) x /*__stdcall*/ 
	#else // BFX_EXPORTS
			#define BFX_API(x) BFX_EXTERN_C __declspec(dllimport) x /*__stdcall*/ 
	#endif // BFX_EXPORTS
#elif defined(BFX_COMPILER_GCC)
    #if defined(BFX_STATIC)
            #define BFX_API(x) BFX_EXTERN_C x
	#elif defined(BFX_EXPORTS)
			#define BFX_API(x) BFX_EXTERN_C __attribute__((__visibility__("default")/*, __stdcall__*/)) x
	#else // BFX_EXPORTS
			#define BFX_API(x) BFX_EXTERN_C __attribute__((__visibility__("default")/*, __stdcall__*/)) x 
	#endif // BFX_EXPORTS
#endif // complier


// 特殊属性相关定义
#ifdef BFX_COMPILER_GCC
    #define BFX_LOCAL   __attribute__ ((visibility ("hidden")))
    #define BFX_PUBLIC  __attribute__ ((visibility ("default")))
    #define BFX_PACK(exp)  exp  __attribute__((__packed__))
    #define BFX_PACK_STRUCT(name)  struct  __attribute__((__packed__)) name
    #define BFX_TYPEOF __typeof__
    #define BFX_INLINE __inline__
    #define BFX_THREAD_LOCAL __thread
#else // MSVC
    #define BFX_LOCAL
    #define BFX_PUBLIC
    #define BFX_PACK(exp) __pragma(pack(push, 1)) exp __pragma(pack(pop))
    #define BFX_PACK_STRUCT(name) __pragma(pack(push, 1)) struct name __pragma(pack(pop))
    #define BFX_TYPEOF typeof
    #define BFX_INLINE inline
    #define BFX_THREAD_LOCAL __declspec(thread)
#endif // MSVC


// 句柄类型的相关定义
#define BFX_DECLARE_HANDLE(name) struct __BFX_SAFE_HANDLE_##name { int unused; }; typedef struct __BFX_SAFE_HANDLE_##name *name;

#ifdef __cplusplus
	#define BFX_DECLARE_HANDLE_EX(name, base) struct __BFX_SAFE_HANDLE_##name : public __BFX_SAFE_HANDLE_##base { int n##name; }; typedef __BFX_SAFE_HANDLE_##name *name;
#else
	#define BFX_DECLARE_HANDLE_EX(name, base) BFX_DECLARE_HANDLE(name)
#endif //__cplusplus 


// 编译时函数名称宏
#if defined(BFX_COMPILER_GCC)
    #define BFX_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
    #define BFX_CURRENT_FUNCTION __FUNCSIG__
#elif defined(BFX_COMPILER_MSVC)
    #define BFX_CURRENT_FUNCTION __FUNCTION__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
    #define BFX_CURRENT_FUNCTION __func__
#else // others
    #define BFX_CURRENT_FUNCTION "UnknownFunction"
#endif // others

// 用以中断的辅助宏
#if defined(_MSC_VER)
    #define BFX_DEBUGBREAK()  __debugbreak()
#else // GNUC
    #if defined(BFX_ARCH_CPU_ARM_FAMILY)
		#if defined(BFX_ARCH_CPU_32_BITS)
		#       define BFX_DEBUGBREAK() asm("bkpt 0")
		#   else // 64bits
		#       define BFX_DEBUGBREAK() asm("brk 0")
		#   endif // 64bits
    #elif defined(BFX_ARCH_CPU_MIPS_FAMILY)
	    #define BFX_DEBUGBREAK() asm("break 2")
    #elif defined(BFX_ARCH_CPU_X86_FAMILY)
	    #define BFX_DEBUGBREAK() asm("int3")
    #else
	    #error "unknown cpu arch!!"
    #endif //ARCH
#endif // GNUC


// 整型数组合辅助宏
#define BFX_MAKEUINT16(a, b)    ((uint16_t)(((uint8_t)(((uint32_t)(a)) & 0xFF)) | ((uint16_t)((uint8_t)(((uint32_t)(b)) & 0xFF))) << 8))
#define BFX_MAKEUINT32(a, b)    ((uint32_t)(((uint16_t)(((uint32_t)(a)) & 0xFFFF)) | ((uint32_t)((uint16_t)(((uint32_t)(b)) & 0xFFFF))) << 16))
#define BFX_MAKEUINT64(a, b)    ((uint64_t)(((uint32_t)(((uint64_t)(a)) & 0xFFFFFFFF)) | ((uint64_t)((uint32_t)(((uint64_t)(b)) & 0xFFFFFFFF))) << 32))

#define BFX_LOUINT32(x)         ((uint32_t)(((uint64_t)(x)) & 0xFFFFFFFF))
#define BFX_HIUINT32(x)         ((uint32_t)((((uint64_t)(x)) >> 32) & 0xFFFFFFFF))
#define BFX_LOUINT16(x)         ((uint16_t)(((uint32_t)(x)) & 0xFFFF))
#define BFX_HIUINT16(x)         ((uint16_t)((((uint32_t)(x)) >> 16) & 0xFFFF))
#define BFX_LOUINT8(x)          ((uint8_t)(((uint32_t)(x)) & 0xFF))
#define BFX_HIUINT8(x)          ((uint8_t)((((uint32_t)(x)) >> 8) & 0xFF))


// 检查一个返回值指针参数是否为空，
// 如果为空，返回BFX_RESULT_INVALID_POINTER;
// 否则设置默认返回值为NULL
#define BFX_VALIDATE_OUT_POINTER(x) \
    do {					        \
    TS_CHECK(x != NULL);	        \
    if (x == NULL)			        \
    return BFX_RESULT_INVALID_POINTER;\
    *x = NULL;				        \
    } while(0);

// 检查一个指针参数是否为空，
// 如果为空，返回BFX_RESULT_INVALID_POINT;
#define BFX_VALIDATE_POINTER(x)     \
    do {					        \
    TS_CHECK(x != NULL);	        \
    if (x == NULL)			        \
    return BFX_RESULT_INVALID_POINTER;	\
    } while(0);


// 禁止指定的warning
#if defined(BFX_COMPILER_MSVC)
    #define BFX_DISABLE_WARNING_BEGIN(warn) \
        __pragma(warning(push)) \
        __pragma(warning(disable : warn))
    #define BFX_DISABLE_WARNING_END() \
        __pragma(warning(pop))
#else // GNUC
    #define BFX_DISABLE_WARNING_BEGIN(warn) 
    #define BFX_DISABLE_WARNING_END()
#endif // GUNC

// 辅助宏
#define BFX_CONCAT_IMPL(x, y) x##y
#define BFX_MACRO_CONCAT(x, y) BFX_CONCAT_IMPL(x, y)

#define BFX_MACRO_STR(s) #s
#define BFX_MACRO_CONCAT_STR(x, y) BFX_MACRO_STR(x ## y)

#if BFX_GCC_VERSION >= 40200
    #define BFX_GCC_DIAGNOSTIC_DO_PRAGMA(x) _Pragma (#x)
    #define BFX_GCC_DIAGNOSTIC_PRAGMA(x) BFX_GCC_DIAGNOSTIC_DO_PRAGMA(GCC diagnostic x)
#endif //BFX_GCC_VERSION >= 40200

#if BFX_GCC_VERSION >= 40600
    #define BFX_DISABLE_GCCWARNING_BEGIN(warn) \
        BFX_GCC_DIAGNOSTIC_PRAGMA(push) \
        BFX_GCC_DIAGNOSTIC_PRAGMA(ignored BFX_MACRO_CONCAT_STR(-W, warn))
    #define BFX_DISABLE_GCCWARNING_END() \
        BFX_GCC_DIAGNOSTIC_PRAGMA(pop)

    #define BFX_DISABLE_GCCWARNING(warn) \
        BFX_GCC_DIAGNOSTIC_PRAGMA(ignored BFX_MACRO_CONCAT_STR(-W, warn))
    #define BFX_ENABLE_GCCWARNING(warn) \
        BFX_GCC_DIAGNOSTIC_PRAGMA(warning BFX_MACRO_CONCAT_STR(-W, warn))
#else // MSVN OR GCC under 4.6
    #define BFX_DISABLE_GCCWARNING_BEGIN(warn)
    #define BFX_DISABLE_GCCWARNING_END()

    #define BFX_DISABLE_GCCWARNING(warn)
    #define BFX_ENABLE_GCCWARNING(warn)
#endif //


// 数组长度
#define BFX_ARRAYSIZE(a)  (sizeof(a) / sizeof(*(a)))


// 计算成员变量大小
#define membersize(t, m) sizeof(((t*)(0))->m)



// 编译器指令优化相关宏
#ifdef BFX_COMPILER_GCC
    #define BFX_EXPECT(exp, value) __builtin_expect((exp), (value))
#else // MSVC
    #define BFX_EXPECT(exp, value) (exp)
#endif // MSVC

#define BFX_EXPECT_FALSE(exp) BFX_EXPECT(!!(exp), 0)
#define BFX_EXPECT_TRUE(exp)  BFX_EXPECT(!!(exp), 1)

#define BFX_LIKELY(exp)     BFX_EXPECT_TRUE(exp)
#define BFX_UNLIKELY(exp)   BFX_EXPECT_FALSE(exp)

/*
if (BFX_LIKELY(value)) 等价于if (value)，但是value为TRUE的可能性更大
if (BFX_UNLIKELY(value)) 也等价于if (value)，但是value为FASLE的可能性更大
使用上面两个宏用以“分支转移”的信息提供给编译器，编译器可以对代码进行优化，用以减少指令跳转带来的性能下降。

You should use it only in cases when the likeliest branch is very very very likely
, or when the unlikeliest branch is very very very unlikely.
*/

// 变量未使用警告
#ifdef BFX_COMPILER_GCC
#   define BFX_UNUSED_VARIABLE(x) UNUSED_ ## x __attribute__((__unused__))
#else // MSVC
#   define BFX_UNUSED_VARIABLE(x) (void)(x)
#endif //MSVC

#endif // __BUCKYBASE_COMPILERSPECIFIC_H__