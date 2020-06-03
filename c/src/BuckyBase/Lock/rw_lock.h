

#ifdef BFX_LOCK_CHECK

typedef struct BfxRwLock {
    uv_rwlock_t lock;
    BfxTlsKey readThreadID;
    volatile BfxThreadID writeThreadID;
}BfxRwLock;

#else // !BFX_LOCK_CHECK

typedef uv_rwlock_t BfxRwLock;

#endif // !BFX_LOCK_CHECK

// ��ʼ����ؽӿ�
BFX_API(int) BfxRwLockInit(BfxRwLock* lock);
BFX_API(void) BfxRwLockDestroy(BfxRwLock* lock);

// ������ؽӿ�
BFX_API(int) BfxRwLockRTryLock(BfxRwLock* lock);
BFX_API(void) BfxRwLockRLock(BfxRwLock* lock);
BFX_API(void) BfxRwLockRUnlock(BfxRwLock* lock);

// д����ؽӿ�
BFX_API(int) BfxRwLockWTryLock(BfxRwLock* lock);
BFX_API(void) BfxRwLockWLock(BfxRwLock* lock);
BFX_API(void) BfxRwLockWUnlock(BfxRwLock* lock);


#ifdef BFX_LOCK_CHECK
void _rwLockReadCheckOwned(BfxRwLock* lock);
void _rwLockReadCheckUnowned(BfxRwLock* lock);
void _rwLockWriteCheckOwned(BfxRwLock* lock);
void _rwLockWriteCheckUnowned(BfxRwLock* lock);
#endif // BFX_LOCK_CHECK


// RwLock����־�����
#ifdef BFX_LOCK_CHECK
#   define BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_OWNED(lock) \
        _rwLockReadCheckOwned(lock);

#   define BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_UNOWNED(lock) \
        _rwLockReadCheckUnowned(lock);

#   define BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_OWNED(lock) \
        _rwLockWriteCheckOwned(lock);

#   define BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_UNOWNED(lock) \
        _rwLockWriteCheckUnowned(lock);

#else // !BFX_LOCK_CHECK

#   define BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_OWNED(lock)     BLOG_NOOP
#   define BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_UNOWNED(lock)   BLOG_NOOP
#   define BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_OWNED(lock)    BLOG_NOOP
#   define BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_UNOWNED(lock)  BLOG_NOOP

#endif // !BFX_LOCK_CHECK