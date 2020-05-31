#ifdef BFX_LOCK_CHECK

typedef struct BfxThreadLock {
    uv_mutex_t lock;
    volatile BfxThreadID ownThreadID;
} BfxThreadLock;

#else // !BFX_LOCK_CHECK

typedef uv_mutex_t BfxThreadLock;

#endif // !BFX_LOCK_CHECK

BFX_API(int) BfxThreadLockInit(BfxThreadLock* lock);
BFX_API(void) BfxThreadLockDestroy(BfxThreadLock* lock);

BFX_API(int) BfxThreadLockTryLock(BfxThreadLock* lock);
BFX_API(void) BfxThreadLockLock(BfxThreadLock* lock);
BFX_API(void) BfxThreadLockUnlock(BfxThreadLock* lock);


// ThreadLockµÄÈÕÖ¾°æ¼ì²âºê
#ifdef BFX_LOCK_CHECK
#   define BFX_THREADLOCK_CHECK_CURRENT_THREAD_OWNED(lock) \
        BLOG_CHECK((lock)->ownThreadID == BfxGetCurrentThreadID());

#   define BFX_THREADLOCK_CHECK_CURRENT_THREAD_UNOWNED(lock) \
        BLOG_CHECK((lock)->ownThreadID != BfxGetCurrentThreadID());
#else // !BFX_LOCK_CHECK
#   define BFX_THREADLOCK_CHECK_CURRENT_THREAD_OWNED(lock) BLOG_NOOP
#   define BFX_THREADLOCK_CHECK_CURRENT_THREAD_UNOWNED(lock)  BLOG_NOOP
#endif // !BFX_LOCK_CHECK