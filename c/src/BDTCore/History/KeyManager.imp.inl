
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in Module.c of history module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include <assert.h>
#include "./KeyManager.h"
#include "./Storage.h"
#include <SGLib/SGLib.h>
#include <mbedtls/sha256.h>

#define MIX_HASH_WITH_APPEND_COUNT			(BDT_MIX_HASH_4_KEY_LIMIT - 1)
#define MIX_HASH_REDUNDANCY_MINUTES			((BDT_MIX_HASH_4_KEY_LIMIT - 1) >> 1) 

typedef struct MixHashInfo
{
	uint8_t hash[BDT_MIX_HASH_LENGTH];
	union
	{
		uint32_t minutes;
		uint8_t append[4];
	};
} MixHashInfo;

typedef struct MixHashKeyMap MixHashKeyMapNode;

typedef struct KeyNode
{
	BdtHistory_KeyInfo keyInfo;
	union
	{
		MixHashKeyMapNode* mixHashNodes[BDT_MIX_HASH_4_KEY_LIMIT];
		struct
		{
			MixHashKeyMapNode* originalHashNode;
			MixHashKeyMapNode* mixHashNodesWithAppend[BDT_MIX_HASH_4_KEY_LIMIT - 1];
			uint32_t nextUpdateHash; // 下次要更新的HASH
		} mixHashs4Update;
	};
	uint32_t hashCount;

	struct KeyNode* previous;
	struct KeyNode* next;
	struct KeyNode* nextAesKey; // 归属同一个PEER的KEY列表
} KeyNode;

// <mixHash-KeyNode>
typedef struct MixHashKeyMapValue
{
	KeyNode* keyNode;
	MixHashInfo mixHashInfo;
} MixHashKeyMapValue;

typedef struct MixHashKeyMap
{
	struct MixHashKeyMap* left;
	struct MixHashKeyMap* right;
	int color;
	MixHashKeyMapValue value;
} MixHashKeyMap, MixHashKeyMapNode;

// <peerid-KeyNode>
typedef struct PeeridKeyMapValue
{
	BdtPeerid peerid;
	KeyNode* keyNodeList;
} PeeridKeyMapValue;

typedef struct PeeridKeyMap
{
	struct PeeridKeyMap* left;
	struct PeeridKeyMap* right;
	int color;
	PeeridKeyMapValue value;
} PeeridKeyMap;

static int key_node_compare(KeyNode* left, KeyNode* right)
{
	assert(FALSE);
	return 0;
}

typedef KeyNode key_node;
SGLIB_DEFINE_DL_LIST_PROTOTYPES(key_node, key_node_compare, previous, next)
SGLIB_DEFINE_DL_LIST_FUNCTIONS(key_node, key_node_compare, previous, next)

// 归属同一个PEER的KEY列表
typedef KeyNode key4peer_node;
SGLIB_DEFINE_LIST_PROTOTYPES(key4peer_node, key_node_compare, nextAesKey)
SGLIB_DEFINE_LIST_FUNCTIONS(key4peer_node, key_node_compare, nextAesKey)

typedef MixHashKeyMap mix_hash_key_map;
static int mix_hash_key_map_compare(const MixHashKeyMap* left, const MixHashKeyMap* right)
{
	return memcmp(left->value.mixHashInfo.hash, right->value.mixHashInfo.hash, BDT_MIX_HASH_LENGTH);
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(mix_hash_key_map, left, right, color, mix_hash_key_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(mix_hash_key_map, left, right, color, mix_hash_key_map_compare)

typedef PeeridKeyMap peerid_key_map;
static int peerid_key_map_compare(const PeeridKeyMap* left, const PeeridKeyMap* right)
{
	return memcmp(&left->value.peerid, &right->value.peerid, BDT_PEERID_LENGTH);
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(peerid_key_map, left, right, color, peerid_key_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(peerid_key_map, left, right, color, peerid_key_map_compare)


typedef struct BdtHistory_KeyManager {
	BdtStorage* storage;
	const BdtAesKeyConfig* config;

	MixHashKeyMap* mixHashKeyMap; // <mixHash-key>
	PeeridKeyMap* peeridKeyMap; // <peerid-key>
	struct
	{
		KeyNode* header; // 无超时HASH的KEY列表头
		KeyNode* tail; // 链表尾
		uint32_t firstHashMinutes; // 第一个KEY计算HASH的时间(分钟数)
	} keyList;
	KeyNode* rehashKeyList; // 需要重新HASH的KEY列表
	BfxRwLock lock;
} BdtHistory_KeyManager, KeyManager;

/*
	aesKey<*--1>peerid
*/

static void MixHashKeyMapDestroy(MixHashKeyMap* map);
static void PeeridKeyMapDestroy(PeeridKeyMap* map);
static void KeyNodeListDestroy(KeyNode* list);
static KeyNode* KeyNodeCreate(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const PeeridKeyMapValue* peeridKeyMapValue);
static void KeyNodeDestroy(KeyNode* node);
static bool CalcKeyHashImpl(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const uint8_t* appendBuffer,
	uint32_t appendSize,
	uint8_t hash[BDT_MIX_HASH_LENGTH]);
static uint32_t KeyManagerAddNewKeyNoLock(KeyManager* keyManager,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const BdtPeerid* peerid,
	PeeridKeyMap* existPeerKeyNode,
	KeyNode** outAddKeyNode);
static void KeyManagerGenerateAesKey(uint8_t aesKey[BDT_AES_KEY_LENGTH]);
static uint32_t KeyManagerRehashAesKey(KeyNode* keyNode,
	MixHashKeyMap* newHashNodes[BDT_MIX_HASH_4_KEY_LIMIT],
	MixHashKeyMap* deadHashNodes[BDT_MIX_HASH_4_KEY_LIMIT]);
static PeeridKeyMap* KeyManagerClearExpireKeyNoLock(KeyManager* keyManager,
	PeeridKeyMap* peeridKeyNode/* = NULL*/,
	KeyNode* keyNode /* = NULL*/);
static void KeyManagerRemoveKeyNodeFromList(KeyManager* keyManager, KeyNode* keyNode, uint32_t minHashMinutes);
static PeeridKeyMap* KeyManagerFindKeyByPeeridNoLock(KeyManager* keyManager,
	const BdtPeerid* peerid);
static void KeyManagerDeletePeer(KeyManager* keyManager,
	PeeridKeyMap* peeridKeyMapNode);
static void KeyManagerDeleteKey(KeyManager* keyManager,
	KeyNode* keyNode,
	uint32_t minHashMinutes);
static void KeyManagerDeleteMixHash(KeyManager* keyManager,
	MixHashKeyMap* mixHashNode,
	bool freeNode);

static void KeyManagerMergeOuterKeys(
	KeyManager* keyManager,
	BdtStorage* storage,
	const BdtHistory_KeyInfo* outerKeys[],
	size_t keyCount
);

static BdtHistory_KeyInfo* KeyManagerAddOuterKey(
	KeyManager* keyManager,
	const BdtHistory_KeyInfo* outerKeyInfo
);
static bool KeyManagerUpdateKey(
	BdtHistory_KeyInfo* keyInfo,
	bool confirmed,
	int64_t expireTime);

static bool KeyManagerPrepareForStorage(KeyManager* keyManager, BdtHistory_KeyInfo* keyInfo, bool* canStorage);
static void KeyManagerStorageUpdateAesKey(
	KeyManager* keyManager,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
);

static void KeyManagerInit(
	KeyManager* keyManager,
	const BdtAesKeyConfig* config
)
{
	memset(keyManager, 0, sizeof(KeyManager));
	BfxRwLockInit(&keyManager->lock);
	keyManager->config = config;
}

static void KeyManagerUninit(KeyManager* keyManager)
{
	MixHashKeyMapDestroy(keyManager->mixHashKeyMap);
	keyManager->mixHashKeyMap = NULL;
	PeeridKeyMapDestroy(keyManager->peeridKeyMap);
	keyManager->peeridKeyMap = NULL;

	KeyNodeListDestroy(keyManager->keyList.header);
	keyManager->keyList.header = NULL;
	keyManager->keyList.tail = NULL;
	KeyNodeListDestroy(keyManager->rehashKeyList);
	keyManager->rehashKeyList = NULL;

	BfxRwLockDestroy(&keyManager->lock);
}

KeyManager* BdtHistory_KeyManagerCreate(const BdtAesKeyConfig* config)
{
	KeyManager* keyManager = BFX_MALLOC_OBJ(KeyManager);
	KeyManagerInit(keyManager, config);
	return keyManager;
}

void BdtHistory_KeyManagerDestroy(KeyManager* keyManager)
{
	KeyManagerUninit(keyManager);
	BfxFree(keyManager);
}

// 从等待重算hash的表中，取第一个重算HASH，如果命中指定mixHash，返回相应key信息；
// 该函数太长，且需要加锁，为提高可读性把它从KeyManagerFindByMixHash函数里抽取出来；
// 原则上只被KeyManagerFindByMixHash做子函数调用
static uint32_t KeyManagerFindByMixHashInNextRehashNodeNoLock(
	KeyManager* keyManager,
	const uint8_t mixHash[BDT_MIX_HASH_LENGTH],
	KeyNode** outFoundKeyNode,
	uint32_t* newHashCount
)
{
	*newHashCount = 0;
	uint32_t i = 0;
	MixHashKeyMap* newHashNodes[BDT_MIX_HASH_4_KEY_LIMIT];
	MixHashKeyMap* deadHashNodes[BDT_MIX_HASH_4_KEY_LIMIT];
	KeyNode* node = keyManager->rehashKeyList;
	uint32_t result = BFX_RESULT_NOT_FOUND;

	if (node == NULL)
	{
		return BFX_RESULT_NOT_FOUND;
	}

	sglib_key_node_delete(&keyManager->rehashKeyList, node);
	// 1.重新计算HASH，2.搜索新计算出来的HASH是否匹配
	*newHashCount = KeyManagerRehashAesKey(node, newHashNodes, deadHashNodes);
	for (i = 0; i < *newHashCount; i++)
	{
		KeyManagerDeleteMixHash(keyManager, deadHashNodes[i], true);

		// 添加失败设定一个标记，在keynode销毁或者hash超时再删除
		newHashNodes[i]->color = RBTREE_COLOR_INVALID;
		sglib_mix_hash_key_map_add(&keyManager->mixHashKeyMap, newHashNodes[i]);
		assert(newHashNodes[i]->color != RBTREE_COLOR_INVALID);
		if (newHashNodes[i]->color != RBTREE_COLOR_INVALID &&
			memcmp(newHashNodes[i]->value.mixHashInfo.hash, mixHash, BDT_MIX_HASH_LENGTH) == 0)
		{
			*outFoundKeyNode = newHashNodes[i]->value.keyNode;
			result = BFX_RESULT_SUCCESS;
		}
	}

	// 命中的放表头，没命中的放表尾，大体上维持最近被访问的KEY比较靠前
	if (result == BFX_RESULT_SUCCESS)
	{
		sglib_key_node_add_before(&keyManager->keyList.header, node);
		if (keyManager->keyList.tail == NULL)
		{
			keyManager->keyList.firstHashMinutes = BDT_MINUTES_NOW;
			keyManager->keyList.tail = node;
		}
	}
	else
	{
		sglib_key_node_add_after(&keyManager->keyList.tail, node);
		if (keyManager->keyList.header == NULL)
		{
			keyManager->keyList.firstHashMinutes = BDT_MINUTES_NOW;
			keyManager->keyList.header = node;
		}
	}
	return result;
}

// 按mixhash搜索，但不重算HASH
static uint32_t KeyManagerFindByMixHashNoRehashNoLock(
	KeyManager* keyManager,
	const uint8_t mixHash[BDT_MIX_HASH_LENGTH],
	KeyNode** outFoundKeyNode
)
{
	MixHashKeyMap mixHashMapNode;
	MixHashInfo* mixHashInfo = &mixHashMapNode.value.mixHashInfo;
	memset(&mixHashMapNode, 0, sizeof(mixHashMapNode));
	mixHashInfo->minutes = 0;
	memcpy(mixHashInfo->hash, mixHash, BDT_MIX_HASH_LENGTH);

	MixHashKeyMap* targetMixHashMapNode = NULL;
	uint32_t result = BFX_RESULT_NOT_FOUND;

	targetMixHashMapNode = sglib_mix_hash_key_map_find_member(keyManager->mixHashKeyMap, &mixHashMapNode);
	if (targetMixHashMapNode != NULL)
	{
		assert(targetMixHashMapNode->value.keyNode != NULL);
		*outFoundKeyNode = targetMixHashMapNode->value.keyNode;
		result = BFX_RESULT_SUCCESS;
	}

	return result;
}

static void KeyManagerMoveAllKeyToRehashNoLock(KeyManager* keyManager)
{
	// 合并待HASH KEY和过期的HASH（只要有一个HASH过期就全部算过期）
	// mixhash命中失败后将在HASH过期的列表中搜索
	keyManager->keyList.tail->next = keyManager->rehashKeyList;
	if (keyManager->rehashKeyList != NULL)
	{
		keyManager->rehashKeyList->previous = keyManager->keyList.tail;
	}
	keyManager->rehashKeyList = keyManager->keyList.header;
	keyManager->keyList.header = NULL;
	keyManager->keyList.tail = NULL;
}

uint32_t BdtHistory_KeyManagerFindByMixHash(
	KeyManager* keyManager,
	const uint8_t mixHash[BDT_MIX_HASH_LENGTH],
	uint8_t outKey[BDT_AES_KEY_LENGTH],
	BdtPeerid* outPeerid,
	// if touch stub in manager to extend key's expire time
	bool touch,
	// confirm the key by remote
	bool confirmedByRemote
)
{
	/*
	0.如果该hash已经在搜索，等待完成即可
	1.按hash在map里搜索
	2.在没有更新hash的map里搜索，同步更新hash
	*/
	bool changed = false;
	bool isNewStorage = false;
	bool canStorage = false;
	KeyNode* keyNode = NULL;
	uint32_t result = BFX_RESULT_NOT_FOUND;
	int64_t expireTime = 0;
	uint32_t flags = 0;
	int64_t lastAccessTime = 0;

	{
		BfxRwLockRLock(&keyManager->lock);

		result = KeyManagerFindByMixHashNoRehashNoLock(
			keyManager,
			mixHash,
			&keyNode
		);

		if (result == BFX_RESULT_SUCCESS)
		{
			BLOG_CHECK(keyNode != NULL);

			BdtHistory_KeyInfo* keyInfo = &keyNode->keyInfo;
			memcpy(outKey, keyInfo->aesKey, BDT_AES_KEY_LENGTH);
			*outPeerid = *keyInfo->peerid;

			keyInfo->lastAccessTime = BDT_MICRO_SECONDS_2_SECONDS(BfxTimeGetNow(false));
			changed = KeyManagerUpdateKey(keyInfo,
						confirmedByRemote,
						touch ? keyInfo->lastAccessTime + (int64_t)keyManager->config->activeTime : 0);

			isNewStorage = KeyManagerPrepareForStorage(keyManager, keyInfo, &canStorage);

			expireTime = keyInfo->expireTime;
			flags = keyInfo->flagU32;
			lastAccessTime = keyInfo->lastAccessTime;
		}

		BfxRwLockRUnlock(&keyManager->lock);
	}

	if (result == BFX_RESULT_SUCCESS)
	{
		if (changed && canStorage)
		{
			KeyManagerStorageUpdateAesKey(keyManager,
				outPeerid,
				outKey,
				expireTime,
				flags,
				lastAccessTime,
				isNewStorage);
		}
		return BFX_RESULT_SUCCESS;
	}

	const int64_t nowMicroSeconds = BfxTimeGetNow(FALSE);
	const uint32_t nowMinutes = BDT_MICRO_SECONDS_2_MINUTES(nowMicroSeconds);
	const int64_t nowSeconds = BDT_MICRO_SECONDS_2_SECONDS(nowMicroSeconds);

	BfxRwLockWLock(&keyManager->lock);

	if (nowMinutes - keyManager->keyList.firstHashMinutes > 0 &&
		keyManager->keyList.tail != NULL)
	{
		KeyManagerMoveAllKeyToRehashNoLock(keyManager);
	}

	BfxRwLockWUnlock(&keyManager->lock);

	if (keyManager->rehashKeyList != NULL)
	{
		uint32_t newHashCount = 0;

		BfxRwLockWLock(&keyManager->lock);

		while (keyManager->rehashKeyList != NULL && result != BFX_RESULT_SUCCESS)
		{
			result = KeyManagerFindByMixHashInNextRehashNodeNoLock(
				keyManager,
				mixHash,
				&keyNode,
				&newHashCount
			);

			if (result == BFX_RESULT_SUCCESS)
			{
				BLOG_CHECK(keyNode != NULL);

				BdtHistory_KeyInfo* keyInfo = &keyNode->keyInfo;
				memcpy(outKey, keyInfo->aesKey, BDT_AES_KEY_LENGTH);
				*outPeerid = *keyInfo->peerid;

				keyInfo->lastAccessTime = nowSeconds;
				changed = KeyManagerUpdateKey(keyInfo,
					confirmedByRemote,
					touch ? keyInfo->lastAccessTime + (int64_t)keyManager->config->activeTime : 0);

				expireTime = keyInfo->expireTime;
				flags = keyInfo->flagU32;
				lastAccessTime = keyInfo->lastAccessTime;
			}

			if (newHashCount > 0)
			{
				// 重算hash可能耗时比较长，释放锁，让其他线程有机会处理
				BfxRwLockWUnlock(&keyManager->lock);
				BfxRwLockWLock(&keyManager->lock);
			}
		}

		BfxRwLockWUnlock(&keyManager->lock);
	}

	if (result != BFX_RESULT_SUCCESS)
	{
		// 最后再从map里搜索一遍；
		// 因为重算HASH过程中，可能被其他线程进入，也计算了部分HASH
		BfxRwLockRLock(&keyManager->lock);

		result = KeyManagerFindByMixHashNoRehashNoLock(
			keyManager,
			mixHash,
			&keyNode
		);

		if (result == BFX_RESULT_SUCCESS)
		{
			BLOG_CHECK(keyNode != NULL);

			BdtHistory_KeyInfo* keyInfo = &keyNode->keyInfo;
			memcpy(outKey, keyInfo->aesKey, BDT_AES_KEY_LENGTH);
			*outPeerid = *keyInfo->peerid;

			keyInfo->lastAccessTime = nowSeconds;
			changed = KeyManagerUpdateKey(keyInfo,
				confirmedByRemote,
				touch ? keyInfo->lastAccessTime + (int64_t)keyManager->config->activeTime : 0);

			isNewStorage = KeyManagerPrepareForStorage(keyManager, keyInfo, &canStorage);
			expireTime = keyInfo->expireTime;
			flags = keyInfo->flagU32;
			lastAccessTime = keyInfo->lastAccessTime;
		}

		BfxRwLockRUnlock(&keyManager->lock);
	}

	if (changed && canStorage && result == BFX_RESULT_SUCCESS)
	{
		KeyManagerStorageUpdateAesKey(keyManager,
			outPeerid,
			outKey,
			expireTime,
			flags,
			lastAccessTime,
			isNewStorage
		);
	}


	{
		BLOG_DEBUG("find key:%02x%02x%02x%02x%02x%02x%02x%02x in key manager result %u",
			mixHash[0],
			mixHash[1],
			mixHash[2],
			mixHash[3],
			mixHash[4],
			mixHash[5],
			mixHash[6],
			mixHash[7], 
			result);
	}

	return result;
}

static KeyNode* KeyManagerFindLastAccessKeyByRemoteNoLock(KeyManager* keyManager,
	const BdtPeerid* peerid,
	PeeridKeyMap** peeridKeyMapNode)
{
	PeeridKeyMap peeridMapNode = { .value = {.peerid = *peerid} };

	PeeridKeyMap* existPeerKeyNode = NULL;
	KeyNode* node = NULL;
	uint32_t result = BFX_RESULT_NOT_FOUND;
	int64_t nowSeconds = BDT_MICRO_SECONDS_2_SECONDS(BfxTimeGetNow(FALSE));
	existPeerKeyNode = sglib_peerid_key_map_find_member(keyManager->peeridKeyMap, &peeridMapNode);
	if (existPeerKeyNode != NULL)
	{
		*peeridKeyMapNode = existPeerKeyNode;
		KeyNode* lastAccessKeyNode = (node = existPeerKeyNode->value.keyNodeList);
		assert(lastAccessKeyNode != NULL);
		while ((node = node->nextAesKey) != NULL)
		{
			if (node->keyInfo.lastAccessTime > lastAccessKeyNode->keyInfo.lastAccessTime)
			{
				lastAccessKeyNode = node;
			}
		}

		if (nowSeconds < lastAccessKeyNode->keyInfo.expireTime)
		{
			lastAccessKeyNode->keyInfo.lastAccessTime = nowSeconds;
			return lastAccessKeyNode;
		}
	}
	return NULL;
}

uint32_t BdtHistory_KeyManagerGetKeyByRemote(
	KeyManager* keyManager,
	const BdtPeerid* peerid,
	uint8_t outKey[BDT_AES_KEY_LENGTH],
	// in: if create a new key for remote when no key exists 
	// out: if set to true when input, output value of this param shows if the key returned is newly created
	bool* inoutNew,
	// out: set 'true' when the key is confirmed by remote
	bool* outIsConfirmByRemote,
	// if touch stub in manager to extend key's expire time
	bool touch
)
{
	*outIsConfirmByRemote = false;
	uint32_t result = BFX_RESULT_NOT_FOUND;
	PeeridKeyMap* peeridKeyMapNode = NULL;
	bool changed = false;
	bool isNewStorage = false;
	bool canStorage = false;
	int64_t expireTime = 0;
	uint32_t flags = 0;
	int64_t lastAccessTime = 0;

	BfxRwLockRLock(&keyManager->lock);

	KeyNode* node = KeyManagerFindLastAccessKeyByRemoteNoLock(keyManager,
		peerid,
		&peeridKeyMapNode);

	if (node != NULL)
	{
		BdtHistory_KeyInfo* keyInfo = &node->keyInfo;
		memcpy(outKey, keyInfo->aesKey, BDT_AES_KEY_LENGTH);
		*inoutNew = false;
		*outIsConfirmByRemote = (bool)keyInfo->flags.isConfirmed;

		changed = KeyManagerUpdateKey(keyInfo,
			false,
			touch? keyInfo->lastAccessTime + (int64_t)keyManager->config->activeTime : 0);

		isNewStorage = KeyManagerPrepareForStorage(keyManager, keyInfo, &canStorage);
		expireTime = keyInfo->expireTime;
		flags = keyInfo->flagU32;
		lastAccessTime = keyInfo->lastAccessTime;

		result = BFX_RESULT_SUCCESS;

		assert(peeridKeyMapNode != NULL);
		if (peeridKeyMapNode->value.keyNodeList->nextAesKey != NULL)
		{
			// 不止一个key，很可能有超时
			KeyManagerClearExpireKeyNoLock(keyManager,
				peeridKeyMapNode,
				NULL);
		}
	}

	BfxRwLockRUnlock(&keyManager->lock);

	if (node != NULL || !*inoutNew)
	{
		if (changed && canStorage)
		{
			KeyManagerStorageUpdateAesKey(keyManager,
				peerid,
				outKey,
				expireTime,
				flags,
				lastAccessTime,
				isNewStorage
				);
		}
		return result;
	}

	{
		KeyManagerGenerateAesKey(outKey);

		BfxRwLockWLock(&keyManager->lock);

		if (peeridKeyMapNode != NULL)
		{
			// 超时清理
			peeridKeyMapNode = KeyManagerClearExpireKeyNoLock(keyManager,
				KeyManagerFindKeyByPeeridNoLock(keyManager, peerid),
				NULL);
		}

		result = KeyManagerAddNewKeyNoLock(keyManager, outKey, peerid, peeridKeyMapNode, &node);

		isNewStorage = KeyManagerPrepareForStorage(keyManager, &node->keyInfo, &canStorage);
		expireTime = node->keyInfo.expireTime;
		flags = node->keyInfo.flagU32;
		lastAccessTime = node->keyInfo.lastAccessTime;

		BfxRwLockWUnlock(&keyManager->lock);
	}

	if (canStorage && (result == BFX_RESULT_SUCCESS || result == BFX_RESULT_ALREADY_EXISTS))
	{
		KeyManagerStorageUpdateAesKey(keyManager,
			peerid,
			outKey,
			expireTime,
			flags,
			lastAccessTime,
			true);
	}
	return result;
}

static PeeridKeyMap* KeyManagerFindKeyByPeeridNoLock(KeyManager* keyManager,
	const BdtPeerid* peerid)
{
	PeeridKeyMap peeridMapNodeStack = { .value = {.peerid = *peerid } };

	return sglib_peerid_key_map_find_member(keyManager->peeridKeyMap, &peeridMapNodeStack);
}

static KeyNode* KeyManagerFindAesKeyNoLock(KeyManager* keyManager,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const BdtPeerid* peerid,
	PeeridKeyMap** outExistPeerKeyNode)
{
	KeyNode* keyNode = NULL;

	PeeridKeyMap* existPeerKeyNode = *outExistPeerKeyNode;
	if (existPeerKeyNode == NULL)
	{
		existPeerKeyNode = KeyManagerFindKeyByPeeridNoLock(keyManager, peerid);
	}
	else
	{
		assert(memcmp(&existPeerKeyNode->value.peerid, peerid, sizeof(BdtPeerid)) == 0);
	}

	if (existPeerKeyNode != NULL)
	{
		// 所有key共享相同的PeerInfo对象
		assert(existPeerKeyNode->value.keyNodeList != NULL);

		for (keyNode = existPeerKeyNode->value.keyNodeList;
			keyNode != NULL && memcmp(keyNode->keyInfo.aesKey, aesKey, BDT_AES_KEY_LENGTH) != 0;
			keyNode = keyNode->nextAesKey);
	}

	*outExistPeerKeyNode = existPeerKeyNode;
	return keyNode;
}

uint32_t BdtHistory_KeyManagerAddKey(KeyManager* keyManager,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const BdtPeerid* peerid, 
	bool confirmedByRemote)
{
	{
		uint8_t mixHash[BDT_MIX_HASH_LENGTH];
		BdtHistory_KeyManagerCalcKeyHash(aesKey, mixHash);
		char szPeerid[BDT_PEERID_STRING_LENGTH + 1];
		BdtPeeridToString(peerid, szPeerid);
		BLOG_DEBUG("add peerid:%s key:%02x%02x%02x%02x%02x%02x%02x%02x to key manager",
			szPeerid,
			mixHash[0],
			mixHash[1],
			mixHash[2],
			mixHash[3],
			mixHash[4],
			mixHash[5],
			mixHash[6],
			mixHash[7]);
	}
	uint32_t result = BFX_RESULT_FAILED;
	KeyNode* keyNode = NULL;
	bool changed = false;
	bool isNewStorage = false;
	bool canStorage = false;
	int64_t expireTime = 0;
	uint32_t flags = 0;
	int64_t lastAccessTime = 0;

	{
		BfxRwLockRLock(&keyManager->lock);

		PeeridKeyMap* existPeeridMapNode = NULL;
		keyNode = KeyManagerFindAesKeyNoLock(keyManager, aesKey, peerid, &existPeeridMapNode);
		if (keyNode != NULL)
		{
			BdtHistory_KeyInfo* keyInfo = &keyNode->keyInfo;
			keyNode->keyInfo.lastAccessTime = BDT_MICRO_SECONDS_2_SECONDS(BfxTimeGetNow(FALSE));
			changed = KeyManagerUpdateKey(keyInfo,
				confirmedByRemote,
				keyInfo->lastAccessTime + (int64_t)keyManager->config->activeTime);

			isNewStorage = KeyManagerPrepareForStorage(keyManager, keyInfo, &canStorage);
			expireTime = keyInfo->expireTime;
			flags = keyInfo->flagU32;
			lastAccessTime = keyInfo->lastAccessTime;
		}

		BfxRwLockRUnlock(&keyManager->lock);
	}

	if (keyNode != NULL && canStorage)
	{
		KeyManagerStorageUpdateAesKey(keyManager,
			peerid,
			aesKey,
			expireTime,
			flags,
			lastAccessTime,
			isNewStorage);
		return BFX_RESULT_ALREADY_EXISTS;
	}

	{
		changed = true;

		BfxRwLockWLock(&keyManager->lock);
		result = KeyManagerAddNewKeyNoLock(keyManager, aesKey, peerid, NULL, &keyNode);
		if (result == BFX_RESULT_ALREADY_EXISTS || result == BFX_RESULT_SUCCESS)
		{
			BLOG_CHECK(keyNode != NULL);
			if (confirmedByRemote)
			{
				keyNode->keyInfo.flags.isConfirmed = 1;
			}

			isNewStorage = KeyManagerPrepareForStorage(keyManager, &keyNode->keyInfo, &canStorage);
			expireTime = keyNode->keyInfo.expireTime;
			flags = keyNode->keyInfo.flagU32;
			lastAccessTime = keyNode->keyInfo.lastAccessTime;
		}
		BfxRwLockWUnlock(&keyManager->lock);
	}

	if ((result == BFX_RESULT_SUCCESS || result == BFX_RESULT_ALREADY_EXISTS) && canStorage)
	{
		KeyManagerStorageUpdateAesKey(
			keyManager,
			peerid,
			aesKey,
			expireTime,
			flags,
			lastAccessTime,
			true);
	}
	return result;
}

static void MixHashKeyMapDestroy(MixHashKeyMap* map)
{
	mix_hash_key_map* node = map;
	while (node != NULL)
	{
		sglib_mix_hash_key_map_delete(&map, node);
		// BFX_FREE(node); 在keyNode里释放内存
		node = map;
	}
	assert(map == NULL);
}

static void PeeridKeyMapDestroy(PeeridKeyMap* map)
{
	peerid_key_map* node = map;
	while (node != NULL)
	{
		sglib_peerid_key_map_delete(&map, node);
		BFX_FREE(node);
		node = map;
	}
	assert(map == NULL);
}

static void KeyNodeListDestroy(KeyNode* list)
{
	key_node* node = list;
	while (node != NULL)
	{
		sglib_key_node_delete(&list, node);
		KeyNodeDestroy(node);
		node = list;
	}
	assert(list == NULL);
}

static KeyNode* KeyNodeCreate(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const PeeridKeyMapValue* peeridKeyMapValue)
{
	KeyNode* keyNode = BFX_MALLOC_OBJ(KeyNode);
	memset(keyNode, 0, sizeof(*keyNode));
	memcpy(keyNode->keyInfo.aesKey, aesKey, BDT_AES_KEY_LENGTH);
	keyNode->keyInfo.peerid = &peeridKeyMapValue->peerid;
	keyNode->keyInfo.lastAccessTime = BDT_MICRO_SECONDS_2_SECONDS(BfxTimeGetNow(FALSE));
	return keyNode;
}

static void KeyNodeDestroy(KeyNode* node)
{
	for (int i = 0; i < BFX_ARRAYSIZE(node->mixHashNodes); i++)
	{
		if (node->mixHashNodes[i] != NULL)
		{
			BFX_FREE(node->mixHashNodes[i]);
		}
	}
	BFX_FREE(node);
}

bool BdtHistory_KeyManagerCalcKeyHash(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	uint8_t hash[BDT_MIX_HASH_LENGTH])
{
	return CalcKeyHashImpl(aesKey, NULL, 0, hash);
}

static bool CalcKeyHashImpl(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const uint8_t* appendBuffer,
	uint32_t appendSize,
	uint8_t hash[BDT_MIX_HASH_LENGTH])
{
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	int ret = mbedtls_sha256_starts_ret(&ctx, 0);
	ret = ret || mbedtls_sha256_update_ret(&ctx, aesKey, BDT_AES_KEY_LENGTH);
	if (appendSize > 0)
	{
		ret = ret || mbedtls_sha256_update_ret(&ctx, appendBuffer, appendSize);
	}
	uint8_t sha256[32];
	ret = ret || mbedtls_sha256_finish_ret(&ctx, sha256);
	if (ret == 0)
	{
		memcpy(hash, sha256, BDT_MIX_HASH_LENGTH);
	}
	return !ret;
}

static uint32_t KeyManagerAddNewKeyNoLock(KeyManager* keyManager,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const BdtPeerid* peerid,
	PeeridKeyMap* existPeerKeyNode,
	KeyNode** outAddKeyNode)
{
	*outAddKeyNode = KeyManagerFindAesKeyNoLock(keyManager, aesKey, peerid, &existPeerKeyNode);
	if (*outAddKeyNode != NULL)
	{
		return BFX_RESULT_ALREADY_EXISTS;
	}

	PeeridKeyMap* peeridMapNode = existPeerKeyNode;
	if (existPeerKeyNode == NULL)
	{
		peeridMapNode = BFX_MALLOC_OBJ(PeeridKeyMap);
		memset(peeridMapNode, 0, sizeof(*peeridMapNode));
		peeridMapNode->value.peerid = *peerid;
	}

	KeyNode* keyNode = KeyNodeCreate(aesKey, &peeridMapNode->value);
	keyNode->keyInfo.expireTime = keyNode->keyInfo.lastAccessTime + (int64_t)keyManager->config->activeTime;
	MixHashKeyMap* newHashNodes[BDT_MIX_HASH_4_KEY_LIMIT];
	MixHashKeyMap* deadHashNodes[BDT_MIX_HASH_4_KEY_LIMIT];
	KeyManagerRehashAesKey(keyNode, newHashNodes, deadHashNodes);
	assert(keyNode != NULL);

	sglib_key_node_add_before(&keyManager->keyList.header, keyNode);
	if (keyManager->keyList.tail == NULL)
	{
		keyManager->keyList.firstHashMinutes = BDT_MINUTES_NOW;
		keyManager->keyList.tail = keyNode;
	}
	sglib_key4peer_node_add(&peeridMapNode->value.keyNodeList, keyNode);
	if (existPeerKeyNode == NULL)
	{
		peeridMapNode->color = RBTREE_COLOR_INVALID;
		sglib_peerid_key_map_add(&keyManager->peeridKeyMap, peeridMapNode);
		assert(peeridMapNode->color != RBTREE_COLOR_INVALID);
	}
	uint32_t i = 0;
	for (i = 0; i < keyNode->hashCount; i++)
	{
		MixHashKeyMap* hashKeyNode = keyNode->mixHashNodes[i];
		hashKeyNode->color = RBTREE_COLOR_INVALID;
		sglib_mix_hash_key_map_add(&keyManager->mixHashKeyMap, hashKeyNode);
		assert(hashKeyNode->color != RBTREE_COLOR_INVALID);
	}

	*outAddKeyNode = keyNode;
	return BFX_RESULT_SUCCESS;
}

static void KeyManagerGenerateAesKey(uint8_t aesKey[BDT_AES_KEY_LENGTH])
{
	BfxRandomBytes(aesKey, BDT_AES_KEY_LENGTH);
}

static uint32_t KeyManagerRehashAesKey(KeyNode* keyNode,
	MixHashKeyMap* newHashNodes[BDT_MIX_HASH_4_KEY_LIMIT],
	MixHashKeyMap* deadHashNodes[BDT_MIX_HASH_4_KEY_LIMIT])
{
	const uint8_t* aesKey = keyNode->keyInfo.aesKey;

	uint32_t calcCount = 0;
	uint32_t minutes = BDT_MINUTES_NOW;
	const uint32_t minMinutes = minutes - MIX_HASH_REDUNDANCY_MINUTES;
	const uint32_t maxMinutes = minutes + MIX_HASH_REDUNDANCY_MINUTES;
	if (keyNode->hashCount == 0)
	{
		// 计算KEY的sha256
		keyNode->hashCount = BDT_MIX_HASH_4_KEY_LIMIT;
		MixHashKeyMap* newOriginalHashNode = BFX_MALLOC_OBJ(MixHashKeyMap);
		memset(newOriginalHashNode, 0, sizeof(MixHashKeyMap));
		newOriginalHashNode->value.keyNode = keyNode;
		MixHashInfo* originalHash = &newOriginalHashNode->value.mixHashInfo;
		originalHash->minutes = 0;
		if (!CalcKeyHashImpl(aesKey, NULL, 0, originalHash->hash))
		{
			BFX_FREE(newOriginalHashNode);
			return 0;
		}
		keyNode->mixHashs4Update.nextUpdateHash = 0;
		keyNode->mixHashs4Update.originalHashNode = newOriginalHashNode;
		deadHashNodes[calcCount] = NULL;
		newHashNodes[calcCount++] = newOriginalHashNode;
		minutes -= MIX_HASH_REDUNDANCY_MINUTES;
	}
	else
	{
		// 搜索时间相关的最早HASH，以判定是否有超时HASH
		const uint32_t newestHashIndex = ((keyNode->mixHashs4Update.nextUpdateHash - 1) % MIX_HASH_WITH_APPEND_COUNT);
		MixHashKeyMap* newestMixHashNode = keyNode->mixHashs4Update.mixHashNodesWithAppend[newestHashIndex];
		uint32_t nextMinutes = newestMixHashNode->value.mixHashInfo.minutes + 1;
		if (nextMinutes < minMinutes)
		{
			minutes -= MIX_HASH_REDUNDANCY_MINUTES;
		}
		else if (nextMinutes > maxMinutes)
		{
			return 0;
		}
	}

	uint32_t i = 0;
	MixHashInfo* hashInfo = NULL;
	MixHashKeyMap* hashNode = NULL;

	// 计算时间相关HASH值
	for (; i < MIX_HASH_WITH_APPEND_COUNT; i++)
	{
		hashNode = keyNode->mixHashs4Update.mixHashNodesWithAppend[keyNode->mixHashs4Update.nextUpdateHash];
		if (hashNode)
		{
			hashInfo = &hashNode->value.mixHashInfo;
			if (hashInfo->minutes >= minMinutes)
			{
				break;
			}
		}

		deadHashNodes[calcCount] = hashNode;

		hashNode = BFX_MALLOC_OBJ(MixHashKeyMap);
		memset(hashNode, 0, sizeof(MixHashKeyMap));
		hashNode->value.keyNode = keyNode;
		hashInfo = &hashNode->value.mixHashInfo;
		hashInfo->minutes = minutes++;
		if (!CalcKeyHashImpl(aesKey, hashInfo->append, sizeof(hashInfo->append), hashInfo->hash))
		{
			assert(false);
			BFX_FREE(hashNode);
			break;
		}
		keyNode->mixHashs4Update.mixHashNodesWithAppend[keyNode->mixHashs4Update.nextUpdateHash] = hashNode;
		keyNode->mixHashs4Update.nextUpdateHash++;
		keyNode->mixHashs4Update.nextUpdateHash %= MIX_HASH_WITH_APPEND_COUNT;
		newHashNodes[calcCount++] = hashNode;
	}
	return calcCount;
}

// 清理指定PEER的指定超时KEY，如果没超时不处理；
// peeridKeyNode参数是keyNode指定KEY对应PEER在MAP<peerid, keyNode>中的节点，可以置NULL
// keyNode置空时，清理peeridKeyNode指定PEER对应的所有超时KEY，传入有效值就只清理唯一指定的超时KEY
// keyNode和peeridKeyNode最少指定其中之一
static PeeridKeyMap* KeyManagerClearExpireKeyNoLock(KeyManager* keyManager,
	PeeridKeyMap* peeridKeyNode/* = NULL*/,
	KeyNode* keyNode /* = NULL*/)
{
	if (keyNode == NULL && peeridKeyNode == NULL)
	{
		assert(FALSE);
		return NULL;
	}

	// 找到要删除KEY对应PEER的MAP-NODE
	if (peeridKeyNode == NULL)
	{
		peeridKeyNode = KeyManagerFindKeyByPeeridNoLock(keyManager, keyNode->keyInfo.peerid);
		assert(peeridKeyNode != NULL);
	}
#ifdef BFX_DEBUG
	else
	{
		if (keyNode != NULL)
		{
			assert(peeridKeyNode == KeyManagerFindKeyByPeeridNoLock(keyManager, keyNode->keyInfo.peerid));
		}
	}
#endif

	const int64_t now = BfxTimeGetNow(FALSE);
	const int64_t nowSeconds = BDT_MICRO_SECONDS_2_SECONDS(now);

	// 构造要清理KEY列表
	KeyNode** nextKeyNodePtrPtr = NULL;
	if (keyNode == NULL)
	{
		nextKeyNodePtrPtr = &peeridKeyNode->value.keyNodeList;
		keyNode = *nextKeyNodePtrPtr;
	}
	else
	{
		if (keyNode->keyInfo.expireTime > nowSeconds)
		{
			// 没超时
			return peeridKeyNode;
		}

		sglib_key4peer_node_delete(&peeridKeyNode->value.keyNodeList, keyNode);
		keyNode->nextAesKey = NULL; // 保证下面循环删除key的列表中只有一个key
		nextKeyNodePtrPtr = &keyNode;
	}

	const uint32_t nowMinutes = BDT_MICRO_SECONDS_2_MINUTES(now);
	const uint32_t minHashMinutes = nowMinutes - MIX_HASH_REDUNDANCY_MINUTES;

	for (; keyNode != NULL; keyNode = *nextKeyNodePtrPtr)
	{
		if (keyNode->keyInfo.expireTime > nowSeconds)
		{
			// KEY没超时，处理下一个
			nextKeyNodePtrPtr = &keyNode->nextAesKey;
			continue;
		}

		// KEY超时
		// 从该PEER的KEY列表中断开该节点
		*nextKeyNodePtrPtr = keyNode->nextAesKey;

		KeyManagerDeleteKey(keyManager, keyNode, minHashMinutes);
	}

	if (peeridKeyNode->value.keyNodeList == NULL)
	{
		KeyManagerDeletePeer(keyManager, peeridKeyNode);
		return NULL;
	}
	return peeridKeyNode;
}

static void KeyManagerRemoveKeyNodeFromList(KeyManager* keyManager,
	KeyNode* keyNode,
	uint32_t minHashMinutes)
{
	if (keyNode->mixHashs4Update.mixHashNodesWithAppend[keyNode->mixHashs4Update.nextUpdateHash]->value.mixHashInfo.minutes < minHashMinutes)
	{
		// HASH超时，先从rehash列表里搜索
		KeyNode* foundKeyNode = NULL;
		sglib_key_node_delete_if_member(&keyManager->rehashKeyList, keyNode, &foundKeyNode);
		if (foundKeyNode == NULL)
		{
			sglib_key_node_delete(&keyManager->keyList.header, keyNode);
			if (keyManager->keyList.header == NULL)
			{
				keyManager->keyList.tail = NULL;
			}
		}
	}
	else
	{
		// HASH没超时，先从keylist里搜索
		KeyNode* foundKeyNode = NULL;
		sglib_key_node_delete_if_member(&keyManager->keyList.header, keyNode, &foundKeyNode);
		if (foundKeyNode != NULL)
		{
			if (keyManager->keyList.header == NULL)
			{
				keyManager->keyList.tail = NULL;
			}
		}
		else
		{
			sglib_key_node_delete(&keyManager->rehashKeyList, keyNode);
		}
	}
}

static void KeyManagerDeleteMixHash(KeyManager* keyManager,
	MixHashKeyMap* mixHashNode,
	bool freeNode)
{
	if (mixHashNode == NULL)
	{
		return;
	}
	assert(mixHashNode->color != RBTREE_COLOR_INVALID);
	if (mixHashNode->color != RBTREE_COLOR_INVALID)
	{
		sglib_mix_hash_key_map_delete(&keyManager->mixHashKeyMap, mixHashNode);
	}

	if (freeNode)
	{
		BFX_FREE(mixHashNode);
	}
}

static void KeyManagerDeleteKey(KeyManager* keyManager,
	KeyNode* keyNode,
	uint32_t minHashMinutes)
{
	if (keyNode == NULL)
	{
		return;
	}
	for (int i = 0; i < BDT_MIX_HASH_4_KEY_LIMIT; i++)
	{
		KeyManagerDeleteMixHash(keyManager, keyNode->mixHashNodes[i], false);
	}

	KeyManagerRemoveKeyNodeFromList(keyManager, keyNode, minHashMinutes);
	KeyNodeDestroy(keyNode);
}

static void KeyManagerDeletePeer(KeyManager* keyManager,
	PeeridKeyMap* peeridKeyMapNode)
{
	if (peeridKeyMapNode == NULL)
	{
		return;
	}
	assert(peeridKeyMapNode->color != RBTREE_COLOR_INVALID);
	if (peeridKeyMapNode->color != RBTREE_COLOR_INVALID)
	{
		sglib_peerid_key_map_delete(&keyManager->peeridKeyMap, peeridKeyMapNode);
	}

	KeyNode* keyNode = peeridKeyMapNode->value.keyNodeList;
	if (keyNode != NULL)
	{
		const uint32_t nowMinutes = BDT_MICRO_SECONDS_2_MINUTES(BfxTimeGetNow(FALSE));
		const uint32_t minHashMinutes = nowMinutes - MIX_HASH_REDUNDANCY_MINUTES;

		for (KeyNode* nextKeyNode = NULL; keyNode != NULL; keyNode = nextKeyNode)
		{
			nextKeyNode = keyNode->nextAesKey;
			KeyManagerDeleteKey(keyManager, keyNode, minHashMinutes);
		}
	}
	BFX_FREE(peeridKeyMapNode);
}

// 历史数据加载完成
void BdtHistory_KeyManagerOnLoadDone(
	KeyManager* keyManager,
	BdtStorage* storage,
	uint32_t errorCode,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount
)
{
	if (errorCode != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("load aeskey failed, errorCode:%d.", errorCode);
		return;
	}

	BLOG_INFO("load aeskey success, count:%d", (int)keyCount);
	BLOG_CHECK(keyCount >= 0);
	BLOG_CHECK(keyManager != NULL);
	BLOG_CHECK(storage != NULL);

	KeyManagerMergeOuterKeys(
		keyManager,
		storage,
		aesKeys,
		keyCount
	);
}


// 把外部历史记录列表合并；
static void KeyManagerMergeOuterKeysNoLock(
	KeyManager* keyManager,
	const BdtHistory_KeyInfo* outerKeys[],
	size_t keyCount
)
{
	for (size_t i = 0; i < keyCount; i++)
	{
		const BdtHistory_KeyInfo* outerKeyInfo = outerKeys[i];
		KeyManagerAddOuterKey(keyManager, outerKeyInfo);
	}
}

static void KeyManagerMergeOuterKeys(
	KeyManager* keyManager,
	BdtStorage* storage,
	const BdtHistory_KeyInfo* outerKeys[],
	size_t keyCount
)
{

	BLOG_CHECK(keyManager != NULL);
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(outerKeys != NULL);

	BfxRwLockWLock(&keyManager->lock);

	keyManager->storage = storage;
	if (keyCount > 0)
	{
		KeyManagerMergeOuterKeysNoLock(
			keyManager,
			outerKeys,
			keyCount
		);
	}

	BfxRwLockWUnlock(&keyManager->lock);
}

// 加入外部历史记录
static BdtHistory_KeyInfo* KeyManagerAddOuterKey(
	BdtHistory_KeyManager* keyManager,
	const BdtHistory_KeyInfo* outerKeyInfo
)
{
	BLOG_CHECK(keyManager != NULL);
	BLOG_CHECK(outerKeyInfo != NULL);

	// <TODO>加载的key可能比较多，一次性加入，计算hash可能很耗时

	BdtHistory_KeyInfo* keyInfo = NULL;
	KeyNode* keyNode = NULL;
	uint32_t result = KeyManagerAddNewKeyNoLock(keyManager, outerKeyInfo->aesKey, outerKeyInfo->peerid, NULL, &keyNode);
	BLOG_CHECK(keyNode != NULL);
	keyInfo = &keyNode->keyInfo;
	if (result == BFX_RESULT_ALREADY_EXISTS)
	{
		return keyInfo;
	}

	keyInfo->flags.isConfirmed = outerKeyInfo->flags.isConfirmed;
	keyInfo->flags.isStorage = 1;
	keyInfo->expireTime = outerKeyInfo->expireTime;
	keyInfo->lastAccessTime = outerKeyInfo->lastAccessTime;

	return keyInfo;
}

static bool KeyManagerUpdateKey(
	BdtHistory_KeyInfo* keyInfo,
	bool confirmed,
	int64_t expireTime)
{
	bool changed = false;
	if (confirmed && !keyInfo->flags.isConfirmed)
	{
		changed = true;
		keyInfo->flags.isConfirmed = 1;
	}
	
	if (expireTime &&
		(expireTime < keyInfo->expireTime || expireTime - keyInfo->expireTime > 60))
	{
		// 超过1分钟才更新
		changed = true;
		keyInfo->expireTime = expireTime;
	}
	return changed;
}

static bool KeyManagerPrepareForStorage(KeyManager* keyManager, BdtHistory_KeyInfo* keyInfo, bool* canStorage)
{
	if (keyManager->storage != NULL)
	{
		*canStorage = true;
		if (!keyInfo->flags.isStorage)
		{
			keyInfo->flags.isStorage = 1;
			*canStorage = true;
			return true;
		}
	}
	else
	{
		*canStorage = false;
	}

	return false;
}

static void KeyManagerStorageUpdateAesKey(
	KeyManager* keyManager,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
)
{
	uint32_t result = BdtHistory_StorageUpdateAesKey(keyManager->storage,
		peerid,
		aesKey,
		expireTime,
		flags,
		lastAccessTime,
		isNew);

	if (result != BFX_RESULT_PENDING &&
		isNew &&
		result != BFX_RESULT_SUCCESS &&
		result != BFX_RESULT_ALREADY_OPERATION)
	{
		BfxRwLockRLock(&keyManager->lock);

		PeeridKeyMap* peeridMapNode = NULL;
		KeyNode* keyNode = KeyManagerFindAesKeyNoLock(keyManager, aesKey, peerid, &peeridMapNode);
		if (keyNode != NULL)
		{
			keyNode->keyInfo.flags.isStorage = 0;
		}

		BfxRwLockRUnlock(&keyManager->lock);
	}
}