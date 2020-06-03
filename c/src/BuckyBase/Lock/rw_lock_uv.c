#include "../BuckyBase.h"
#include "./rw_lock_uv.inl"


#ifdef BFX_LOCK_CHECK

void _rwLockReadCheckOwned(BfxRwLock* lock) {
    BfxThreadID id = (BfxThreadID)(intptr_t)BfxTlsGetData(&lock->readThreadID);

    BLOG_CHECK(id == BfxGetCurrentThreadID());
}

void _rwLockReadCheckUnowned(BfxRwLock* lock) {
    BfxThreadID id = (BfxThreadID)(intptr_t)BfxTlsGetData(&lock->readThreadID);

    BLOG_CHECK(id != BfxGetCurrentThreadID());
}

void _rwLockWriteCheckOwned(BfxRwLock* lock) {
    BLOG_CHECK(lock->writeThreadID == BfxGetCurrentThreadID());
}

void _rwLockWriteCheckUnowned(BfxRwLock* lock) {
    BLOG_CHECK(lock->writeThreadID != BfxGetCurrentThreadID());
}

static void _rwLockReadCheckOwnedAndSetUnowned(BfxRwLock* lock) {
    BfxThreadID id = (BfxThreadID)(intptr_t)BfxTlsGetData(&lock->readThreadID);

    BLOG_CHECK(id == BfxGetCurrentThreadID());
    BfxTlsSetData(&lock->readThreadID, (void*)0);
}

static void _rwLockReadCheckUnownedAndSetOwned(BfxRwLock* lock) {
    BfxThreadID id = (BfxThreadID)(intptr_t)BfxTlsGetData(&lock->readThreadID);
    BLOG_CHECK(id == 0);
    BfxTlsSetData(&lock->readThreadID, (void*)(intptr_t)BfxGetCurrentThreadID());
}

static void _rwLockWriteCheckOwnedAndSetUnowned(BfxRwLock* lock) {
    BLOG_CHECK(lock->writeThreadID == BfxGetCurrentThreadID());
    lock->writeThreadID = 0;
}

static void _rwLockWriteCheckUnownedAndSetOwned(BfxRwLock* lock) {
    BLOG_CHECK(lock->writeThreadID == 0);
    lock->writeThreadID = BfxGetCurrentThreadID();
}

BFX_API(int) BfxRwLockInit(BfxRwLock* lock) {
    int ret = _uvRwLockInit(&lock->lock);
    if (ret != 0) {
        return ret;
    }

    BfxTlsCreate(&lock->readThreadID);
    lock->writeThreadID = 0;

    return 0;
}

BFX_API(void) BfxRwLockDestroy(BfxRwLock* lock) {
    BfxThreadID id = (BfxThreadID)(intptr_t)BfxTlsGetData(&lock->readThreadID);
    BLOG_CHECK(id == 0);
    BLOG_CHECK(lock->writeThreadID == 0);

    BfxTlsDelete(&lock->readThreadID);
    lock->writeThreadID = 0;

    _uvRwLockDestroy(&lock->lock);
}

// 读锁相关接口
BFX_API(int) BfxRwLockRTryLock(BfxRwLock* lock) {
    int ret = _uvRwLockRTryLock(&lock->lock);
    if (ret == 0) {
        _rwLockReadCheckUnownedAndSetOwned(lock);
    }

    return ret;
}

BFX_API(void) BfxRwLockRLock(BfxRwLock* lock) {
    _uvRwLockRLock(&lock->lock);

    _rwLockReadCheckUnownedAndSetOwned(lock);
}

BFX_API(void) BfxRwLockRUnlock(BfxRwLock* lock) {
    _rwLockReadCheckOwnedAndSetUnowned(lock);

    _uvRwLockRUnlock(&lock->lock);
}

// 写锁相关接口
BFX_API(int) BfxRwLockWTryLock(BfxRwLock* lock) {
    int ret = _uvRwLockWTryLock(&lock->lock);
    if (ret == 0) {
        _rwLockWriteCheckUnownedAndSetOwned(lock);
    }

    return ret;
}

BFX_API(void) BfxRwLockWLock(BfxRwLock* lock) {
    _uvRwLockWLock(&lock->lock);

    _rwLockWriteCheckUnownedAndSetOwned(lock);
}

BFX_API(void) BfxRwLockWUnlock(BfxRwLock* lock) {
    _rwLockWriteCheckOwnedAndSetUnowned(lock);

    _uvRwLockWUnlock(&lock->lock);
}

#else // !BFX_LOCK_CHECK

BFX_API(int) BfxRwLockInit(BfxRwLock* lock) {
    return _uvRwLockInit(lock);
}

BFX_API(void) BfxRwLockDestroy(BfxRwLock* lock) {
    _uvRwLockDestroy(lock);
}

// 读锁相关接口
BFX_API(int) BfxRwLockRTryLock(BfxRwLock* lock) {
    return _uvRwLockRTryLock(lock);
}

BFX_API(void) BfxRwLockRLock(BfxRwLock* lock) {
    _uvRwLockRLock(lock);
}

BFX_API(void) BfxRwLockRUnlock(BfxRwLock* lock) {
    _uvRwLockRUnlock(lock);
}

// 写锁相关接口
BFX_API(int) BfxRwLockWTryLock(BfxRwLock* lock) {
    return _uvRwLockWTryLock(lock);
}

BFX_API(void) BfxRwLockWLock(BfxRwLock* lock) {
    _uvRwLockWLock(lock);
}

BFX_API(void) BfxRwLockWUnlock(BfxRwLock* lock) {
	_uvRwLockWUnlock(lock);
}

#endif // !BFX_LOCK_CHECK
