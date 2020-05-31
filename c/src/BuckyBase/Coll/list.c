#include "../BuckyBase.h"

BFX_API(void) BfxListInit(BfxList* list)
{
	list->count = 0;
	list->head.next = list->head.prev = &list->head;
}

BFX_API(bool) BfxListExists(const BfxList *list, const BfxListItem *target) {
	const BfxListItem* it = list->head.next;
	while (it != &list->head) {
		if (it == target) {
			return true;
		}
		it = it->next;
	}

	return false;
}

BFX_API(void) BfxListInsertAfter(BfxList* list, BfxListItem *pos, BfxListItem *item) {
	assert(pos == &list->head || BfxListExists(list, pos));
	assert(!BfxListExists(list, item));

	item->next = pos->next;
	item->prev = pos;

	pos->next->prev = item;
	pos->next = item;

	++list->count;
}

BFX_API(void) BfxListMultiInsertAfter(BfxList* list, BfxListItem* pos, BfxListItem** items, size_t count) {

    for (size_t i = 0; i < count; ++i) {
        BfxListItem* item = items[i];
        BfxListInsertAfter(list, pos, item);
        pos = item;
    }
}

BFX_API(void) BfxListErase(BfxList* list, BfxListItem* item) {
	assert(BfxListExists(list, item));

	assert(list->head.next != &list->head);
	assert(list->head.prev != &list->head);

	item->next->prev = item->prev;
	item->prev->next = item->next;

	item->next = item->prev = NULL;

	assert(list->count > 0);
	--list->count;
}

BFX_API(void) BfxListSwap(BfxList* list, BfxList* other) {
    
    BfxListItem listHead = list->head;
    //BfxListItem otherHead = list->head;
    
    if (other->count == 0) {
        list->head.next = list->head.prev = &list->head;
    } else {
        list->head = other->head;
        list->head.next->prev = &list->head;
        list->head.prev->next = &list->head;
    }
    
    if (list->count == 0) {
        other->head.next = other->head.prev = &other->head;
    } else {
        other->head = listHead;
        other->head.next->prev = &other->head;
        other->head.prev->next = &other->head;
    }

	size_t size = list->count;
	list->count = other->count;
	other->count = size;
}

BFX_API(void) BfxListPushBack(BfxList* list, BfxListItem* item) {
	BfxListInsertAfter(list, list->head.prev, item);
}

BFX_API(void) BfxListPushFront(BfxList* list, BfxListItem* item) {
	BfxListInsertAfter(list, &list->head, item);
}

BFX_API(BfxListItem*) BfxListPopBack(BfxList* list) {
	BfxListItem* item = NULL;
	if (list->count > 0)
	{
		item = list->head.prev;
		BfxListErase(list, item);
	}

	return item;
}

BFX_API(BfxListItem*) BfxListPopFront(BfxList* list) {
	BfxListItem* item = NULL;
	if (list->count > 0)
	{
		item = list->head.next;
		BfxListErase(list, item);
	}

	return item;
}

BFX_API(size_t) BfxListSize(const BfxList* list) {
	assert(list->count >= 0);
	return list->count;
}

BFX_API(bool) BfxListIsEmpty(const BfxList* list) {
	assert(list->count >= 0);
	return list->count <= 0;
}

BFX_API(void) BfxListClear(BfxList* list) {
	list->count = 0;
	list->head.next = list->head.prev = &list->head;
}

BFX_API(BfxListItem*) BfxListFirst(const BfxList* list) {
	if (list->count == 0) {
		return NULL;
	}

	return list->head.next;
}

BFX_API(BfxListItem*) BfxListNext(const BfxList* list, const BfxListItem* current) {
	// assert(BfxListExists(list, current));

	assert(current != &list->head);
	if (current->next == &list->head) {
		return NULL;
	}

	return current->next;
}

BFX_API(BfxListItem*) BfxListLast(const BfxList* list) {
    if (list->count == 0) {
        return NULL;
    }

    return list->head.prev;
}

BFX_API(BfxListItem*) BfxListPrev(const BfxList* list, const BfxListItem* current) {
    assert(BfxListExists(list, current));

    assert(current != &list->head);
    if (current->prev == &list->head) {
        return NULL;
    }

    return current->prev;
}

BFX_API(void) BfxListMerge(BfxList* list, BfxList* other, bool atBack) {
    BLOG_CHECK(other);

    if (other->count == 0) {
        return;
    }

    if (atBack) {
        other->head.next->prev = list->head.prev;
        other->head.prev->next = &list->head;
        
        list->head.prev->next = other->head.next;
        list->head.prev = other->head.prev;
    } else {
        other->head.next->prev = &list->head;
        other->head.prev->next = list->head.next;

        list->head.next->prev = other->head.prev;
        list->head.next = other->head.next;
    }

    list->count += other->count;
    BfxListClear(other);
}

BFX_API(void) BfxListMergeAfter(BfxList* list, BfxList* other, BfxListItem* pos) {
    BLOG_CHECK(other);

    if (other->count == 0) {
        return;
    }

    other->head.next->prev = pos;
    other->head.prev->next = pos->next;

    pos->next->prev = other->head.prev;
    pos->next = other->head.next;

    list->count += other->count;
    BfxListClear(other);
}