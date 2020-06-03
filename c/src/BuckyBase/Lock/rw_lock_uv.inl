int _uvRwLockInit(uv_rwlock_t* lock) {
    int ret = uv_rwlock_init(lock);
    if (ret != 0) {
        BLOG_ERROR("init uv rwlock failed! err=%d", ret);
        return BFX_RESULT_FAILED;
    }

    return 0;
}

void _uvRwLockDestroy(uv_rwlock_t* lock) {
    assert(lock);

    uv_rwlock_destroy(lock);
}

// 读锁相关接口
int _uvRwLockRTryLock(uv_rwlock_t* lock) {
    int ret = uv_rwlock_tryrdlock(lock);
    if (ret != 0) {
        if (ret == UV_EBUSY) {
            ret = BFX_RESULT_BUSY;
        } else {
            BLOG_ERROR("uv rwlock try read lock error, ret=%d", ret);
            ret = BFX_RESULT_FAILED;
        }
    }

    return ret;
}

void _uvRwLockRLock(uv_rwlock_t* lock) {
    uv_rwlock_rdlock(lock);
}

void _uvRwLockRUnlock(uv_rwlock_t* lock) {
    uv_rwlock_rdunlock(lock);
}

// 写锁相关接口
int _uvRwLockWTryLock(uv_rwlock_t* lock) {
    int ret = uv_rwlock_trywrlock(lock);
    if (ret != 0) {
        if (ret == UV_EBUSY) {
            ret = BFX_RESULT_BUSY;
        } else {
            BLOG_ERROR("uv rwlock try write lock error, ret=%d", ret);
            ret = BFX_RESULT_FAILED;
        }
    }

    return ret;
}

void _uvRwLockWLock(uv_rwlock_t* lock) {
    uv_rwlock_wrlock(lock);
}

void _uvRwLockWUnlock(uv_rwlock_t* lock) {
    uv_rwlock_wrunlock(lock);
}