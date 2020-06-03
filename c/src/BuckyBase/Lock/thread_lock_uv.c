#include "../BuckyBase.h"
#include "./thread_lock_uv.inl"


#ifdef BFX_LOCK_CHECK
static void _threadLockCheckOwnedAndSetUnowned(BfxThreadLock* lock) {
    BLOG_CHECK(lock->ownThreadID == BfxGetCurrentThreadID());
    lock->ownThreadID = 0;
}

static void _threadLockCheckUnownedAndSetOwned(BfxThreadLock* lock) {
    BLOG_CHECK(lock->ownThreadID == 0);
    lock->ownThreadID = BfxGetCurrentThreadID();
}

BFX_API(int) BfxThreadLockInit(BfxThreadLock* lock) {
    lock->ownThreadID = 0;
    return _uvThreadLockInit(&lock->lock);
}

BFX_API(void) BfxThreadLockDestroy(BfxThreadLock* lock) {
    BLOG_CHECK(lock->ownThreadID == 0);
    _uvThreadLockDestroy(&lock->lock);
}

BFX_API(int) BfxThreadLockTryLock(BfxThreadLock* lock) {
    int ret = _uvThreadLockTryLock(&lock->lock);
    if (ret == 0) {
        _threadLockCheckUnownedAndSetOwned(lock);
    }

    return ret;
}

BFX_API(void) BfxThreadLockLock(BfxThreadLock* lock) {
    _uvThreadLockLock(&lock->lock);

    _threadLockCheckUnownedAndSetOwned(lock);
}

BFX_API(void) BfxThreadLockUnlock(BfxThreadLock* lock) {
    _threadLockCheckOwnedAndSetUnowned(lock);

    _uvThreadLockUnlock(&lock->lock);
}

#else // !BFX_LOCK_CHECK

BFX_API(int) BfxThreadLockInit(BfxThreadLock *lock) {
    return _uvThreadLockInit(lock);
}

BFX_API(void) BfxThreadLockDestroy(BfxThreadLock *lock) {
    _uvThreadLockDestroy(lock);
}

BFX_API(int) BfxThreadLockTryLock(BfxThreadLock *lock) {
    return _uvThreadLockTryLock(lock);
}

BFX_API(void) BfxThreadLockLock(BfxThreadLock* lock) {
    _uvThreadLockLock(lock);
}

BFX_API(void) BfxThreadLockUnlock(BfxThreadLock* lock) {
    _uvThreadLockUnlock(lock);
}

#endif // !BFX_LOCK_CHECK