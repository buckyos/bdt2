#include <BuckyBase.h>
#include "../bucky_thread.h"
#include "./at_exit.h"
#include "./thread_info.h"


typedef struct {
    BfxListItem stub;

    BfxThreadExitCallBack proc;
    BfxUserData userData;
} _AtExitItem;


_AtExitManager* _newAtExitManager() {
    _AtExitManager* manager = (_AtExitManager*)BfxMalloc(sizeof(_AtExitManager));
    BfxListInit(&manager->procList);

    return manager;
}

void _deleteAtExitManager(_AtExitManager* manager) {
    BfxFree(manager);
}

static _AtExitItem* _newAtExit(BfxThreadExitCallBack proc, BfxUserData userData) {
    _AtExitItem* item = (_AtExitItem*)BfxMalloc(sizeof(_AtExitItem));

    item->proc = proc;
    item->userData = userData;
    if (userData.lpfnAddRefUserData) {
        userData.lpfnAddRefUserData(userData.userData);
    }

    return item;
}

static void _deleteAtExit(_AtExitItem* item) {
    item->proc = NULL;

    if (item->userData.lpfnReleaseUserData) {
        item->userData.lpfnReleaseUserData(item->userData.userData);
    }

    BfxFree(item);
}

static void _pendAtExit(_AtExitManager* manager, BfxThreadExitCallBack proc, BfxUserData userData) {
    _AtExitItem* item = _newAtExit(proc, userData);
    BfxListPushFront(&manager->procList, &item->stub);
}

static void _execAtExit(_AtExitItem* item) {
    assert(item->proc);
    item->proc(item->userData.userData);

    _deleteAtExit(item);
}

void _execAtExits(_AtExitManager* manager) {
    if (BfxListSize(&manager->procList) == 0) {
        return;
    }

    BfxListItem* it = BfxListFirst(&manager->procList);
    do {
        _AtExitItem* item = (_AtExitItem*)it;
        it = BfxListNext(&manager->procList, it);

        _execAtExit(item);

    } while (it);

    BfxListClear(&manager->procList);
}

int _atExit(BfxThreadExitCallBack proc, BfxUserData userData) {
    _ThreadInfo* info = _getCurrentThreadInfo();
    if (info == NULL) {
        BLOG_ERROR("invalid thread for atExit!");
        return BFX_RESULT_NOT_SUPPORT;
    }

    if (info->atExitManager == NULL) {
        info->atExitManager = _newAtExitManager();
    }

    _pendAtExit(info->atExitManager, proc, userData);

    return 0;
}