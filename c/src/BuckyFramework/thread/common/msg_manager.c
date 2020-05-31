#include "../bucky_thread.h"
#include "./msg_manager.h"


typedef struct {
	BfxListItem stub;

    BfxOnMessage lpfnOnMessage;
	BfxUserData userData;
} _ThreadMessage;

static void _deleteMessage(_ThreadMessage* msg) {
    msg->lpfnOnMessage = NULL;
    if (msg->userData.lpfnReleaseUserData) {
        msg->userData.lpfnReleaseUserData(msg->userData.userData);
    }

    BfxFree(msg);
}

static _ThreadMessage* _newMessage(BfxOnMessage onMessage, BfxUserData userData) {
    assert(onMessage);

    _ThreadMessage* msg = (_ThreadMessage*)BfxMalloc(sizeof(_ThreadMessage));
    msg->lpfnOnMessage = onMessage;
    msg->userData = userData;
    if (userData.lpfnAddRefUserData) {
        userData.lpfnAddRefUserData(userData.userData);
    }

    return msg;
}

void _messageManagerInit(_MessageManager* manager) {
	BfxListInit(&manager->queue);
	uv_mutex_init(&manager->lock);
}

static void _clearMessage(_MessageManager* manager) {
    BfxList list;
    BfxListInit(&list);

    if (BfxListSize(&manager->queue) > 0) {
        uv_mutex_lock(&manager->lock);
        BfxListSwap(&manager->queue, &list);
        uv_mutex_unlock(&manager->lock);
    }

    BfxListItem* it = BfxListFirst(&list);
    while (it) {
        _ThreadMessage* msg = (_ThreadMessage*)it;
        it = BfxListNext(&list, it);

        _deleteMessage(msg);
    }
}

void _MessageManagerUninit(_MessageManager* manager) {
	_clearMessage(manager);
	uv_mutex_destroy(&manager->lock);
}

_MessageManager* _messageManagerNew() {
	_MessageManager* manager = (_MessageManager*)BfxMalloc(sizeof(_MessageManager));
	manager->ref = 1;

	_messageManagerInit(manager);

	return manager;
}

int32_t _messageManagerAddRef(_MessageManager* manager) {
	assert(manager->ref >= 1);
	return BfxAtomicInc32(&manager->ref);
}

int32_t _messageManagerRelease(_MessageManager* manager) {
	assert(manager->ref >= 1);
	int32_t ret = BfxAtomicDec32(&manager->ref);
	if (ret == 0) {
		_MessageManagerUninit(manager);
		BfxFree(manager);
	}

	return ret;
}

static void _pendMessage(_MessageManager* manager, _ThreadMessage* msg) {
	assert(msg);

	uv_mutex_lock(&manager->lock);
	BfxListPushBack(&manager->queue, &msg->stub);
	uv_mutex_unlock(&manager->lock);
}

static bool _peekMessage(_MessageManager* manager, BfxList *list) {

	if (BfxListSize(&manager->queue) > 0) {
		uv_mutex_lock(&manager->lock);
		BfxListSwap(&manager->queue, list);
		uv_mutex_unlock(&manager->lock);
	}
	
	return BfxListSize(list) > 0;
}

static void _execMessage(_ThreadMessage *msg) {
	assert(msg->lpfnOnMessage);

	msg->lpfnOnMessage(msg->userData.userData);
	
	_deleteMessage(msg);
}

static int _dispatchMessage(_MessageManager* manager) {
	BfxList list;
	BfxListInit(&list);

    int count = 0;
	while (_peekMessage(manager, &list)) {

        BfxListItem* it = BfxListFirst(&list);
        do {
            _ThreadMessage* msg = (_ThreadMessage*)it;
            it = BfxListNext(&list, it);

            _execMessage(msg);
            ++count;

        } while (it);

		BfxListClear(&list);
	}

    return count;
}

int _messageManagerDispatchMsg(_MessageManager* manager) {
    return _dispatchMessage(manager);
}

int _messageManagerPostMsg(_MessageManager* manager, BfxOnMessage onMessage,
    BfxUserData userData) {
    _ThreadMessage* msg = _newMessage(onMessage, userData);
    _pendMessage(manager, msg);

    return 0;
}

