
#ifndef _BUCKY_BASE_H_
#define _BUCKY_BASE_H_

// 平台相关定义
#include "./Global/platform_specific.h"

// 错误码定义
#include "./Global/result.h"

// 条件编译相关宏
#include "./Global/global_config.h"

// 编译器相关定义
#include "./Global/compiler_specific.h"

// 基础类型相关定义
#include "./Global/basic_type.h"

// 不同平台相关定义和头文件
#include "./Global/platform_headers.h"

// 日志版，开启log检测
#if !defined(BLOG_DISABLE)
#define BFX_LOCK_CHECK 1
#endif // !BLOG_DISABLE


#include "./Global/base.h"

#include <uv.h>

#include "./Global/e_int.h"

#include "./crt/crt.h"

#include "./Atomic/atomic.h"

#include "./Memory/memory.h"
#include "./Memory/byte_order.h"

#include "./TLS/tls.h"

#include "./thread/thread.h"

#include "./Buffer/Buffer.h"
#include "./Buffer/stack_buffer.h"
#include "./Buffer/thread_buffer.h"
#include "./Buffer/buffer_stream.h"

#include "./Transcode/transcode.h"

#include "./String/string.h"

#include "./Coll/vector.h"
#include "./Coll/list.h"
#include "./Coll/hash.h"

#include "./Time/time.h"

#include "./Lock/thread_lock.h"
#include "./Lock/rw_lock.h"

#include "./Random/random.h"

#include "./log/blog.h"

#include "./Path/path.h"

#include "./File/cJSON.h"
#include "./File/file.h"

#include "./debug/debug.h"

#include "./events/event_emitter.h"

#endif //_BUCKY_BASE_H_