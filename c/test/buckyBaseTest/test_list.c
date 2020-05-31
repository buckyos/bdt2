#include "../../src/BuckyBase/BuckyBase.h"

typedef struct {
    BfxListItem stub;
    size_t value;
}_TestListItem;

static _TestListItem* _newItem(size_t value) {
    _TestListItem* item = BFX_MALLOC_OBJ(_TestListItem);
    item->value = value;

    return item;
}

static void _printList(BfxList* list) {
    char buffer[1024];
    int pos = 0;
    for (BfxListItem* it = BfxListFirst(list); it; it = BfxListNext(list, it)) {
        _TestListItem* item = (_TestListItem*)it;
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%d ", (int)item->value);
    }

    buffer[pos] = 0;

    BLOG_INFO("list=%s", buffer);
}

static BfxListItem* _calcPos(BfxList* list, size_t pos) {
    BfxListItem* it = BfxListFirst(list);

    while (pos > 0) {
        it = BfxListNext(list, it);
        BLOG_CHECK(it);

        --pos;
    }

    return it;
}

void testList() {
    BfxList list;
    BfxListInit(&list);

    for (size_t i = 0; i < 10; ++i) {
        _TestListItem* item = _newItem(i);
        BfxListPushBack(&list, &item->stub);
    }

    _printList(&list);

    for (size_t i = 10; i < 15; ++i) {
        _TestListItem* item = _newItem(i);
        BfxListPushFront(&list, &item->stub);
    }

    _printList(&list);

    BfxListItem** items = BFX_MALLOC_ARRAY(BfxListItem*, 10);
    for (size_t i = 20; i < 25; ++i) {
        items[i - 20] = &_newItem(i)->stub;
    }

    
    BfxListMultiInsertAfter(&list, BfxListFirst(&list), items, 5);

    _printList(&list);

    BfxList other;
    BfxListInit(&other);
    for (size_t i = 30; i < 35; ++i) {
        BfxListPushFront(&other, &_newItem(i)->stub);
    }

    BfxListMergeAfter(&list, &other, _calcPos(&list, 3));

    _printList(&list);
}
