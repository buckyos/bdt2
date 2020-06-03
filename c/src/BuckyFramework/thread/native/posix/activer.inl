#include "./activer.h"

//////////////////////////////////////////////////////////////////////////
// epoll激活器

static int _setNoneBlocking(int fd) {
    BLOG_CHECK(fd >= 0);

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        flags = 0;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int _initActiver(_BuckyMessagePumpActiver* activer, int epoll) {
    BLOG_CHECK(epoll > 0);

    int socks[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, activer->sockets);
    BLOG_CHECK(ret == 0);
    if (ret < 0) {
        BLOG_ERROR("create activer socket pari failed! err=%d", errno);
        return BFX_RESULT_FAILED;
    }

    // 设置为非阻塞模式
    _setNoneBlocking(activer->sockets[0]);
    _setNoneBlocking(activer->sockets[1]);

    // 添加到epoll
    _epollAdd(epoll, activer->sockets[1], EPOLLIN, activer);

    return 0;
}

static void _uninitActiver(_BuckyMessagePumpActiver* activer, int epoll) {
    BLOG_CHECK(epoll > 0);

    BLOG_CHECK(activer->sockets[0] > 0);
    BLOG_CHECK(activer->sockets[1] > 0);

    _epollRemove(epoll, activer->sockets[1]);

    BFX_HANDLE_EINTR(close(activer->sockets[0]));
    BFX_HANDLE_EINTR(close(activer->sockets[1]));

    activer->sockets[0] = activer->sockets[1] = -1;
}

static int _activerActive(_BuckyMessagePumpActiver* activer) {
    BLOG_CHECK(activer->sockets[0] > 0);

    char dump = 0;
    int count = 0;

    int err = 0;

    for (;;) {
        int ret = BFX_HANDLE_EINTR(send(activer->sockets[0], &buf, sizeof(buf), MSG_DONTWAIT | MSG_NOSIGNAL));
        if (ret > 0) {
            BLOG_CHECK(ret == 1);
            break;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }

        BLOG_ERROR("send active byte failed! err=%d", errno);
        err = BFX_RESULT_FAILED;
        break;
    }

    return err;
}

static void _handleActive(_BuckyMessagePumpActiver* activer) {
    BLOG_CHECK(activer->sockets[1] > 0);

    for (;;) {
        char buf[4];
        int ret = BFX_HANDLE_EINTR(recv(activer->sockets[1], buf, sizeof(buf), MSG_DONTWAIT | MSG_NOSIGNAL));
        if (ret <= 0) {
            break;
        }
    }
}