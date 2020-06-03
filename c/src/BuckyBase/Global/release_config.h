
#ifndef __BUCKYBASE_RELEASECONFIG_H__
#define __BUCKYBASE_RELEASECONFIG_H__

#ifdef XPF_CONFIG_HEADER
#error XPF_CONFIG_HEADER already been defined!
#endif // XPF_CONFIG_HEADER

#define  XPF_CONFIG_HEADER

// 如果没有开启XPF_LOG宏，那么开启
#ifndef XPF_LOG
#define XPF_LOG                 1
#endif // !XPF_LOG

// 日志的相关静态开关宏
// 标准日志输出的日志宏，包括TRACE到WARN的标准格式化和流式输出
#define XPF_LOG_NORMAL          1

// 区域性日志
#define XPF_LOG_SCOPE           1

// 内存dump类日志
#define XPF_LOG_HEXDUMP         1

// CHECK类日志
#define XPF_LOG_CHECK           1

// 对象跟踪系统类
#define XPF_LOG_TRACK           1


// 是否关闭日志的HOOK功能
/* #undef XPF_LOG_HOOK_OFF    */       

// 是否完全关闭日志的控制台输出
/* #undef XPF_LOG_CONSOLE_OUTPUT_OFF    */

// 是否完全关闭日志的调试器输出
/* #undef XPF_LOG_DEBUGGER_OUTPUT_OFF   */

// 是否完全关闭日志的文件输出
/* #undef XPF_LOG_FILE_OUTPUT_OFF       */

// 是否完全关闭日志的错误框输出
/* #undef XPF_LOG_MSGBOX_OUTPUT_OFF     */


// 禁止使用自旋锁，使用线程锁代替
// #define XPF_DISEBLE_SPINLOCK

// 线程安全检测
#define XPF_THREAD_SAFE_CHECK   1
#define XPF_THREAD_CHECK        1


// 开启严格模式
#define XPF_STRICT              1


// 多线程执行代码段的动态检测
#define XPF_SCOPE_VERIFY        1


// 线程锁的检测
#define XPF_LOCK_CHECK          1


// Task追踪宏
#define XPF_TRACK_TASK          1


// FixAllocator的安全检测
#define XPF_FIXALLOCATOR_CHECK  1
//#define XPF_FIXALLOCATOR_FULL_CHECK 0


// RefBase的调试开关
#define XPF_DEBUG_REF           1

// RangeTree的检测逻辑
#define XPF_RANGETREE_DEBUG     1

// Trie的检测逻辑
#define XPF_TRIE_DEBUG          1

// PlexList的检测逻辑
#define XPF_PLEXLIST_DEBUG      1

// XPFBuffer的检测
#define XPF_BUFFER_CHECK        1


// EndPoint的检测
#define XPF_ENDPOINT_CHECK      1


// LuaRuntime的Lua虚拟机的Assert检测
#define XPF_LUARUNTIM_ASSERT    1


// Lua运行时的线程LuaContext
#define XPF_LUACONTEXT          1


// LuaRuntime的线程安全检测
// 该功能依赖XPF_LUACONTEXT宏的开启
#define XPF_LUA_THREAD_CHECK    1



#endif // __BUCKYBASE_RELEASECONFIG_H__