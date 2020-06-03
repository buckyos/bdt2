#pragma once

#if defined(BFX_OS_POSIX)

#include <errno.h>

#if (!__cplusplus || __cplusplus < 201103L)

#if defined(NDEBUG)
    #define BFX_HANDLE_EINTR(x) ({ \
        typeof(x) result; \
        do { \
            result = (x); \
        } while (result == -1 && errno == EINTR); \
        result; \
    })

#else // DEBUG

    #define BFX_HANDLE_EINTR(x) ({ \
        int counter = 0; \
        typeof(x) result; \
        do { \
            result = (x); \
        } while (result == -1 && errno == EINTR && counter++ < 100); \
        result; \
    })
#endif  // NDEBUG

#else // c++ && >= c++11

#if defined(NDEBUG)
    #define BFX_HANDLE_EINTR(x) ({ \
	    decltype(x) result; \
	    do { \
	        result = (x); \
	    } while (result == -1 && errno == EINTR); \
	    result; \
    })

#else // DEBUG

    #define BFX_HANDLE_EINTR(x) ({ \
	    int counter = 0; \
	    decltype(x) result; \
	    do { \
	    result = (x); \
	    } while (result == -1 && errno == EINTR && \
	    counter++ < 100); \
	    result; \
    })

#endif  // NDEBUG
#endif // c++0x, c++11

#else	// BFX_OS_WIN

#define BFX_HANDLE_EINTR(x) (x)

#endif  // BFX_OS_POSIX