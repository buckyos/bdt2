#pragma once

#include "./thread_info.h"
#include "./msg_manager.h"


typedef struct {
    volatile int32_t ref;

    _IOThreadInfo* info;
    _MessageManager* msgManager;
} _ThreadMessageProxy;


_ThreadMessageProxy* _newThreadProxy(_IOThreadInfo* info);

int32_t _threadProxyAddRef(_ThreadMessageProxy* proxy);
int32_t _threadProxyRelease(_ThreadMessageProxy* proxy);

int _threadProxyPostMessage(_ThreadMessageProxy* proxy,
    BfxOnMessage onMessage, BfxUserData userData);