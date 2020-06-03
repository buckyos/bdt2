#include "../../src/BuckyBase/BuckyBase.h"

static void testThreadLock() {
    BfxThreadLock lock;
    BfxThreadLockInit(&lock);

    BFX_THREADLOCK_CHECK_CURRENT_THREAD_UNOWNED(&lock);
    BfxThreadLockLock(&lock);
    
    BFX_THREADLOCK_CHECK_CURRENT_THREAD_OWNED(&lock);

    BfxThreadLockUnlock(&lock);

    BFX_THREADLOCK_CHECK_CURRENT_THREAD_UNOWNED(&lock);
}

static void testRwLock() {
    BfxRwLock lock;
    BfxRwLockInit(&lock);

    BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_UNOWNED(&lock);
    BfxRwLockRLock(&lock);
    BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_OWNED(&lock);
    BfxRwLockRUnlock(&lock);
    BFX_RWLOCK_READ_CHECK_CURRENT_THREAD_UNOWNED(&lock);

    BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_UNOWNED(&lock);
    BfxRwLockWLock(&lock);
    BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_OWNED(&lock);
    BfxRwLockWUnlock(&lock);
    BFX_RWLOCK_WRITE_CHECK_CURRENT_THREAD_UNOWNED(&lock);
}

void testLock() {
    testThreadLock();

    testRwLock();
}