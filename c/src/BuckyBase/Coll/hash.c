
#include "../BuckyBase.h"
#include "./hash.h"


BFX_API(int) BfxHashTableInit(BfxHashTable* hashTable, size_t dim, BfxHashTableHashFunction hashFunc, BfxHashTableCompareFunction compareFunc, BfxUserData udata) {
    hashTable->hashVec = BFX_MALLOC(sizeof(BfxListItem) * dim);
    if (hashTable->hashVec == NULL) {
        BLOG_ERROR("malloc memory for hashtable vec failed! size=%d", sizeof(BfxListItem) * dim);
        return BFX_RESULT_FAILED;
    }
    memset(hashTable->hashVec, 0, sizeof(BfxListItem) * dim);

    hashTable->dim = dim;
    hashTable->compareFunc = compareFunc;
    hashTable->hashFunc = hashFunc;
    hashTable->udata = udata;
    if (udata.lpfnAddRefUserData) {
        udata.lpfnAddRefUserData(udata.userData);
    }

    return 0;
}

void BfxHashTableUninit(BfxHashTable* hashTable) {

    if (hashTable->udata.lpfnReleaseUserData) {
        hashTable->udata.lpfnReleaseUserData(hashTable->udata.userData);
    }

    BFX_FREE(hashTable->hashVec);
    hashTable->hashVec = NULL;
    hashTable->count = 0;
    hashTable->dim = 0;
}

static bool _isInPlace(BfxHashTable* hashTable, BfxListItem* item) {
    return (item >= hashTable->hashVec) && (item < (hashTable->hashVec + hashTable->dim));
}

static BfxListItem* _calcPlace(BfxHashTable* hashTable, BfxListItem* item) {
    size_t h = hashTable->hashFunc(item, hashTable->udata.userData);

    return hashTable->hashVec + h % hashTable->dim;
}

static void _listInsertAfter(BfxHashTable* hashTable, BfxListItem* pos, BfxListItem* item) {
    if (pos->next) {
        item->next = pos->next;
        item->prev = pos;

        pos->next->prev = item;
        pos->next = item;
    } else {
        item->next = item->prev = pos;
        pos->next = pos->prev = item;
    }

    ++hashTable->count;
}

static BfxListItem* _listFind(BfxHashTable* hashTable, BfxListItem* head, BfxListItem* item) {
    for (BfxListItem* it = head->next; it != NULL && it != head; it = it->next) {
        if (hashTable->compareFunc(it, item, hashTable->udata.userData) == 0) {
            return it;
        }
    }

    return NULL;
}

BFX_API(void) BfxHashTableAdd(BfxHashTable* hashTable, BfxListItem* item) {
    BfxListItem* list = _calcPlace(hashTable, item);
    _listInsertAfter(hashTable, list, item);
}

BFX_API(int) BfxHashTableAddIfNotMember(BfxHashTable* hashTable, BfxListItem* item, BfxListItem** cur) {
    BfxListItem* list = _calcPlace(hashTable, item);
    BfxListItem* curItem = _listFind(hashTable, list, item);
    if (curItem) {
        if (cur) {
            *cur = curItem;
        }

        return BFX_RESULT_ALREADY_EXISTS;
    }

    _listInsertAfter(hashTable, list, item);

    return 0;
}

static void _listRemoveItem(BfxHashTable* hashTable, BfxListItem* item) {
    BLOG_CHECK(!_isInPlace(hashTable, item));

    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->next = item->prev = NULL;

    BLOG_CHECK(hashTable->count > 0);
    --hashTable->count;
}

BFX_API(int) BfxHashTableRemove(BfxHashTable* hashTable, BfxListItem* item) {
    BfxListItem* list = _calcPlace(hashTable, item);

    BfxListItem* curItem = _listFind(hashTable, list, item);
    if (curItem == NULL) {
        return BFX_RESULT_NOT_FOUND;
    }

    _listRemoveItem(hashTable, curItem);

    return 0;
}

BFX_API(int) BfxHashTableRemoveUnsafe(BfxHashTable* hashTable, BfxListItem* item) {

    _listRemoveItem(hashTable, item);

    return 0;
}

BFX_API(int) BfxHashTableFind(BfxHashTable* hashTable, BfxListItem* item, BfxListItem** cur) {
    BfxListItem* list = _calcPlace(hashTable, item);

    BfxListItem* curItem = _listFind(hashTable, list, item);
    if (curItem == NULL) {
        return BFX_RESULT_NOT_FOUND;
    }

    if (cur) {
        *cur = curItem;
    }

    return 0;
}

BFX_API(size_t) BfxHashTableGetCount(BfxHashTable* hashTable) {
    return hashTable->count;
}

BFX_API(BfxListItem*) BfxHashTableFirst(BfxHashTable* hashTable) {
    BfxListItem* it = hashTable->hashVec, *end = hashTable->hashVec + hashTable->dim;

    bool found = false;
    for (;;) {
        if (it >= end) {
            break;
        }

        if (it->next && it->next != it) {
            BLOG_CHECK(it->prev && it->prev != it);
            found = true;
            break;
        }

        ++it;
    }

    if (!found) {
        return NULL;
    }

    return it->next;
}

BFX_API(BfxListItem*) BfxHashTableNext(BfxHashTable* hashTable, BfxListItem* it) {
    it = it->next;
    if (!_isInPlace(hashTable, it)) {
        return it;
    }

    BfxListItem* end = hashTable->hashVec + hashTable->dim;
    bool found = false;
    for (;;) {
        if (it >= end) {
            break;
        }

        if (it->next && it->next != it) {
            BLOG_CHECK(it->prev && it->prev != it);
            found = true;
            break;
        }

        ++it;
    }

    if (!found) {
        return NULL;
    }

    return it->next;
}

BFX_API(void) BfxHashTableClear(BfxHashTable* hashTable) {
    memset(hashTable->hashVec, 0, sizeof(BfxListItem) * hashTable->dim);
    hashTable->count = 0;
}