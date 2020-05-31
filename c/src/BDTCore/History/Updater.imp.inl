
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in Module.c of history module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include "./Updater.h"
#include <SGLib/SGLib.h>
#include "../Global/PeerInfo.h"

typedef struct PeerInfoNode
{
	volatile int32_t ref;
	struct PeerInfoNode* nextPeer;
	struct PeerInfoNode* previousPeer;
	struct PeerInfoNode* nextSn;
	struct PeerInfoNode* previousSn;
	union
	{
		BdtHistory_PeerInfo peerHistory;
		BdtHistory_SuperNodeInfo snHistory;
	};

	// <TODO> 每个peer都会有一个peerid-map的节点，内存分一起吧？不用两次malloc
	// PeeridMapNode mapNode;
} PeerInfoNode;

#define BDT_GET_PEER_NODE_FROM_HISTORY_INFO(historyInfo) (PeerInfoNode*)((uint8_t*)(historyInfo) - (uint8_t*)&(((PeerInfoNode*)0)->peerHistory))

typedef struct PeeridMapNode
{
	struct PeeridMapNode* left;
	struct PeeridMapNode* right;
	int color;

	const BdtPeerid* peerid; // key
	PeerInfoNode* peerHistoryNode; // value
} PeeridMapNode;

static int PeerUpdaterPeerCompareor(PeerInfoNode* left, PeerInfoNode* right)
{
	assert(false);
	return 0;
}

typedef PeerInfoNode peer_history;
SGLIB_DEFINE_DL_LIST_PROTOTYPES(peer_history, PeerUpdaterPeerCompareor, previousPeer, nextPeer)
SGLIB_DEFINE_DL_LIST_FUNCTIONS(peer_history, PeerUpdaterPeerCompareor, previousPeer, nextPeer)

typedef PeerInfoNode super_node_history;
SGLIB_DEFINE_DL_LIST_PROTOTYPES(super_node_history, PeerUpdaterPeerCompareor, previousSn, nextSn)
SGLIB_DEFINE_DL_LIST_FUNCTIONS(super_node_history, PeerUpdaterPeerCompareor, previousSn, nextSn)

typedef PeeridMapNode peerid_history_map;
static int PeerUpdaterPeeridHistoryMapCompareor(const PeeridMapNode* left,
	const PeeridMapNode* right)
{
	return memcmp(left->peerid, right->peerid, sizeof(BdtPeerid));
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(peerid_history_map, left, right, color, PeerUpdaterPeeridHistoryMapCompareor)
SGLIB_DEFINE_RBTREE_FUNCTIONS(peerid_history_map, left, right, color, PeerUpdaterPeeridHistoryMapCompareor)

typedef struct SpeedStatistic
{
	int64_t startTime;
	uint32_t byteSize;
	uint32_t speed;
} SpeedStatistic;

typedef struct BdtHistory_PeerUpdaterTransferTask
{
	BdtHistory_PeerUpdater* updater;
	PeerInfoNode* peerHistoryNode;
	BdtHistory_PeerConnectInfo* connectInfo;
	bool isNewConnect;

	SpeedStatistic sendStat;
	SpeedStatistic recvStat;

	int64_t reqTime;
	int64_t respTime;
	uint32_t rto;
} BdtHistory_PeerUpdaterTransferTask, PeerUpdaterTransferTask;

typedef struct BdtHistory_PeerUpdater
{
	BdtStorage* storage;
	const BdtPeerHistoryConfig* config;
	BdtHistory_PeerInfo* localHistory;

	BfxRwLock lock; // for peeridMap/peerList/peerListTail/superNodeList
	PeeridMapNode* peeridMap;
	PeerInfoNode* peerList;
	PeerInfoNode* peerListTail;
	PeerInfoNode* superNodeList;

	uint32_t peerCount;
} BdtHistory_PeerUpdater, PeerUpdater;

typedef struct LoadPeerHistoryByPeerIdUserData
{
    PeerUpdater* updater;
    BdtPeerid peerid;
    BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK callback;
    BfxUserData userData;
} LoadPeerHistoryByPeerIdUserData;

static void HistoryPeerInfoInit(
	const BdtPeerid* peerid,
	const BdtPeerInfo* peerInfo, 
	BdtHistory_PeerInfo* peerHistory
);
static void HistoryPeerInfoUpdateEndpoints(
	const BdtEndpoint* endpoints,
	size_t count,
	BdtHistory_PeerInfo* peerHistory
);
static PeerInfoNode* HistoryPeerInfoUpdateFull(
	PeerUpdater* updater,
	const BdtPeerid* peerid,
	const BdtPeerInfo* peerInfo,
	bool* isNew,
    bool* isNewConstInfo
);
static void HistoryPeerUpdaterAddHistory(
	PeerUpdater* updater,
	PeerInfoNode* node
);
static void HistoryPeerDestroyPeeridMap(PeeridMapNode* map);
static void HistoryPeerDestroyHistoryList(PeerUpdater* updater);
static BdtHistory_SuperNodeInfo* HistoryPeerInfoAddSuperNode(
	BdtHistory_PeerInfo* peerHistory,
	BdtHistory_SuperNodeInfo* snPeerHistory
);
static BdtHistory_PeerConnectInfo* HistoryPeerInfoAddConnectInfo(
	BdtHistory_PeerInfo* peerHistory,
	const BdtEndpoint* endpoint,
	int64_t foundTime
);
static BdtHistory_PeerConnectInfo* HistoryPeerFindLocalEndpoint(
	PeerUpdater* updater,
	const BdtEndpoint* localEndpoint);
static BdtHistory_PeerConnectInfo* HistoryPeerFindOrAddConnectInfo(
	BdtHistory_PeerInfo* peerHistory,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint,
	bool* isNew
);
static uint32_t HistoryPeerStatisticAverage(
	uint32_t oldValue,
	uint32_t newValue
);
static void SpeedStatisticAddBytes(
	SpeedStatistic* stat,
	uint32_t bytes
);
static void SpeedStatisticPause(SpeedStatistic* stat);

static void HistoryPeerUpdaterMergeOuterHistory(
	PeerUpdater* updater,
	BdtStorage* storage,
	const BdtHistory_PeerInfo* outerHistorys[],
	size_t historyCount
);

static bool HistoryEndpointIsEqual(const BdtEndpoint* ep1, const BdtEndpoint* ep2);

// 加入外部历史记录
static BdtHistory_PeerInfo* PeerUpdaterAddOuterHistory(
	PeerUpdater* updater,
	const BdtHistory_PeerInfo* outerHistory,
    const BdtPeerid* addSNPeerids,
    size_t addSNCount
);
static void HistoryUpdateSuperNodeToStorage(
	BdtStorage* storage,
	BdtHistory_PeerInfo* peerInfo,
	BdtHistory_SuperNodeInfo* snInfo,
	bool isNewSnForPeer);
static void HistoryUpdateConnectInfoToStorage(BdtStorage* storage,
	BdtHistory_PeerInfo* peerHistory,
    bool isNewConstInfo,
	BdtHistory_PeerConnectInfo* connectInfo,
	bool isNewConnect);
static bool HistoryEnsurePeerInfoToStorage(
	BdtStorage* storage,
	BdtHistory_PeerInfo* peerInfo,
    bool isNewConstInfo);

static int32_t PeerInfoNodeAddRef(PeerInfoNode* node);
static int32_t PeerInfoNodeRelease(PeerInfoNode* node,
	BdtHistory_PeerInfo* localHistory);
static void HistoryLimitPeerCount(PeerUpdater* updater);
static bool HistoryUpdateConnectInfo(
    BdtHistory_PeerConnectInfo* connectInfo,
	int64_t lastConnectTime,
	uint32_t rto,
	uint32_t sendSpeed,
	uint32_t recvSpeed,
	uint32_t flags);
static bool HistoryUpdateSuperNodeInfo(
    BdtHistory_SuperNodeInfo* snInfo,
	int64_t lastResponseTime);
static void HistoryMovePeerHistoryToHeader(
    PeerUpdater* updater,
    PeerInfoNode* historyNode);

static void HistoryLoadPeerHistoryByPeerid(
    uint32_t errorCode,
    const BdtHistory_PeerInfo* history,
    const BdtPeerid snPeerids[],
    size_t snCount,
    void* userData
);

static void PeerUpdaterInit(
	PeerUpdater* updater,
	const BdtPeerInfo* localPeerInfo,
	bool isSuperNode,
	const BdtPeerHistoryConfig* config)
{
	assert(localPeerInfo != NULL);
	memset(updater, 0, sizeof(*updater));

	bool isNew = true;
    bool isNewConstInfo = false;
	PeerInfoNode* localPeerHistoryNode = HistoryPeerInfoUpdateFull(
		updater, 
		BdtPeerInfoGetPeerid(localPeerInfo),
		localPeerInfo, 
		&isNew,
        &isNewConstInfo);
	localPeerHistoryNode->peerHistory.flags.isLocal = 1;
	if (isSuperNode)
	{
		localPeerHistoryNode->peerHistory.flags.isSn = 1;
	}
	assert(isNew && !isNewConstInfo);

	size_t epCount = 0;
	const BdtEndpoint* epArray = BdtPeerInfoListEndpoint(localPeerInfo, &epCount);

	updater->localHistory = &localPeerHistoryNode->peerHistory;
	updater->storage = NULL;
	updater->config = config;

	BfxRwLockInit(&updater->lock);
}

static void PeerUpdaterUninit(PeerUpdater* updater)
{
	updater->storage = NULL;

	HistoryPeerDestroyPeeridMap(updater->peeridMap);
	updater->peeridMap = NULL;
	HistoryPeerDestroyHistoryList(updater);
	updater->peerList = NULL;
	updater->peerListTail = NULL;
	updater->superNodeList = NULL; // SN列表是peer列表的子集，不用专门删除
	updater->localHistory = NULL;
	BfxRwLockDestroy(&updater->lock);
}

PeerUpdater* BdtHistory_PeerUpdaterCreate(
	const BdtPeerInfo* localPeerInfo,
	bool isSuperNode,
	const BdtPeerHistoryConfig* config)
{
	PeerUpdater* updater = BFX_MALLOC_OBJ(PeerUpdater);
	PeerUpdaterInit(updater, localPeerInfo, isSuperNode, config);
	return updater;
}

void BdtHistory_PeerUpdaterDestroy(PeerUpdater* updater)
{
	PeerUpdaterUninit(updater);
	BfxFree(updater);
}

// 查询指定peerid节点的历史信息记录
uint32_t BdtHistory_PeerUpdaterFindByPeerid(PeerUpdater* updater,
	const BdtPeerid* peerid,
	bool searchInStorage,
	BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK callback,
	const BfxUserData* userData)
{
    BLOG_CHECK(updater != NULL);
    BLOG_CHECK(peerid != NULL);
    BLOG_CHECK(callback != NULL);

	PeeridMapNode targetNode = { .peerid = peerid };
	BfxRwLockRLock(&updater->lock);
	BdtHistory_PeerInfo* localHistory = updater->localHistory;
	PeeridMapNode* foundNode = sglib_peerid_history_map_find_member(updater->peeridMap, &targetNode);
	PeerInfoNode* peerHistoryNode = NULL;
	if (foundNode != NULL)
	{
		peerHistoryNode = foundNode->peerHistoryNode;
        HistoryMovePeerHistoryToHeader(updater, peerHistoryNode);
		PeerInfoNodeAddRef(peerHistoryNode);
	}
	BfxRwLockRUnlock(&updater->lock);
	if (peerHistoryNode != NULL)
	{
		callback(&peerHistoryNode->peerHistory, userData ? userData->userData : NULL);
		PeerInfoNodeRelease(peerHistoryNode, localHistory);
		return BFX_RESULT_SUCCESS;
	}

	if (searchInStorage)
	{
        LoadPeerHistoryByPeerIdUserData* userDataImpl = BFX_MALLOC_OBJ(LoadPeerHistoryByPeerIdUserData);
        userDataImpl->peerid = *peerid;
        userDataImpl->updater = updater;
        userDataImpl->callback = callback;
        userDataImpl->userData = *userData;
        if (userData == NULL)
        {
            userDataImpl->userData.lpfnAddRefUserData = NULL;
            userDataImpl->userData.lpfnReleaseUserData = NULL;
            userDataImpl->userData.userData = NULL;
        }
        else
        {
            userDataImpl->userData = *userData;
            if (userData->lpfnAddRefUserData != NULL && userData->userData != NULL)
            {
                userData->lpfnAddRefUserData(userData->userData);
            }
        }

        BfxUserData userDataParam = { .userData = userDataImpl,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };

        BdtHistory_StorageLoadPeerHistoryByPeerid(updater->storage,
            peerid,
            BfxTimeGetNow(false) - updater->config->activeTime,
            HistoryLoadPeerHistoryByPeerid,
            &userDataParam);
		return BFX_RESULT_PENDING;
	}
	return BFX_RESULT_NOT_FOUND;
}

// 从SN响应ping
uint32_t BdtHistory_PeerUpdaterOnSuperNodePingResponse(
	PeerUpdater* updater,
	const BdtPeerInfo* snPeerInfo)
{
	bool isNewSn = false;
    bool isNewConstInfo = false;
	bool isNewSnForPeer = false;
	bool isChanged = false;
	size_t snCount = 0;

	BfxRwLockWLock(&updater->lock);

	BdtHistory_PeerInfo* localPeerHistory = updater->localHistory;
	PeerInfoNode* localPeerHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(localPeerHistory);
	snCount = localPeerHistory->snVector.size;
	PeerInfoNode* snPeerHistoryNode = HistoryPeerInfoUpdateFull(
		updater, 
		BdtPeerInfoGetPeerid(snPeerInfo),
		snPeerInfo, 
		&isNewSn,
        &isNewConstInfo);
    assert(!isNewConstInfo);
	isChanged = HistoryUpdateSuperNodeInfo(&snPeerHistoryNode->snHistory, BDT_MS_NOW);
	BdtHistory_SuperNodeInfo* snInfo = HistoryPeerInfoAddSuperNode(localPeerHistory, &snPeerHistoryNode->snHistory);
	isNewSnForPeer = localPeerHistory->snVector.size > snCount;

	if (isChanged || isNewSnForPeer)
	{
		PeerInfoNodeAddRef(localPeerHistoryNode);
		PeerInfoNodeAddRef(snPeerHistoryNode);
	}

	HistoryLimitPeerCount(updater);

	BfxRwLockWUnlock(&updater->lock);

	if (isChanged || isNewSnForPeer)
	{
		HistoryUpdateSuperNodeToStorage(updater->storage, localPeerHistory, snInfo, isNewSnForPeer);
		PeerInfoNodeRelease(localPeerHistoryNode, NULL);
		PeerInfoNodeRelease(snPeerHistoryNode, localPeerHistory);
	}

	return BFX_RESULT_SUCCESS;
}

// 从SN查到PEER信息时调用
uint32_t BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(
	PeerUpdater* updater,
	const BdtPeerInfo* peerInfo,
	const BdtPeerInfo* snPeerInfo)
{
	bool isNewPeer = false;
    bool isNewConstInfo = false;
	bool isNewSn = false;
	bool isNewSnForPeer = false;
	bool isChanged = false;
	size_t snCount = 0;

	BfxRwLockWLock(&updater->lock);

	PeerInfoNode* peerHistoryNode = NULL;
	BdtHistory_PeerInfo* localHistory = updater->localHistory;

	if (memcmp(BdtPeerInfoGetPeerid(peerInfo), &updater->localHistory->peerid, sizeof(BdtPeerid)) != 0)
	{
		peerHistoryNode = HistoryPeerInfoUpdateFull(
			updater, 
			BdtPeerInfoGetPeerid(peerInfo), 
			peerInfo, 
			&isNewPeer,
            &isNewConstInfo);
        assert(!isNewConstInfo);
	}
	else
	{
		peerHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(localHistory);
	}

	snCount = peerHistoryNode->peerHistory.snVector.size;
	PeerInfoNode* snPeerHistoryNode = HistoryPeerInfoUpdateFull(
		updater, 
		BdtPeerInfoGetPeerid(snPeerInfo),
		snPeerInfo, 
		&isNewSn,
        &isNewConstInfo);
    assert(!isNewConstInfo);
	isChanged = HistoryUpdateSuperNodeInfo(&snPeerHistoryNode->snHistory, BDT_MS_NOW);
	BdtHistory_SuperNodeInfo* snInfo = HistoryPeerInfoAddSuperNode(&peerHistoryNode->peerHistory, &snPeerHistoryNode->snHistory);
	isNewSnForPeer = peerHistoryNode->peerHistory.snVector.size > snCount;

	HistoryLimitPeerCount(updater);

	if (isChanged || isNewSnForPeer)
	{
		PeerInfoNodeAddRef(peerHistoryNode);
		PeerInfoNodeAddRef(snPeerHistoryNode);
	}

	BfxRwLockWUnlock(&updater->lock);

	if (isChanged || isNewSnForPeer)
	{
		HistoryUpdateSuperNodeToStorage(updater->storage, &peerHistoryNode->peerHistory, snInfo, isNewSnForPeer);
		PeerInfoNodeRelease(peerHistoryNode, localHistory);
		PeerInfoNodeRelease(snPeerHistoryNode, localHistory);
	}

	return BFX_RESULT_SUCCESS;
}

// 对端响应了本地发出的请求包，无法确定rto时置0
uint32_t BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
	PeerUpdater* updater, 
	const BdtPeerid* remotePeerid, 
	const BdtPeerInfo* remotePeerInfo,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint,
	BdtHistory_PeerConnectType connectType,
	uint32_t rto)
{
	if (memcmp(remotePeerid, &updater->localHistory->peerid, sizeof(BdtPeerid)) == 0)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	BdtHistory_PeerConnectInfo* localConnectHistory = HistoryPeerFindLocalEndpoint(updater, localEndpoint);
	if (localConnectHistory == NULL)
	{
		return BFX_RESULT_NOT_FOUND;
	}

	bool isNewPeer = false;
    bool isNewConstInfo = false;
	bool isNewConnect = false;
	bool isChanged = false;

	BfxRwLockWLock(&updater->lock);
	
	BdtHistory_PeerInfo* localHistory = updater->localHistory;

	PeerInfoNode* peerHistoryNode = HistoryPeerInfoUpdateFull(updater, remotePeerid, remotePeerInfo, &isNewPeer, &isNewConstInfo);
	BdtHistory_PeerInfo* peerHistory = &peerHistoryNode->peerHistory;
	BdtHistory_PeerConnectInfo* connectHistory = HistoryPeerFindOrAddConnectInfo(
		peerHistory,
		remoteEndpoint,
		&localConnectHistory->endpoint,
		&isNewConnect);

	BdtHistory_PeerConnectInfoFlags flags = { .u32 = connectHistory->flags.u32 };
	if (connectType == BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT)
	{
		flags.directTcp = 1;
	}
	else if (connectType == BDT_HISTORY_PEER_CONNECT_TYPE_TCP_REVERSE)
	{
		flags.reverseTcp = 1;
	}

	isChanged = HistoryUpdateConnectInfo(connectHistory,
		BDT_MS_NOW,
		rto,
		0,
		0,
		flags.u32);

	if (isNewConnect && connectHistory->localEndpoint != NULL)
	{
		// 有localEndpoint指向updater->localHistory，所以，要增加一个引用计数保持localHistory的有效性
		PeerInfoNode* localHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(localHistory);
		PeerInfoNodeAddRef(localHistoryNode);
	}

	HistoryLimitPeerCount(updater);

	if (isNewPeer || isNewConnect || isChanged)
	{
		PeerInfoNodeAddRef(peerHistoryNode);
	}

	BfxRwLockWUnlock(&updater->lock);

	if (isNewPeer || isNewConnect || isChanged)
	{
		HistoryUpdateConnectInfoToStorage(updater->storage, peerHistory, isNewConstInfo, connectHistory, isNewConnect);
		PeerInfoNodeRelease(peerHistoryNode, localHistory);
	}

	return BFX_RESULT_SUCCESS;
}

// 开始和对端传输数据
BdtHistory_PeerUpdaterTransferTask* BdtHistory_PeerUpdaterOnStartTransfer(
	BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* remotePeerInfo,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint)
{
	if (memcmp(BdtPeerInfoGetPeerid(remotePeerInfo), &updater->localHistory->peerid, sizeof(BdtPeerid)) == 0)
	{
		return NULL;
	}

	BdtHistory_PeerConnectInfo* localConnectHistory = HistoryPeerFindLocalEndpoint(updater, localEndpoint);
	if (localConnectHistory == NULL)
	{
		return NULL;
	}

	bool isNew = false;
    bool isNewConstInfo = false;

	BfxRwLockWLock(&updater->lock);

	PeerInfoNode* localHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(updater->localHistory);

	PeerInfoNode* peerHistoryNode = HistoryPeerInfoUpdateFull(
		updater, 
		BdtPeerInfoGetPeerid(remotePeerInfo), 
		remotePeerInfo, 
		&isNew,
        &isNewConstInfo);
    assert(!isNewConstInfo);
	BdtHistory_PeerInfo* peerHistory = &peerHistoryNode->peerHistory;

	BdtHistory_PeerUpdaterTransferTask* task = BFX_MALLOC_OBJ(BdtHistory_PeerUpdaterTransferTask);
	memset(task, 0, sizeof(BdtHistory_PeerUpdaterTransferTask));
	task->updater = updater;
	task->peerHistoryNode = peerHistoryNode;
	task->connectInfo = HistoryPeerFindOrAddConnectInfo(peerHistory, remoteEndpoint, &localConnectHistory->endpoint, &task->isNewConnect);
	
	if (task->isNewConnect && task->connectInfo->localEndpoint != NULL)
	{
		PeerInfoNodeAddRef(localHistoryNode);
	}
	PeerInfoNodeAddRef(peerHistoryNode); // 由task持有

	HistoryLimitPeerCount(updater);

	BfxRwLockWUnlock(&updater->lock);

	// BdtPeerHistoryUpdaterOnTransferEnd里再释放
	// PeerInfoNodeRelease(peerHistoryNode, &localHistoryNode->peerHistory);

	return task;
}

// 开始测试和对端的响应时间
void BdtHistory_PeerUpdaterOnTransferRtoTestBegin(BdtHistory_PeerUpdaterTransferTask* task)
{
	task->reqTime = BDT_MS_NOW;
}

// 结束测试和对端的响应时间
void BdtHistory_PeerUpdaterOnTransferRtoTestEnd(BdtHistory_PeerUpdaterTransferTask* task)
{
	task->respTime = BDT_MS_NOW;
	assert(task->reqTime != 0);
	if (task->reqTime == 0 || task->respTime < task->reqTime)
	{
		return;
	}

	task->rto = HistoryPeerStatisticAverage(task->rto, (uint32_t)(task->respTime - task->reqTime));
	task->reqTime = 0;
}

// 有数据发出
void BdtHistory_PeerUpdaterOnTransferDataSend(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes)
{
	SpeedStatisticAddBytes(&task->sendStat, bytes);
}

// 发送暂停，因逻辑需求暂停发送或者发送缓冲区发完并全部被对方确认
void BdtHistory_PeerUpdaterOnTransferSendPause(BdtHistory_PeerUpdaterTransferTask* task)
{
	SpeedStatisticPause(&task->sendStat);
}

// 有数据接收
void BdtHistory_PeerUpdaterOnTransferDataRecv(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes)
{
	SpeedStatisticAddBytes(&task->recvStat, bytes);
}

// 接收暂停，因逻辑需求暂停接收
void BdtHistory_PeerUpdaterOnTransferRecvPause(BdtHistory_PeerUpdaterTransferTask* task)
{
	SpeedStatisticPause(&task->recvStat);
}

// 数据传输停止
void BdtHistory_PeerUpdaterOnTransferEnd(BdtHistory_PeerUpdaterTransferTask* task)
{
	BdtHistory_PeerInfo* localHistory = task->updater->localHistory;

	BdtHistory_PeerConnectInfo* connectInfo = task->connectInfo;

	bool isChanged = HistoryUpdateConnectInfo(connectInfo,
		0,
		HistoryPeerStatisticAverage(connectInfo->rto, task->rto),
		HistoryPeerStatisticAverage(connectInfo->sendSpeed, task->sendStat.speed),
		HistoryPeerStatisticAverage(connectInfo->recvSpeed, task->recvStat.speed),
		0);

	if (isChanged || task->isNewConnect)
	{
		HistoryUpdateConnectInfoToStorage(task->updater->storage,
			&task->peerHistoryNode->peerHistory,
            false,
			task->connectInfo,
			task->isNewConnect);
	}

	PeerInfoNodeRelease(task->peerHistoryNode, localHistory);

	BFX_FREE(task);
}

static BdtHistory_PeerConnectInfo* HistoryPeerInfoAddConnectInfo(
	BdtHistory_PeerInfo* peerHistory,
	const BdtEndpoint* endpoint,
	int64_t foundTime)
{
	BdtHistory_PeerConnectInfo* connectInfo = BFX_MALLOC_OBJ(BdtHistory_PeerConnectInfo);
	memset(connectInfo, 0, sizeof(BdtHistory_PeerConnectInfo));
	connectInfo->endpoint = *endpoint;
	// 清掉签名标记，这里不需要，并且影响搜索
	connectInfo->endpoint.flag &= ~(BDT_ENDPOINT_FLAG_SIGNED);
	connectInfo->foundTime = foundTime;
	bfx_vector_history_connect_push_back(&peerHistory->connectVector, connectInfo);
	return connectInfo;
}

static void HistoryPeerInfoInit(
	const BdtPeerid* peerid, 
	const BdtPeerInfo* peerInfo,
	BdtHistory_PeerInfo* peerHistory)
{
	memset(peerHistory, 0, sizeof(BdtHistory_PeerInfo));
	bfx_vector_history_connect_init(&peerHistory->connectVector);
	bfx_vector_super_node_info_init(&peerHistory->snVector);
	peerHistory->peerid = *peerid;
	
	if (peerInfo)
	{
        peerHistory->flags.hasConst = 1;
		peerHistory->constInfo = *BdtPeerInfoGetConstInfo(peerInfo);
		size_t epCount = 0;
		const BdtEndpoint* epArray = BdtPeerInfoListEndpoint(peerInfo, &epCount);
		HistoryPeerInfoUpdateEndpoints(epArray, epCount, peerHistory);
	}
}

static void HistoryPeerInfoUpdateEndpoints(
	const BdtEndpoint* endpoints,
	size_t count,
	BdtHistory_PeerInfo* peerHistory)
{
	const int64_t now = BDT_MS_NOW;

	for (size_t i = 0; i < count; i++)
	{
		const BdtEndpoint* ep = endpoints + i;
		size_t j = 0;
		for (j = 0; j < peerHistory->connectVector.size; j++)
		{
			BdtHistory_PeerConnectInfo* existConnect = peerHistory->connectVector.connections[j];
			if (HistoryEndpointIsEqual(ep, &existConnect->endpoint))
			{
                // 更新BDT_ENDPOINT_FLAG_STATIC_WAN标记
                existConnect->endpoint.flag &= ~(BDT_ENDPOINT_FLAG_STATIC_WAN);
                existConnect->endpoint.flag |= (ep->flag & BDT_ENDPOINT_FLAG_STATIC_WAN);
				existConnect->foundTime = now;
				break;
			}
		}

		if (j == peerHistory->connectVector.size)
		{
			HistoryPeerInfoAddConnectInfo(peerHistory, ep, now);
		}
	}
}

static void HistoryMovePeerHistoryToHeader(PeerUpdater* updater, PeerInfoNode* historyNode)
{
    // 最近访问过的数据，往表头挪
    if (updater->peerListTail == historyNode &&
        updater->peerList != historyNode)
    {
        updater->peerListTail = historyNode->previousPeer;
    }
    sglib_peer_history_delete(&updater->peerList, historyNode);
    sglib_peer_history_add(&updater->peerList, historyNode);
}

static PeerInfoNode* HistoryPeerMakeSurePeerHistoryExist(
	PeerUpdater* updater, 
	const BdtPeerid* peerid, 
	const BdtPeerInfo* peerInfo,
	bool* isNew,
    bool* isNewConstInfo)
{
	*isNew = true;
    *isNewConstInfo = false;
	BdtHistory_PeerInfo* peerHistory = NULL;
	PeerInfoNode* peerHistoryNode = NULL;

	PeeridMapNode targetPeerNode = { .peerid = peerid };
	PeeridMapNode* foundPeerNode = sglib_peerid_history_map_find_member(updater->peeridMap, &targetPeerNode);
	if (foundPeerNode != NULL)
	{
		*isNew = false;
		peerHistoryNode = foundPeerNode->peerHistoryNode;
		peerHistory = &peerHistoryNode->peerHistory;

        HistoryMovePeerHistoryToHeader(updater, peerHistoryNode);

		if (peerInfo)
		{
            if (!peerHistory->flags.hasConst)
            {
                *isNewConstInfo = true;
                peerHistory->flags.hasConst = 1;
			    peerHistory->constInfo = *BdtPeerInfoGetConstInfo(peerInfo);
            }

			size_t endpointCount = 0;
			const BdtEndpoint* endpoints = BdtPeerInfoListEndpoint(peerInfo, &endpointCount);
			HistoryPeerInfoUpdateEndpoints(
				endpoints,
				endpointCount,
				peerHistory);
		}
	}
	else
	{
		peerHistoryNode = BFX_MALLOC_OBJ(PeerInfoNode);
		peerHistoryNode->ref = 1;
		peerHistoryNode->snHistory.lastResponseTime = 0;
		peerHistory = &peerHistoryNode->peerHistory;

		HistoryPeerInfoInit(peerid, peerInfo, peerHistory);
		HistoryPeerUpdaterAddHistory(updater, peerHistoryNode);
	}
	return peerHistoryNode;
}

static PeerInfoNode* HistoryPeerInfoUpdateFull(
	PeerUpdater* updater,
	const BdtPeerid* peerid,
	const BdtPeerInfo* peerInfo,
	bool* isNew,
    bool* isNewConstInfo)
{
	PeerInfoNode* peerHistoryNode = HistoryPeerMakeSurePeerHistoryExist(updater, peerid, peerInfo, isNew, isNewConstInfo);
	BdtHistory_PeerInfo* peerHistory = &peerHistoryNode->peerHistory;

	if (peerInfo)
	{
		size_t snCount = 0;
		const BdtPeerInfo** snList = BdtPeerInfoListSnInfo(peerInfo, &snCount);
		for (size_t i = 0; i < snCount; i++)
		{
			const BdtPeerInfo* snInfo = snList[i];
			const BdtPeerid* snPeerid = BdtPeerInfoGetPeerid(snInfo);
			bool isSnNew = false;
            bool isNewSnConstInfo = false;
			PeerInfoNode* snHistoryNode = HistoryPeerMakeSurePeerHistoryExist(
				updater, 
				BdtPeerInfoGetPeerid(snInfo), 
				snInfo, 
				&isSnNew,
                &isNewSnConstInfo);
            assert(!isNewSnConstInfo);
			BdtHistory_SuperNodeInfo* snHistory = &snHistoryNode->snHistory;

			if (!snHistory->peerInfo.flags.isSn)
			{
				snHistory->peerInfo.flags.isSn = 1;
				sglib_super_node_history_add(&updater->superNodeList, snHistoryNode);
			}

			HistoryPeerInfoAddSuperNode(peerHistory, snHistory);
		}
	}
	
	return peerHistoryNode;
}

static void HistoryPeerUpdaterAddHistory(
	PeerUpdater* updater,
	PeerInfoNode* peerHistoryNode)
{
	BdtHistory_PeerInfo* peerHistory = &peerHistoryNode->peerHistory;
	sglib_peer_history_add(&updater->peerList, peerHistoryNode);
	updater->peerCount++;
	if (updater->peerListTail == NULL)
	{
		updater->peerListTail = peerHistoryNode;
	}

	PeeridMapNode* peeridMapNode = BFX_MALLOC_OBJ(PeeridMapNode);
	memset(peeridMapNode, 0, sizeof(PeeridMapNode));
	peeridMapNode->peerid = &peerHistory->peerid;
	peeridMapNode->peerHistoryNode = peerHistoryNode;
	assert(sglib_peerid_history_map_find_member(updater->peeridMap, peeridMapNode) == NULL);
	sglib_peerid_history_map_add(&updater->peeridMap, peeridMapNode);
	if (peerHistoryNode->peerHistory.flags.isSn)
	{
		sglib_super_node_history_add(&updater->superNodeList, peerHistoryNode);
	}
}

static void HistoryPeerDestroyPeeridMap(PeeridMapNode* map)
{
	PeeridMapNode * node = map;
	while (node != NULL)
	{
		sglib_peerid_history_map_delete(&map, node);
		BFX_FREE(node);
		node = map;
	}
	assert(map == NULL);
}

static void HistoryPeerNodeDelete(PeerInfoNode* node,
	BdtHistory_PeerInfo* localHistory)
{
	PeerInfoNode* localHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(localHistory);

	for (size_t i = 0; i < node->peerHistory.connectVector.size; i++)
	{
		BdtHistory_PeerConnectInfo* connectInfo = node->peerHistory.connectVector.connections[i];
		BLOG_CHECK(connectInfo != NULL);
		if (connectInfo != NULL && connectInfo->localEndpoint != NULL)
		{
			PeerInfoNodeRelease(localHistoryNode, NULL);
		}
		BFX_FREE(connectInfo);
	}
	bfx_vector_history_connect_cleanup(&node->peerHistory.connectVector);
	bfx_vector_super_node_info_cleanup(&node->peerHistory.snVector);
	BFX_FREE(node);
}

static int32_t PeerInfoNodeAddRef(PeerInfoNode* node)
{
	return BfxAtomicInc32(&node->ref);
}

static int32_t PeerInfoNodeRelease(PeerInfoNode* node,
	BdtHistory_PeerInfo* localHistory)
{
	int32_t ref = BfxAtomicDec32(&node->ref);
	if (ref == 0)
	{
		HistoryPeerNodeDelete(node, localHistory);
	}
	return ref;
}

int32_t BdtHistory_PeerHistoryAddRef(const BdtHistory_PeerInfo* peerHistory)
{
    return PeerInfoNodeAddRef(BDT_GET_PEER_NODE_FROM_HISTORY_INFO(peerHistory));
}

int32_t BdtHistory_PeerHistoryRelease(PeerUpdater* updater, const BdtHistory_PeerInfo* peerHistory)
{
    return PeerInfoNodeRelease(BDT_GET_PEER_NODE_FROM_HISTORY_INFO(peerHistory), updater->localHistory);
}

static void HistoryPeerDestroyHistoryList(PeerUpdater* updater)
{
	PeerInfoNode* list = updater->peerList;
	PeerInfoNode* node = list;
	while (node != NULL)
	{
		sglib_peer_history_delete(&list, node);
		PeerInfoNodeRelease(node, updater->localHistory);
		node = list;
	}
	assert(list == NULL);
	updater->peerList = updater->peerListTail = NULL;
}

static BdtHistory_SuperNodeInfo* HistoryPeerInfoAddSuperNode(
	BdtHistory_PeerInfo* peerHistory,
	BdtHistory_SuperNodeInfo* snPeerHistory
)
{
	size_t i = 0;
	BdtHistory_SuperNodeInfo* snHistory = NULL;
	for (i = 0; i < peerHistory->snVector.size; i++)
	{
		snHistory = peerHistory->snVector.snHistorys[i];
		if (snHistory == snPeerHistory)
		{
			break;
		}
	}

	if (i == peerHistory->snVector.size)
	{
		bfx_vector_super_node_info_push_back(&peerHistory->snVector, snPeerHistory);
		return peerHistory->snVector.snHistorys[peerHistory->snVector.size - 1];
	}
	return snHistory;
}

static BdtHistory_PeerConnectInfo* HistoryPeerFindLocalEndpoint(PeerUpdater* updater,
	const BdtEndpoint* localEndpoint)
{
	if (localEndpoint == NULL)
	{
		return NULL;
	}

	BdtHistory_PeerInfo* localHistory = updater->localHistory;
	BdtHistory_PeerConnectInfo* localConnectHistory = NULL;
	size_t i = 0;
	for (i = 0; i < localHistory->connectVector.size; i++)
	{
		localConnectHistory = localHistory->connectVector.connections[i];
		if (localEndpoint == &localConnectHistory->endpoint ||
			HistoryEndpointIsEqual(localEndpoint, &localConnectHistory->endpoint))
		{
			return localConnectHistory;
		}
	}
	return NULL;
}

// localEndpoint 应该指向updater->localHistory内部保存的Endpoint对象
static BdtHistory_PeerConnectInfo* HistoryPeerFindOrAddConnectInfo(
	BdtHistory_PeerInfo* peerHistory,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint,
	bool* isNew)
{
	BdtHistory_PeerConnectInfo* connectHistory = NULL;
	for (size_t i = 0; i < peerHistory->connectVector.size; i++)
	{
		connectHistory = peerHistory->connectVector.connections[i];

		if (remoteEndpoint == &connectHistory->endpoint ||
			HistoryEndpointIsEqual(&connectHistory->endpoint, remoteEndpoint))
		{
			if (connectHistory->localEndpoint == NULL)
			{
				connectHistory->localEndpoint = localEndpoint;
                connectHistory->endpoint.flag &= ~BDT_ENDPOINT_FLAG_STATIC_WAN;
                connectHistory->endpoint.flag |= (remoteEndpoint->flag & BDT_ENDPOINT_FLAG_STATIC_WAN);
				*isNew = true;
				return connectHistory;
			}
			else if(connectHistory->localEndpoint == localEndpoint)
			{
                connectHistory->endpoint.flag &= ~BDT_ENDPOINT_FLAG_STATIC_WAN;
                connectHistory->endpoint.flag |= (remoteEndpoint->flag & BDT_ENDPOINT_FLAG_STATIC_WAN);
                *isNew = false;
				return connectHistory;
			}
			assert(localEndpoint == NULL || !HistoryEndpointIsEqual(connectHistory->localEndpoint, localEndpoint));
		}
	}
	*isNew = true;
	connectHistory = HistoryPeerInfoAddConnectInfo(peerHistory, remoteEndpoint, 0);
	connectHistory->localEndpoint = localEndpoint;
	return connectHistory;
}

static uint32_t HistoryPeerStatisticAverage(uint32_t oldValue, uint32_t newValue)
{
	if (oldValue == 0)
	{
		return newValue;
	}
	return (oldValue * 7 + newValue) >> 3;
}

static void SpeedStatisticAddBytes(SpeedStatistic* stat, uint32_t bytes)
{
	if (bytes > 0)
	{
		stat->byteSize += bytes;
		if (stat->startTime == 0)
		{
			stat->startTime = BDT_MS_NOW;
		}
	}
}

static void SpeedStatisticPause(SpeedStatistic* stat)
{
	if (stat->startTime == 0 || stat->byteSize == 0)
	{
		return;
	}

	int64_t now = BDT_MS_NOW;
	if (now <= stat->startTime)
	{
		return;
	}

	uint32_t speed = stat->byteSize / (uint32_t)(now - stat->startTime);
	stat->speed = HistoryPeerStatisticAverage(stat->speed, speed);
	stat->startTime = 0;
}

void BdtHistory_PeerUpdaterOnLoadDone(
	PeerUpdater* updater,
	BdtStorage* storage,
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount
)
{
	if (errorCode != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("load history failed, errorCode:%d.", errorCode);
		return;
	}

	BLOG_INFO("load history success, count:%d", (int)historyCount);
	BLOG_CHECK(historyCount >= 0);
	BLOG_CHECK(updater != NULL);
	BLOG_CHECK(storage != NULL);

	HistoryPeerUpdaterMergeOuterHistory(
		updater,
		storage,
		historys,
		historyCount
	);

	HistoryEnsurePeerInfoToStorage(storage, updater->localHistory, false);
}

// 把外部历史记录列表合并；
static void HistoryPeerUpdaterMergeOuterHistoryNoLock(
	PeerUpdater* updater,
	const BdtHistory_PeerInfo* outerHistorys[],
	size_t historyCount
)
{
	// 保存原有节点链表头，外部数据加入完成后，这些节点会被移到表尾；
	// 需要把它们再次移到表头，因为它们是最可能活跃的
	PeerInfoNode* peerListOld = updater->peerList;
	PeerInfoNode* snListOld = updater->superNodeList;

	for (size_t i = 0; i < historyCount; i++)
	{
		const BdtHistory_PeerInfo* outerHistory = outerHistorys[i];
		if (outerHistory == NULL)
		{
			continue;
		}
		if (memcmp(&outerHistory->peerid, &updater->localHistory->peerid, sizeof(BdtPeerid)) == 0)
		{
			updater->localHistory->flags.isStorage = 1;
		}
		PeerUpdaterAddOuterHistory(updater, outerHistory, NULL, 0);
	}

	if (peerListOld != NULL)
	{
		// 把添加外部记录前已经存在的节点信息重新移动到表头
		PeerInfoNode* newPeerListTail = peerListOld->previousPeer;
		if (newPeerListTail != NULL)
		{
			newPeerListTail->nextPeer = NULL;
			peerListOld->previousPeer = NULL;
			updater->peerListTail->nextPeer = updater->peerList;
			updater->peerList->previousPeer = updater->peerListTail;
			updater->peerList = peerListOld;
			updater->peerListTail = newPeerListTail;
		}

		if (snListOld != NULL)
		{
			PeerInfoNode* snListTail = snListOld->previousSn;
			if (snListTail != NULL)
			{
				snListTail->nextSn = NULL;
				snListOld->previousSn = NULL;
				for (snListTail = snListOld;
					snListTail->nextSn != NULL;
					snListTail = snListTail->nextSn);
				snListTail->nextSn = updater->superNodeList;
				updater->superNodeList->previousSn = snListTail;
				updater->superNodeList = snListOld;
			}
		}
	}
}

static void HistoryPeerUpdaterMergeOuterHistory(
	PeerUpdater* updater,
	BdtStorage* storage,
	const BdtHistory_PeerInfo* outerHistorys[],
	size_t historyCount
)
{

	BLOG_CHECK(updater != NULL);
	BLOG_CHECK(outerHistorys != NULL);
	BLOG_CHECK(storage != NULL);

	BfxRwLockWLock(&updater->lock);

	updater->storage = storage;
	if (historyCount > 0)
	{
		HistoryPeerUpdaterMergeOuterHistoryNoLock(
			updater,
			outerHistorys,
			historyCount
		);
	}

	HistoryLimitPeerCount(updater);

	BfxRwLockWUnlock(&updater->lock);
}

// 加入外部历史记录
static BdtHistory_PeerInfo* PeerUpdaterAddOuterHistory(
	PeerUpdater* updater,
	const BdtHistory_PeerInfo* outerHistory,
    const BdtPeerid* addSNPeerids,
    size_t addSNCount
)
{
	BLOG_CHECK(updater != NULL);
	BLOG_CHECK(outerHistory != NULL);
	BLOG_CHECK(!outerHistory->flags.isLocal);

	BdtHistory_PeerInfo* peerHistory = NULL;
	PeerInfoNode* peerHistoryNode = NULL;

	PeeridMapNode targetNode;
	targetNode.peerid = &outerHistory->peerid;
	PeeridMapNode* peerHistoryMapNode = sglib_peerid_history_map_find_member(updater->peeridMap, &targetNode);
	if (peerHistoryMapNode == NULL)
	{
		peerHistoryNode = BFX_MALLOC_OBJ(PeerInfoNode);
		memset(peerHistoryNode, 0, sizeof(PeerInfoNode));
		peerHistoryNode->ref = 1;
		peerHistory = &peerHistoryNode->peerHistory;

		bfx_vector_history_connect_init(&peerHistory->connectVector);
		bfx_vector_history_connect_reserve(&peerHistory->connectVector, outerHistory->connectVector.size);
		bfx_vector_super_node_info_init(&peerHistory->snVector);
		bfx_vector_super_node_info_reserve(&peerHistory->snVector, outerHistory->snVector.size);
		peerHistory->constInfo = outerHistory->constInfo;
		peerHistory->peerid = outerHistory->peerid;

		HistoryPeerUpdaterAddHistory(updater, peerHistoryNode);
	}
	else
	{
		peerHistoryNode = peerHistoryMapNode->peerHistoryNode;
		peerHistory = &peerHistoryNode->peerHistory;
	}

	BLOG_CHECK(outerHistory->flags.zero == 0);
	peerHistory->flags.isStorage = outerHistory->flags.isStorage;
	if (!peerHistory->flags.isSn && outerHistory->flags.isSn)
	{
		peerHistory->flags.isSn = 1;
		sglib_super_node_history_add(&updater->superNodeList, peerHistoryNode);
	}

	for (size_t i = 0; i < outerHistory->connectVector.size; i++)
	{
		const BdtHistory_PeerConnectInfo* outerConnectInfo = outerHistory->connectVector.connections[i];
		if (outerConnectInfo != NULL)
		{
			// 本地地址如果改变就置NULL
			const BdtHistory_PeerConnectInfo* localConnectInfo = HistoryPeerFindLocalEndpoint(updater, outerConnectInfo->localEndpoint);
			const BdtEndpoint* localEndpoint = localConnectInfo == NULL? NULL : &localConnectInfo->endpoint;
			const BdtEndpoint* remoteEndpoint = &outerConnectInfo->endpoint;
			bool isNewConnect = false;
			BdtHistory_PeerConnectInfo* newConnectInfo = HistoryPeerFindOrAddConnectInfo(peerHistory,
				remoteEndpoint,
				localEndpoint,
				&isNewConnect);
			BLOG_CHECK(newConnectInfo != NULL);

			if (isNewConnect && newConnectInfo->localEndpoint != NULL)
			{
				PeerInfoNode* localHistoryNode = BDT_GET_PEER_NODE_FROM_HISTORY_INFO(updater->localHistory);
				PeerInfoNodeAddRef(localHistoryNode);
			}

			if (outerConnectInfo->foundTime > newConnectInfo->foundTime)
			{
				newConnectInfo->foundTime = outerConnectInfo->foundTime;
			}
			if (outerConnectInfo->lastConnectTime > newConnectInfo->lastConnectTime)
			{
				newConnectInfo->lastConnectTime = outerConnectInfo->lastConnectTime;
			}
			if (newConnectInfo->rto == 0)
			{
				newConnectInfo->rto = outerConnectInfo->rto;
			}
			if (newConnectInfo->sendSpeed == 0)
			{
				newConnectInfo->sendSpeed = outerConnectInfo->sendSpeed;
			}
			if (newConnectInfo->recvSpeed == 0)
			{
				newConnectInfo->recvSpeed = outerConnectInfo->recvSpeed;
			}
			BLOG_CHECK(outerConnectInfo->flags.zero == 0);
			if (newConnectInfo->flags.u32 == 0)
			{
				newConnectInfo->flags.u32 = outerConnectInfo->flags.u32;
			}
		}
	}

	if (!outerHistory->flags.isSn)
	{
		for (size_t i = 0; i < outerHistory->snVector.size; i++)
		{
			BdtHistory_SuperNodeInfo* outerSnHistory = outerHistory->snVector.snHistorys[i];
			if (outerSnHistory == NULL)
			{
				continue;
			}

			BdtHistory_PeerInfo* outerSnPeerInfo = &outerSnHistory->peerInfo;
			BLOG_CHECK(outerSnPeerInfo->flags.isSn);
			BdtHistory_SuperNodeInfo* newSnHistory = NULL;
			if (memcmp(&outerSnPeerInfo->peerid, &updater->localHistory->peerid, sizeof(BdtPeerid)) == 0)
			{
				// 本地节点是SN的情况，在初始化的时候就应该明确
				if (updater->localHistory->flags.isSn)
				{
					newSnHistory = (BdtHistory_SuperNodeInfo*)updater->localHistory;
				}
			}
			else
			{
				newSnHistory = (BdtHistory_SuperNodeInfo*)PeerUpdaterAddOuterHistory(updater, outerSnPeerInfo, NULL, 0);
				if (newSnHistory != NULL)
				{
					BLOG_CHECK(newSnHistory->peerInfo.flags.isSn);
					newSnHistory->peerInfo.flags.isSn = 1;
				}
			}

			if (newSnHistory != NULL)
			{
				if (outerSnHistory->lastResponseTime > newSnHistory->lastResponseTime)
				{
					newSnHistory->lastResponseTime = outerSnHistory->lastResponseTime;
				}
				HistoryPeerInfoAddSuperNode(peerHistory, newSnHistory);
			}
		}

        for (size_t ai = 0; ai < addSNCount; ai++)
        {
            PeeridMapNode targetSNNode;
            targetSNNode.peerid = addSNPeerids + ai;
            PeeridMapNode* snHistoryMapNode = sglib_peerid_history_map_find_member(updater->peeridMap, &targetSNNode);
            if (snHistoryMapNode == NULL)
            {
                continue;
            }

            BdtHistory_SuperNodeInfo* snHistory = &snHistoryMapNode->peerHistoryNode->snHistory;
            if (!snHistory->peerInfo.flags.isSn)
            {
                continue;
            }
            HistoryPeerInfoAddSuperNode(peerHistory, snHistory);
        }
	}

	return peerHistory;
}

static bool HistoryEndpointIsEqual(const BdtEndpoint* ep1, const BdtEndpoint* ep2)
{
	uint8_t flag1 = (ep1->flag & ~(BDT_ENDPOINT_FLAG_SIGNED | BDT_ENDPOINT_FLAG_STATIC_WAN));
	uint8_t flag2 = (ep2->flag & ~(BDT_ENDPOINT_FLAG_SIGNED | BDT_ENDPOINT_FLAG_STATIC_WAN));

	if (flag1 != flag2)
	{
		return false;
	}
	if (ep1->port != ep2->port)
	{
		return false;
	}
	if (ep1->flag & BDT_ENDPOINT_IP_VERSION_4)
	{
		return memcmp(ep1->address, ep2->address, sizeof(ep1->address)) == 0;
	}
	return memcmp(ep1->addressV6, ep2->addressV6, sizeof(ep1->addressV6)) == 0;
}

static bool HistoryEnsurePeerInfoToStorage(
	BdtStorage* storage,
	BdtHistory_PeerInfo* peerInfo,
    bool isNewConstInfo)
{
	if (storage != NULL)
	{
		if (!peerInfo->flags.isStorage)
		{
			peerInfo->flags.isStorage = 1;
			uint32_t result = BdtHistory_StorageAddPeer(storage, peerInfo);
			if (result != BFX_RESULT_PENDING &&
				result != BFX_RESULT_SUCCESS &&
				result != BFX_RESULT_ALREADY_OPERATION)
			{
				peerInfo->flags.isStorage = 0;
			}
			return true;
		}
        else if (isNewConstInfo)
        {
			peerInfo->flags.isStorage = 1;
			uint32_t result = BdtHistory_StorageSetConstInfo(storage, peerInfo);
			if (result != BFX_RESULT_PENDING &&
				result != BFX_RESULT_SUCCESS &&
				result != BFX_RESULT_ALREADY_OPERATION)
			{
				peerInfo->flags.isStorage = 0;
			}
			return true;            
        }
	}
	return false;
}

static void HistoryUpdateSuperNodeToStorage(
	BdtStorage* storage,
	BdtHistory_PeerInfo* peerInfo,
	BdtHistory_SuperNodeInfo* snInfo,
	bool isNewSnForPeer)
{
	if (storage != NULL)
	{
		bool isNewPeer = HistoryEnsurePeerInfoToStorage(storage, peerInfo, false);
		bool isNewSn = HistoryEnsurePeerInfoToStorage(storage, &snInfo->peerInfo, false);

		if (peerInfo->flags.isStorage && snInfo->peerInfo.flags.isStorage)
		{
			BdtHistory_StorageUpdateSuperNode(storage,
				&peerInfo->peerid,
				&snInfo->peerInfo.peerid,
				snInfo->lastResponseTime,
				isNewSnForPeer || isNewPeer || isNewSn);
		}
	}
}

static void HistoryUpdateConnectInfoToStorage(BdtStorage* storage,
	BdtHistory_PeerInfo* peerHistory,
    bool isNewConstInfo,
	BdtHistory_PeerConnectInfo* connectInfo,
	bool isNewConnect)
{
	if (storage != NULL)
	{
		bool isNewPeer = HistoryEnsurePeerInfoToStorage(storage, peerHistory, isNewConstInfo);
		if (peerHistory->flags.isStorage)
		{
			BdtHistory_StorageUpdateConnect(storage, &peerHistory->peerid, connectInfo, isNewConnect || isNewPeer);
		}
	}
}

static void HistoryLimitPeerCount(PeerUpdater* updater)
{
	if (updater->peerCount >= updater->config->peerCapacity + 2)
	{
		PeerInfoNode* willRemoveNode = updater->peerListTail;
		while (updater->peerCount > updater->config->peerCapacity && willRemoveNode != NULL)
		{
			PeerInfoNode* nextRemoveNode = willRemoveNode->previousPeer;
			if (!(willRemoveNode->peerHistory.flags.isSn ||
				willRemoveNode->peerHistory.flags.isLocal ||
                willRemoveNode->ref > 1))
			{
				if (willRemoveNode == updater->peerList)
				{
					updater->peerList = willRemoveNode->nextPeer;
					BLOG_CHECK(nextRemoveNode == NULL);
				}
				sglib_peer_history_delete(&updater->peerListTail, willRemoveNode);

				PeeridMapNode mapNode = { .peerid = &willRemoveNode->peerHistory.peerid };
				PeeridMapNode* foundMapNode = NULL;
				sglib_peerid_history_map_delete_if_member(&updater->peeridMap, &mapNode, &foundMapNode);
				BLOG_CHECK(foundMapNode != NULL);
				BFX_FREE(foundMapNode);

				PeerInfoNodeRelease(willRemoveNode, updater->localHistory);
				updater->peerCount--;
			}
			willRemoveNode = nextRemoveNode;
		}
		BLOG_CHECK(updater->config->peerCapacity == 0 || updater->peerListTail != NULL);

		if (updater->peerCount > updater->config->peerCapacity)
		{
			updater->peerCount = updater->config->peerCapacity;
		}
	}
}

static bool HistoryUpdateConnectInfo(BdtHistory_PeerConnectInfo* connectInfo,
	int64_t lastConnectTime,
	uint32_t rto,
	uint32_t sendSpeed,
	uint32_t recvSpeed,
	uint32_t flags)
{
	bool isChanged = false;

	if (lastConnectTime > 0 && lastConnectTime > connectInfo->lastConnectTime + 60000)
	{
		connectInfo->lastConnectTime = lastConnectTime;
		isChanged = true;
	}

	if (rto > 0 && connectInfo->rto != rto)
	{
		connectInfo->rto = rto;
		isChanged = true;
	}

	if (sendSpeed > 0 && connectInfo->sendSpeed != sendSpeed)
	{
		connectInfo->sendSpeed = sendSpeed;
		isChanged = true;
	}

	if (recvSpeed > 0 && connectInfo->recvSpeed != recvSpeed)
	{
		connectInfo->recvSpeed = recvSpeed;
		isChanged = true;
	}

	if (flags != 0 && connectInfo->flags.u32 != flags)
	{
		connectInfo->flags.u32 = flags;
		isChanged = true;
	}
	return isChanged;
}

static bool HistoryUpdateSuperNodeInfo(BdtHistory_SuperNodeInfo* snInfo,
	int64_t lastResponseTime)
{
	bool isChanged = false;
	if (lastResponseTime > 0 && lastResponseTime > snInfo->lastResponseTime + 60)
	{
		snInfo->lastResponseTime = lastResponseTime;
		isChanged = true;
	}

	if (!snInfo->peerInfo.flags.isSn)
	{
		snInfo->peerInfo.flags.isSn = 1;
		isChanged = true;
	}
	return isChanged;
}

static void HistoryLoadPeerHistoryByPeerid(
    uint32_t errorCode,
    const BdtHistory_PeerInfo* history,
    const BdtPeerid snPeerids[],
    size_t snCount,
    void* userDataParam
    )
{
    BLOG_CHECK(userDataParam != NULL);
    LoadPeerHistoryByPeerIdUserData* userData = (LoadPeerHistoryByPeerIdUserData*)userDataParam;
    PeerUpdater* updater = userData->updater;

    if (errorCode != BFX_RESULT_SUCCESS)
    {
        BLOG_WARN("load history failed, errorCode:%d.", errorCode);
    }
    else
    {
        BLOG_CHECK(history != NULL);
        BLOG_HEX(history->peerid.pid, 15, "load history success.");
        BLOG_CHECK(snCount >= 0);

        BfxRwLockWLock(&updater->lock);

        PeerUpdaterAddOuterHistory(updater, history, snPeerids, snCount);

        BfxRwLockWUnlock(&updater->lock);
    }

    BLOG_CHECK(userData->callback != NULL);

    BdtHistory_PeerUpdaterFindByPeerid(userData->updater,
        &userData->peerid,
        false,
        userData->callback,
        &userData->userData);

    if (userData->userData.lpfnReleaseUserData != NULL &&
        userData->userData.userData != NULL)
    {
        userData->userData.lpfnReleaseUserData(userData->userData.userData);
    }
    BFX_FREE(userData);
}

BdtStorage* BdtHistory_PeerUpdaterGetStorage(BdtHistory_PeerUpdater* updater)
{
    return updater->storage;
}