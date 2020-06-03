int _uvThreadLockInit(uv_mutex_t* lock) {
    int ret = uv_mutex_init(lock);
    if (ret != 0) {
        BLOG_ERROR("init uv mutex error, ret=%d", ret);
        return BFX_RESULT_FAILED;
    }

    return 0;
}

void _uvThreadLockDestroy(uv_mutex_t* lock) {
    uv_mutex_destroy(lock);
}

int _uvThreadLockTryLock(uv_mutex_t* lock) {
    int ret = uv_mutex_trylock(lock);
    if (ret != 0) {
        if (ret == UV_EBUSY) {
            ret = BFX_RESULT_BUSY;
        } else {
            BLOG_ERROR("uv mutex trylock error, ret=%d", ret);
            ret = BFX_RESULT_FAILED;
        }
    }

    return ret;
}

void _uvThreadLockLock(uv_mutex_t* lock) {
    uv_mutex_lock(lock);
}

void _uvThreadLockUnlock(uv_mutex_t* lock) {
    uv_mutex_unlock(lock);
}
