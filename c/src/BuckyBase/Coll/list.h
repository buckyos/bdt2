
typedef struct tagBfxListItem {
	struct tagBfxListItem *prev, *next;
} BfxListItem;

typedef struct {
	BfxListItem head;
	size_t count;
} BfxList;


BFX_API(void) BfxListInit(BfxList *list);

// 判断该元素是否在list中
BFX_API(bool) BfxListExists(const BfxList* list, const BfxListItem* target);

BFX_API(void) BfxListPushBack(BfxList *list, BfxListItem *item);
BFX_API(void) BfxListPushFront(BfxList *list, BfxListItem *item);

BFX_API(void) BfxListInsertAfter(BfxList* list, BfxListItem* pos, BfxListItem* item);
BFX_API(void) BfxListMultiInsertAfter(BfxList* list, BfxListItem* pos, BfxListItem** items, size_t count);


BFX_API(BfxListItem*) BfxListPopBack(BfxList* list);
BFX_API(BfxListItem*) BfxListPopFront(BfxList* list);
BFX_API(void) BfxListErase(BfxList* list, BfxListItem* item);

BFX_API(void) BfxListSwap(BfxList* list, BfxList* other);

// 获取元素个数
BFX_API(size_t) BfxListSize(const BfxList* list);
BFX_API(bool) BfxListIsEmpty(const BfxList* list);

// 清空所有元素
BFX_API(void) BfxListClear(BfxList* list);


// 合并两个列表
BFX_API(void) BfxListMerge(BfxList* list, BfxList* other, bool atBack);

BFX_API(void) BfxListMergeAfter(BfxList* list, BfxList* other, BfxListItem* pos);

// 遍历接口, [first, next, next, ..., last]
BFX_API(BfxListItem*) BfxListFirst(const BfxList* list);
BFX_API(BfxListItem*) BfxListNext(const BfxList* list, const BfxListItem *current);

// 反向遍历，[last, prev, prev, ..., first]
BFX_API(BfxListItem*) BfxListLast(const BfxList* list);
BFX_API(BfxListItem*) BfxListPrev(const BfxList* list, const BfxListItem* current);