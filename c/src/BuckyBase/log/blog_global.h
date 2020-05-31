#pragma once

#include "./blog_decl.h"

#if !defined(BLOG_DISABLE)

// 默认的全局isolate
BLOG_DECLARE_ISOLATE(GLOBAL);

#define GLOBAL_LOG_IF_EX(cond, level, ...) _BLOG_LOG_EX(GLOBAL, level, ##__VA_ARGS__);
#define GLOBAL_LOG_EX(level, ...) _BLOG_LOG_EX(GLOBAL, true, level, ##__VA_ARGS__);

#define GLOBAL_VERBOSE(...) GLOBAL_LOG_EX(BLogLevel_Verbose, ##__VA_ARGS__);
#define GLOBAL_TRACE(...)   GLOBAL_LOG_EX(BLogLevel_Trace, ##__VA_ARGS__);
#define GLOBAL_DEBUG(...)   GLOBAL_LOG_EX(BLogLevel_Debug, ##__VA_ARGS__);
#define GLOBAL_INFO(...)    GLOBAL_LOG_EX(BLogLevel_Info, ##__VA_ARGS__);
#define GLOBAL_WARN(...)    GLOBAL_LOG_EX(BLogLevel_Warn, ##__VA_ARGS__);
#define GLOBAL_ERROR(...)   GLOBAL_LOG_EX(BLogLevel_Error, ##__VA_ARGS__);
#define GLOBAL_FATAL(...)   GLOBAL_LOG_EX(BLogLevel_Fatal, ##__VA_ARGS__);

#define GLOBAL_CHECK(cond)  _BLOG_CHECK(GLOBAL, cond);
#define GLOBAL_CHECK_EX(cond, ...)  _BLOG_CHECK_EX(GLOBAL, cond, ##__VA_ARGS__);

#define GLOBAL_VERIFY(cond)  _BLOG_CHECK(GLOBAL, cond);
#define GLOBAL_VERIFY_EX(cond, ...)  _BLOG_CHECK_EX(GLOBAL, cond, ##__VA_ARGS__);

#define GLOBAL_VERIFY_ERROR_CODE(errCode) GLOBAL_VERIFY((errCode) == 0)
#define GLOBAL_VERIFY_ERROR_CODE_EX(errCode, ...) GLOBAL_VERIFY_EX((errCode) == 0, ##__VA_ARGS__)

#define GLOBAL_VERBOSE_IF(cond, ...) GLOBAL_LOG_IF_EX(cond, BLogLevel_Verbose, ##__VA_ARGS__);
#define GLOBAL_TRACE_IF(cond, ...)   GLOBAL_LOG_IF_EX(cond, BLogLevel_Trace, ##__VA_ARGS__);
#define GLOBAL_DEBUG_IF(cond, ...)   GLOBAL_LOG_IF_EX(cond, BLogLevel_Debug, ##__VA_ARGS__);
#define GLOBAL_INFO_IF(cond, ...)    GLOBAL_LOG_IF_EX(cond, BLogLevel_Info, ##__VA_ARGS__);
#define GLOBAL_WARN_IF(cond, ...)    GLOBAL_LOG_IF_EX(cond, BLogLevel_Warn, ##__VA_ARGS__);
#define GLOBAL_ERROR_IF(cond, ...)   GLOBAL_LOG_IF_EX(cond, BLogLevel_Error, ##__VA_ARGS__);
#define GLOBAL_FATAL_IF(cond, ...)   GLOBAL_LOG_IF_EX(cond, BLogLevel_Fatal, ##__VA_ARGS__);

#define GLOBAL_HEX_IF_EX(cond, level, address, length, ...) _BLOG_HEX_EX(GLOBAL, cond, level, address, length, ##__VA_ARGS__);
#define GLOBAL_HEX_EX(level, address, length, ...) _BLOG_HEX_EX(GLOBAL, true, level, address, length, ##__VA_ARGS__);

#define GLOBAL_HEX_IF(cond, address, length, ...) GLOBAL_HEX_IF_EX(cond, BLogLevel_Info, address, length, ##__VA_ARGS__);
#define GLOBAL_HEX(address, length, ...) GLOBAL_HEX_EX(BLogLevel_Info, address, length, ##__VA_ARGS__);


#define GLOBAL_NOT_REACHED() GLOBAL_FATAL("!!!should not reach here!!! %s::%d::%s", __FILE__, __LINE__, __FUNCTION__);
#define GLOBAL_NOT_IMPL() GLOBAL_FATAL("!!!function not impl!!! %s::%d::%s", __FILE__, __LINE__, __FUNCTION__);

#else // disabled

#define GLOBAL_LOG_IF_EX(cond, level, ...)  BLOG_NOOP;
#define GLOBAL_LOG_EX(level, ...)           BLOG_NOOP;

#define GLOBAL_VERBOSE(...) BLOG_NOOP;
#define GLOBAL_TRACE(...)   BLOG_NOOP;
#define GLOBAL_DEBUG(...)   BLOG_NOOP;
#define GLOBAL_INFO(...)    BLOG_NOOP;
#define GLOBAL_WARN(...)    BLOG_NOOP;
#define GLOBAL_ERROR(...)   BLOG_NOOP;
#define GLOBAL_FATAL(...)   BLOG_NOOP;

#define GLOBAL_CHECK(cond)          BLOG_NOOP;
#define GLOBAL_CHECK_EX(cond, ...)  BLOG_NOOP;

#define GLOBAL_VERIFY(cond)             (cond);
#define GLOBAL_VERIFY_EX(cond, ...)     (cond);

#define GLOBAL_VERIFY_ERROR_CODE(errCode)           (errCode);
#define GLOBAL_VERIFY_ERROR_CODE_EX(errCode, ...)   (errCode);

#define GLOBAL_VERBOSE_IF(cond, ...) BLOG_NOOP;
#define GLOBAL_TRACE_IF(cond, ...)   BLOG_NOOP;
#define GLOBAL_DEBUG_IF(cond, ...)   BLOG_NOOP;
#define GLOBAL_INFO_IF(cond, ...)    BLOG_NOOP;
#define GLOBAL_WARN_IF(cond, ...)    BLOG_NOOP;
#define GLOBAL_ERROR_IF(cond, ...)   BLOG_NOOP;
#define GLOBAL_FATAL_IF(cond, ...)   BLOG_NOOP;

#define GLOBAL_HEX_IF_EX(cond, level, address, length, ...) BLOG_NOOP;
#define GLOBAL_HEX_EX(level, address, length, ...)          BLOG_NOOP;

#define GLOBAL_HEX_IF(cond, address, length, ...)   BLOG_NOOP;
#define GLOBAL_HEX(address, length, ...)            BLOG_NOOP;

#define GLOBAL_NOT_REACHED()    BLOG_NOOP;
#define GLOBAL_NOT_IMPL()       BLOG_NOOP;

#endif // disabled