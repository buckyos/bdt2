//////////////////////////////////////////////////////////////////////////
// epoll相关接口的简单封装

static int _epollCreate(int size) {
    int fd = epoll_create(maxEvents);
    if (fd < 0) {
        BLOG_ERROR("create epoll failed! err=%d", errno);
    }

    return fd;
}

static int _epollClose(int epoll) {
    int ret = BFX_HANDLE_EINTR(close(epoll));
    if (ret != 0) {
        BLOG_ERROR("close epoll failed! epoll=%d, err=%d", epoll, errno);
    }

    return ret;
}

static int _epollAdd(int epoll, int fd, int op, void* userData) {
    BLOG_CHECK(fd >= 0);

    struct epoll_event event;
    event.events = op | EPOLLET;
    event.data.ptr = userData;

    int ret = epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &event);
    BLOG_CHECK(ret >= 0);
    if (ret < 0) {
        BLOG_ERROR("call epoll_ctl for add file failed, epoll=%d, fd=%d, op=%d", epoll, fd, op);
        return BFX_RESULT_FAILED;
    }

    BLOG_INFO("call epoll_ctl for add file success! epoll=%d, fd=%d, op=%d", epoll, fd, op);

    return 0;
}

static int _epollRemove(int epoll, int fd) {
    BLOG_CHECK(fd >= 0);

    int ret = epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL);
    BLOG_CHECK(ret >= 0);
    if (ret < 0) {
        BLOG_ERROR("call epoll_ctl for remove file failed, epoll=%d, fd=%d", epoll, fd);
        return BFX_RESULT_FAILED;
    }

    BLOG_INFO("call epoll_ctl for remove file success! epoll=%d,  fd=%d", epoll, fd);

    return 0;
}

static int _epollCtrl(int epoll, int fd, int op) {
    BLOG_CHECK(epoll);
    BLOG_CHECK(fd >= 0);

    struct epoll_event event;
    event.events = op | EPOLLET;
    event.data.ptr = NULL;

    int ret = epoll_ctl(m_epollFD, EPOLL_CTL_MOD, fd, &event);
    BLOG_CHECK(ret >= 0);
    if (ret < 0) {
        BLOG_ERROR("call epoll_ctl failed! fd=%d, err=%d", fd, errno);
        return BFX_RESULT_FAILED;
    }

    BLOG_INFO("call epoll_ctl success! epoll=%d, fd=%d, op=%d", epoll, fd, op);

    return 0;
}

static int _epollWait(int epoll, struct epoll_event* events, int count, int64_t timeout) {
    BLOG_CHECK(epoll);
    BLOG_CHECK(count > 0);

    // 计算等待超时时长
    DWORD during;
    if (timeout >= 0) {
        during = timeout / 1000;
        if (timeout % 1000) {
            ++during;
        }
    } else if (timeout < 0) {
        // Specifying a timeout of - 1 causes epoll_wait() to block indefinitely, while specifying a timeout
        // equal to zero cause epoll_wait() to return immediately, even if no events are available.
        during = -1;
    }

    // 这里不再直接屏蔽signal导致的非正常返回，而是直接返回由消息泵处理，减少timeout的误差
    int ret = epoll_wait(epoll, events, count, during);
    if (ret < 0) {
        if (errno == EINTR) {
            ret = 0;
        } else {
            BLOG_ERROR("call epoll_wait got err! epoll=%d, err=%d", epoll, errno);
        }
    }

    return ret;
}
