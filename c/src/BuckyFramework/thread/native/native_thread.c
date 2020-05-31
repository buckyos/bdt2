#include <BuckyBase.h>
#include "../bucky_thread.h"
#include "../common/thread_info.h"

int _uvTranslateError(int status) {
    if (status == 0) {
        return 0;
    }

    const char* msg = uv_err_name(status);
    printf("%s", msg);

    // TODO

    return -1;
}


BFX_API(uv_loop_t*) BfxThreadGetCurrentLoop() {
    _IOThreadInfo* info = (_IOThreadInfo*)_getCurrentIOThreadInfo();
    if (info == NULL) {
        return NULL;
    }

    return (uv_loop_t*)&info->loop->loop;
}