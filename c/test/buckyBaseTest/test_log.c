#include "../src/BuckyFramework/thread/bucky_thread.h"
#include "../src/BuckyBase/BuckyBase.h"

void testLog() {

    SetLastError(3);
    BLOG_ERROR("got err");

    SetLastError(30);
    BLOG_ERROR("got err2");

    SetLastError(0);
    BLOG_ERROR("got no err");

    const char* name = "%isgraph returns true for the same cases as isprint except for the space character (' '), which returns true when checked with isprint but false when checked with isgraph.";
    BLOG_HEX(name, strlen(name), "test dump%s", name);

    for (int i = 0; i < 1024; ++i) {
        BLOG_INFO("first log %d", i);
        BfxThreadSleep(1000 * 10);
    }
}