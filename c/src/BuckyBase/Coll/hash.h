
#pragma once

// hash function for hashtable
typedef size_t (*BfxHashTableHashFunction)(const BfxListItem* item, void* udata);

// key�ıȽϺ���
// key(left) < key(right), return -1
// key(left) == key(right), return 0
// key(left) > key(right), return 1
typedef int (*BfxHashTableCompareFunction)(const BfxListItem* left, const BfxListItem* right, void* udata);


typedef struct BfxHashTable {
    BfxHashTableHashFunction hashFunc;
    BfxHashTableCompareFunction compareFunc;

    BfxListItem* hashVec;
    size_t dim;

    size_t count;

    BfxUserData udata;
} BfxHashTable;

BFX_API(int) BfxHashTableInit(BfxHashTable* hashTable, size_t dim, BfxHashTableHashFunction hashFunc, BfxHashTableCompareFunction compareFunc, BfxUserData udata);
BFX_API(void) BfxHashTableUninit(BfxHashTable* hashTable);


BFX_API(void) BfxHashTableAdd(BfxHashTable* hashTable, BfxListItem* item);
BFX_API(int) BfxHashTableAddIfNotMember(BfxHashTable* hashTable, BfxListItem* item, BfxListItem** cur);

// �Ƴ�keyItemָ����Ԫ��
BFX_API(int) BfxHashTableRemove(BfxHashTable* hashTable, BfxListItem* keyItem);

// ֱ���Ƴ�item��item������hashTable�����Ԫ��
BFX_API(int) BfxHashTableRemoveUnsafe(BfxHashTable* hashTable, BfxListItem* item);


// ����itemָ����Ԫ���Ƿ����
BFX_API(int) BfxHashTableFind(BfxHashTable* hashTable, BfxListItem* item, BfxListItem** cur);

BFX_API(size_t) BfxHashTableGetCount(BfxHashTable* hashTable);

// ��������Ԫ��
BFX_API(BfxListItem*) BfxHashTableFirst(BfxHashTable* hashTable);
BFX_API(BfxListItem*) BfxHashTableNext(BfxHashTable* hashTable, BfxListItem* it);

// ֱ����գ��ⲿ��Ҫ�Լ��ͷ�ÿһ��Ԫ��
BFX_API(void) BfxHashTableClear(BfxHashTable* hashTable);