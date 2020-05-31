#ifndef __BDT_TUNNEL_HISTORY_IMP_INL__
#define __BDT_TUNNEL_HISTORY_IMP_INL__
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of History module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include "./TunnelHistory.h"
#include <SGLib/SGLib.h>
#include "./Storage.h"
#include "../Interface/TcpInterface.h"


typedef struct TcpLogMapNode TcpLogMapNode;

BFX_VECTOR_DEFINE_FUNCTIONS(tcp_tunnel_log, Bdt_TunnelHistoryTcpLog, logs, size, _allocSize)

BFX_VECTOR_DEFINE_FUNCTIONS(tcp_tunnel_log_for_remote, Bdt_TunnelHistoryTcpLogForRemoteEndpoint, logsForRemote, size, _allocSize)

typedef struct TcpLogMapNode
{
	struct TcpLogMapNode* left;
	struct TcpLogMapNode* right;
	uint8_t color;

    Bdt_TunnelHistoryTcpLogForLocalEndpoint log4Local;
} TcpLogMapNode, tcp_log_map;

static int tcp_log_map_compare(const tcp_log_map* left, const tcp_log_map* right)
{
	return BdtEndpointCompare(&left->log4Local.local, &right->log4Local.local, true);
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(tcp_log_map, left, right, color, tcp_log_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(tcp_log_map, left, right, color, tcp_log_map_compare)

struct Bdt_TunnelHistory
{
	int32_t refCount;
	BfxRwLock tcpLogLock;
	// tcpLogLock protected members begin
	TcpLogMapNode* tcpLogMap;
    int logCount;
	// tcpLogLock protected members finish

    const BdtHistory_PeerInfo* peerHistory;

	BdtPeerid remotePeerid;
	const BdtTunnelHistoryConfig config;
    BdtHistory_PeerUpdater* updater;

	Bdt_TunnelHistoryOnLoadFromStorage loadCallback;
	BfxUserData loadCallbackUserData;
};

int32_t Bdt_TunnelHistoryAddRef(Bdt_TunnelHistory* history)
{
	return BfxAtomicInc32(&history->refCount);
}

int32_t Bdt_TunnelHistoryRelease(Bdt_TunnelHistory* history)
{
	int32_t refCount = BfxAtomicDec32(&history->refCount);
	if (refCount <= 0)
	{
		{
			TcpLogMapNode* node = NULL;
			while ((node = history->tcpLogMap) != NULL)
			{
				sglib_tcp_log_map_delete(&history->tcpLogMap, node);
                for (size_t i = 0; i < node->log4Local.logForRemoteVector.size; i++) {
                    bfx_vector_tcp_tunnel_log_cleanup(&node->log4Local.logForRemoteVector.logsForRemote[i].logVector);
                }
				bfx_vector_tcp_tunnel_log_for_remote_cleanup(&node->log4Local.logForRemoteVector);
				BfxFree(node);
			}
		}


		BfxRwLockDestroy(&history->tcpLogLock);

		if (history->loadCallbackUserData.lpfnReleaseUserData)
		{
			history->loadCallbackUserData.lpfnReleaseUserData(history->loadCallbackUserData.userData);
		}

        if (history->peerHistory != NULL)
        {
            BdtHistory_PeerHistoryRelease(history->updater, history->peerHistory);
            history->peerHistory = NULL;
        }
		BfxFree(history);
	}
	return refCount;
}

static void LoadTcpTunnelHistoryFromStorageCallback(
    uint32_t errorCode,
    const BdtPeerid* remotePeerid,
    const Bdt_TunnelHistoryTcpLogForLocalEndpoint* loadLog4LocalArray,
    size_t endpointCount,
    void* userData
);
static void LoadPeerHistoryCallback(
    const BdtHistory_PeerInfo* peerHistory,
    void* userData);

static void BFX_STDCALL TunnelHistoryAsUserDataAddRef(void* userData)
{
	Bdt_TunnelHistoryAddRef((Bdt_TunnelHistory*)userData);
}

static void BFX_STDCALL TunnelHistoryAsUserDataRelease(void* userData)
{
	Bdt_TunnelHistoryRelease((Bdt_TunnelHistory*)userData);
}

static void TunnelHistoryAsUserData(Bdt_TunnelHistory* history, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = TunnelHistoryAsUserDataAddRef;
	userData->lpfnReleaseUserData = TunnelHistoryAsUserDataRelease;
	userData->userData = history;
}

Bdt_TunnelHistory* Bdt_TunnelHistoryCreate(
	const BdtPeerid* remotePeerid,
    BdtHistory_PeerUpdater* updater,
	const BdtTunnelHistoryConfig* config, 
	Bdt_TunnelHistoryOnLoadFromStorage loadCallback, 
	const BfxUserData* udCallback)
{
	Bdt_TunnelHistory* history = BFX_MALLOC_OBJ(Bdt_TunnelHistory);
	memset(history, 0, sizeof(Bdt_TunnelHistory));
	*(BdtTunnelHistoryConfig*)(&history->config) = *config;
	history->remotePeerid = *remotePeerid;
	history->updater = updater;
	history->refCount = 1;
	BfxRwLockInit(&history->tcpLogLock);

	BfxUserData userData;
	TunnelHistoryAsUserData(history, &userData);

	history->loadCallback = loadCallback;
	history->loadCallbackUserData = *udCallback;
	if (udCallback->lpfnAddRefUserData)
	{
		udCallback->lpfnAddRefUserData(udCallback->userData);
	}

    BdtHistory_PeerUpdaterFindByPeerid(updater,
        remotePeerid,
        true,
        LoadPeerHistoryCallback,
        &userData);

	BdtHistory_LoadTunnelHistoryTcpLog(updater->storage,
		remotePeerid,
		config->logKeepTime * 1000,
		LoadTcpTunnelHistoryFromStorageCallback,
		&userData
	);
	return history;
}


static bool TunnelHistoryTcpLogMerge(
	Bdt_TunnelHistoryTcpLog* existLog,
	Bdt_TunnelHistoryTcpLogType type)
{
	if (existLog == NULL)
	{
		return false;
	}

	if (existLog->type == type)
	{
		existLog->times++;
		return true;
	}
	else
	{
		// <TODO>
	}
	return false;
}

static uint32_t TunnelHistoryAddTcpLogImplNoLock(
	Bdt_TunnelHistory* history,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLog* inoutLog,
	int64_t* oldTimeStamp,
	bool* outMerged)
{
	TcpLogMapNode toFind;
	toFind.log4Local.local = *localEndpoint;

	TcpLogMapNode* targetMapNode = sglib_tcp_log_map_find_member(history->tcpLogMap, &toFind);
    Bdt_TunnelHistoryTcpLogForRemoteEndpoint* log4Remote = NULL;
	Bdt_TunnelHistoryTcpLog* lastLog = NULL;
	if (targetMapNode != NULL)
	{
		// merge
        for (size_t i = 0; i < targetMapNode->log4Local.logForRemoteVector.size; i++) {
            log4Remote = targetMapNode->log4Local.logForRemoteVector.logsForRemote + i;
            if (BdtEndpointCompare(&log4Remote->remote, remoteEndpoint, true) == 0) {
                lastLog = log4Remote->logVector.logs + log4Remote->logVector.size - 1;
            }
        }
	}

    if (lastLog != NULL)
    {
        *oldTimeStamp = lastLog->timestamp;
        if (*outMerged = TunnelHistoryTcpLogMerge(lastLog, inoutLog->type))
        {
            *inoutLog = *lastLog;
            return BFX_RESULT_SUCCESS;
        }
    }

	// 先试着用淘汰的内存，其次才分配内存
	if (history->logCount >= history->config.tcpLogCacheSize)
	{
        TcpLogMapNode* discardMapNode = NULL;
        Bdt_TunnelHistoryTcpLogForRemoteEndpointVector* discardRemoteVector = NULL;
        Bdt_TunnelHistoryTcpLogVector* discardLogVector = &(discardRemoteVector->logsForRemote[0].logVector);
        struct sglib_tcp_log_map_iterator it;

        // 在discardRemoteVector找最早的discardLogVector
        for (discardMapNode = sglib_tcp_log_map_it_init(&it, history->tcpLogMap);
            discardMapNode != NULL;
            discardMapNode = sglib_tcp_log_map_it_next(&it))
        {
            discardRemoteVector = &discardMapNode->log4Local.logForRemoteVector;

            for (size_t i = 1; i < discardRemoteVector->size; i++) {
                Bdt_TunnelHistoryTcpLogVector* cur = &(discardRemoteVector->logsForRemote[i].logVector);
                if (cur->logs[0].timestamp < discardLogVector->logs[0].timestamp) {
                    discardLogVector = cur;
                }
            }
        }

        history->logCount--;
		discardLogVector->size--;
		BLOG_CHECK(discardLogVector->size >= 0);
		memcpy(discardLogVector->logs, discardLogVector->logs + 1, discardLogVector->size * sizeof(Bdt_TunnelHistoryTcpLog));

		if (discardLogVector->size == 0)
		{
            size_t discardPos = 
                ((uint8_t*)discardLogVector - (uint8_t*)(&discardRemoteVector->logsForRemote[0].logVector))
                / sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint);

            bfx_vector_tcp_tunnel_log_cleanup(discardLogVector);

            memcpy(discardRemoteVector->logsForRemote + discardPos,
                discardRemoteVector->logsForRemote + discardPos + 1,
                (discardRemoteVector->size - discardPos - 1) * sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint));
            discardRemoteVector->size--;
            BLOG_CHECK(discardRemoteVector->size >= 0);

            if (discardRemoteVector->size == 0)
            {
                sglib_tcp_log_map_delete(&history->tcpLogMap, discardMapNode);
                if (targetMapNode == NULL)
                {
                    // 没有匹配内存，就用新淘汰的，不需要重新分配
                    targetMapNode = discardMapNode;
                    targetMapNode->log4Local.local = *localEndpoint;
                    sglib_tcp_log_map_add(&history->tcpLogMap, targetMapNode);
                }
                else if (targetMapNode != discardMapNode)
                {
                    bfx_vector_tcp_tunnel_log_for_remote_cleanup(discardRemoteVector);
                    BFX_FREE(discardMapNode);
                }
            }
		}
	}

	if (targetMapNode == NULL)
	{
		targetMapNode = BFX_MALLOC_OBJ(TcpLogMapNode);
		memset(targetMapNode, 0, sizeof(TcpLogMapNode));
        bfx_vector_tcp_tunnel_log_for_remote_init(&targetMapNode->log4Local.logForRemoteVector);
		targetMapNode->log4Local.local = *localEndpoint;
		sglib_tcp_log_map_add(&history->tcpLogMap, targetMapNode);
	}

    if (log4Remote == NULL)
    {
        Bdt_TunnelHistoryTcpLogForRemoteEndpoint newLog4Remote;
        newLog4Remote.remote = *remoteEndpoint;
        bfx_vector_tcp_tunnel_log_init(&newLog4Remote.logVector);
        bfx_vector_tcp_tunnel_log_for_remote_push_back(&targetMapNode->log4Local.logForRemoteVector, newLog4Remote);
        log4Remote = targetMapNode->log4Local.logForRemoteVector.logsForRemote + targetMapNode->log4Local.logForRemoteVector.size - 1;
    }
    bfx_vector_tcp_tunnel_log_push_back(&log4Remote->logVector, *inoutLog);
    history->logCount++;

	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_TunnelHistoryAddTcpLog(
	Bdt_TunnelHistory* history,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type)
{
	bool merged = false;
	int64_t oldTimeStamp = 0;
	Bdt_TunnelHistoryTcpLog log = {
	    .type = type,
	    .times = 1,
	    .timestamp = BfxTimeGetNow(false)
	};

	{
		BfxRwLockWLock(&history->tcpLogLock);

		TunnelHistoryAddTcpLogImplNoLock(history, localEndpoint, remoteEndpoint, &log, &oldTimeStamp, &merged);
	
		BfxRwLockWUnlock(&history->tcpLogLock);
	}

	BdtHistory_UpdateTunnelHistoryTcpLog(
		history->updater->storage,
		&history->remotePeerid,
		localEndpoint,
		remoteEndpoint,
		type,
		log.times,
		log.timestamp,
		oldTimeStamp,
		!merged);
	return BFX_RESULT_SUCCESS;
}

// clone特定local-endpoint相关日志到一块连续内存，减少逐条日志分配内存的开销
static void CloneTcpLogForLocalEndpointInOnPiece(const Bdt_TunnelHistoryTcpLogForLocalEndpoint* src,
    Bdt_TunnelHistoryTcpLogForLocalEndpoint* dest)
{
    size_t logCount = 0;
    for (size_t i = 0; i < src->logForRemoteVector.size; i++)
    {
        logCount += src->logForRemoteVector.logsForRemote[i].logVector.size;
    }

    uint8_t* buffer = (uint8_t*)BFX_MALLOC(sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint) * src->logForRemoteVector.size +
        sizeof(Bdt_TunnelHistoryTcpLog) * logCount);

    Bdt_TunnelHistoryTcpLogForRemoteEndpoint* remoteVector = (Bdt_TunnelHistoryTcpLogForRemoteEndpoint*)buffer;
    Bdt_TunnelHistoryTcpLog* logVector =
        (Bdt_TunnelHistoryTcpLog*)(buffer + sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint) * src->logForRemoteVector.size);

    dest->local = src->local;
    dest->logForRemoteVector.size = dest->logForRemoteVector._allocSize = src->logForRemoteVector.size;
    dest->logForRemoteVector.logsForRemote = remoteVector;
    
    for (size_t j = 0, logi = 0; j < src->logForRemoteVector.size; j++)
    {
        Bdt_TunnelHistoryTcpLogForRemoteEndpoint* destRemote = dest->logForRemoteVector.logsForRemote + j;
        Bdt_TunnelHistoryTcpLogForRemoteEndpoint* srcRemote = src->logForRemoteVector.logsForRemote + j;
        destRemote->remote = srcRemote->remote;
        destRemote->logVector.logs = logVector + logi;
        destRemote->logVector.size =
            destRemote->logVector._allocSize =
            srcRemote->logVector.size;
        memcpy(destRemote->logVector.logs,
            srcRemote->logVector.logs,
            sizeof(Bdt_TunnelHistoryTcpLog) * srcRemote->logVector.size);
        logi += srcRemote->logVector.size;
    }
}

uint32_t Bdt_TunnelHistoryGetTcpLog(
	Bdt_TunnelHistory* history,
	const BdtEndpoint localEndpoint[],
    Bdt_TunnelHistoryTcpLogForLocalEndpoint outLogs[],
	size_t count)
{
	TcpLogMapNode toFind;

	{
		BfxRwLockRLock(&history->tcpLogLock);

		for (size_t ix = 0; ix < count; ++ix)
		{
			toFind.log4Local.local = localEndpoint[ix];
			TcpLogMapNode* foundNode = sglib_tcp_log_map_find_member(history->tcpLogMap, &toFind);
			if (foundNode != NULL)
			{
                CloneTcpLogForLocalEndpointInOnPiece(&foundNode->log4Local, outLogs + ix);
			}
            else
            {
                memset(outLogs + ix, 0, sizeof(Bdt_TunnelHistoryTcpLogForLocalEndpoint));
            }
		}

		BfxRwLockRUnlock(&history->tcpLogLock);
	}

    return BFX_RESULT_SUCCESS;
}

void Bdt_ReleaseTunnelHistoryTcpLogs(
    Bdt_TunnelHistoryTcpLogForLocalEndpoint logs[],
    size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (logs[i].logForRemoteVector.logsForRemote != NULL)
        {
            BFX_FREE(logs[i].logForRemoteVector.logsForRemote);
        }
    }

    memset(logs, 0, sizeof(Bdt_TunnelHistoryTcpLogForLocalEndpoint) * count);
}

uint32_t Bdt_TunnelHistoryOnPackageFromRemotePeer(
    Bdt_TunnelHistory* history, 
    const BdtPeerInfo* remotePeerInfo,
    const BdtEndpoint* remoteEndpoint,
    const BdtEndpoint* localEndpoint,
    uint32_t rto)
{

	BdtHistory_PeerConnectType type;
	if (localEndpoint->flag & BDT_ENDPOINT_PROTOCOL_UDP)
	{
		type = BDT_HISTORY_PEER_CONNECT_TYPE_UDP;
	}
	else if (!Bdt_IsTcpEndpointPassive(localEndpoint, remoteEndpoint))
	{
		type = BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT;
	}
	else
	{
		type = BDT_HISTORY_PEER_CONNECT_TYPE_TCP_REVERSE;
	}
    uint32_t result = BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
		history->updater, 
        &history->remotePeerid, 
		remotePeerInfo,
        remoteEndpoint,
        localEndpoint,
		type, 
        rto);

    if (history->peerHistory == NULL)
    {
        BfxUserData userData;
        TunnelHistoryAsUserData(history, &userData);

        BdtHistory_PeerUpdaterFindByPeerid(history->updater,
            &history->remotePeerid,
            true,
            LoadPeerHistoryCallback,
            &userData);
    }
    return result;
}

uint32_t Bdt_TunnelHistoryGetUdpLog(
    Bdt_TunnelHistory* history,
    const BdtEndpoint localEndpoint[],
    Bdt_TunnelHistoryUdpLogForLocalEndpoint outLogs[],
    size_t count
)
{
    if (history->peerHistory != NULL)
    {
        size_t totalConnectCount = history->peerHistory->connectVector.size;
        uint8_t* buffer = BFX_MALLOC(totalConnectCount *
            (sizeof(BdtHistory_PeerConnectInfo*) + sizeof(BdtHistory_PeerConnectInfo)));

        BdtHistory_PeerConnectInfo** pointerAddress = (BdtHistory_PeerConnectInfo**)buffer;
        BdtHistory_PeerConnectInfo* contentAddress = (BdtHistory_PeerConnectInfo*)(buffer + sizeof(BdtHistory_PeerConnectInfo*) * totalConnectCount);
        
        size_t nextPos = 0;

        for (size_t i = 0; i < count; i++)
        {
            outLogs[i].local = localEndpoint[i];
            outLogs[i].logForRemoteVector.connections = pointerAddress + nextPos;

            for (size_t j = 0; j < history->peerHistory->connectVector.size; j++)
            {
                BdtHistory_PeerConnectInfo* srcConnectInfo = history->peerHistory->connectVector.connections[j];
                if (BdtEndpointCompare(srcConnectInfo->localEndpoint, localEndpoint + i, true) == 0)
                {
                    pointerAddress[nextPos] = contentAddress + nextPos;
                    contentAddress[nextPos] = *srcConnectInfo;
                    outLogs[i].logForRemoteVector.size++;
                    nextPos++;
                }
            }
            outLogs[i].logForRemoteVector._allocsize = outLogs[i].logForRemoteVector.size;
        }
        
        if (nextPos == 0)
        {
            BFX_FREE(buffer);
            if (count > 0)
            {
                outLogs[0].logForRemoteVector.connections = NULL;
                outLogs[0].logForRemoteVector.size = outLogs[0].logForRemoteVector._allocsize = 0;
            }
        }
    }
    return BFX_RESULT_SUCCESS;
}

void Bdt_ReleaseTunnelHistoryUdpLogs(
    const Bdt_TunnelHistoryUdpLogForLocalEndpoint logs[],
    size_t count)
{
    if (count > 0 && logs[0].logForRemoteVector.connections != NULL)
    {
        BFX_FREE(logs[0].logForRemoteVector.connections);
    }
}

static void OnLoadOneLog4LocalEndpointNoLock(Bdt_TunnelHistory* history,
    const Bdt_TunnelHistoryTcpLogForLocalEndpoint* loadLog4Local)
{
    TcpLogMapNode toFind = {
        .log4Local = {
            .local = loadLog4Local->local
        }
    };

    TcpLogMapNode* targetNode = sglib_tcp_log_map_find_member(history->tcpLogMap, &toFind);
    if (targetNode == NULL)
    {
        targetNode = BFX_MALLOC_OBJ(TcpLogMapNode);
        targetNode->log4Local.local = loadLog4Local->local;
        bfx_vector_tcp_tunnel_log_for_remote_init(&targetNode->log4Local.logForRemoteVector);
    }

    // 1.先把新添加的对端地址关联数据挪到队列尾部，把从存储器加载的数据添加到队列头部
    size_t existRemoteEndpointSize = targetNode->log4Local.logForRemoteVector.size;
    bfx_vector_tcp_tunnel_log_for_remote_resize(&targetNode->log4Local.logForRemoteVector,
        existRemoteEndpointSize + loadLog4Local->logForRemoteVector.size);
    memcpy(targetNode->log4Local.logForRemoteVector.logsForRemote + loadLog4Local->logForRemoteVector.size,
        targetNode->log4Local.logForRemoteVector.logsForRemote,
        existRemoteEndpointSize * sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint));
    memcpy(targetNode->log4Local.logForRemoteVector.logsForRemote,
        loadLog4Local->logForRemoteVector.logsForRemote,
        loadLog4Local->logForRemoteVector.size * sizeof(Bdt_TunnelHistoryTcpLogForRemoteEndpoint));

    for (size_t ri = 0; ri < loadLog4Local->logForRemoteVector.size; ri++)
    {
        Bdt_TunnelHistoryTcpLogVector* logVector = &targetNode->log4Local.logForRemoteVector.logsForRemote[ri].logVector;
        bfx_vector_tcp_tunnel_log_init(logVector);
        bfx_vector_tcp_tunnel_log_clone(&loadLog4Local->logForRemoteVector.logsForRemote[ri].logVector, logVector);
    }

    // 2.再把新产生的对端地址信息合并到从存储器中加载数据的后面
    Bdt_TunnelHistoryTcpLogForRemoteEndpoint* newAddRemoteLog = targetNode->log4Local.logForRemoteVector.logsForRemote
        - existRemoteEndpointSize;
    bfx_vector_tcp_tunnel_log_for_remote_resize(&targetNode->log4Local.logForRemoteVector, loadLog4Local->logForRemoteVector.size);
    for (size_t nai = 0; nai < existRemoteEndpointSize; nai++, newAddRemoteLog++)
    {
        Bdt_TunnelHistoryTcpLogForRemoteEndpoint* foundRemoteLog = NULL;
        for (size_t naj = 0; naj < targetNode->log4Local.logForRemoteVector.size; naj++)
        {
            Bdt_TunnelHistoryTcpLogForRemoteEndpoint* loadRemoteLog = targetNode->log4Local.logForRemoteVector.logsForRemote + naj;
            if (BdtEndpointCompare(&loadRemoteLog->remote, &newAddRemoteLog->remote, true) == 0)
            {
                foundRemoteLog = loadRemoteLog;
                break;
            }
        }

        if (foundRemoteLog != NULL)
        {
            size_t loadLogCount = foundRemoteLog->logVector.size;
            bfx_vector_tcp_tunnel_log_resize(&foundRemoteLog->logVector, loadLogCount + newAddRemoteLog->logVector.size);
            memcpy(&foundRemoteLog->logVector.logs + loadLogCount,
                newAddRemoteLog->logVector.logs,
                newAddRemoteLog->logVector.size * sizeof(Bdt_TunnelHistoryTcpLog));
            bfx_vector_tcp_tunnel_log_cleanup(&newAddRemoteLog->logVector);
        }
        else
        {
            bfx_vector_tcp_tunnel_log_for_remote_push_back(&targetNode->log4Local.logForRemoteVector, *newAddRemoteLog);
        }
    }
}

static void LoadTcpTunnelHistoryFromStorageCallback(
	uint32_t errorCode,
	const BdtPeerid* remotePeerid,
	const Bdt_TunnelHistoryTcpLogForLocalEndpoint* loadLog4LocalArray,
	size_t endpointCount,
	void* userData
	)
{
	Bdt_TunnelHistory* history = (Bdt_TunnelHistory*)userData;

	if (errorCode != BFX_RESULT_SUCCESS)
	{
		BLOG_HEX(remotePeerid->pid, 15, "load tcp log from storage failed:%d.", errorCode);
		return;
	}

	{
		BfxRwLockWLock(&history->tcpLogLock);

		for (size_t epi = 0; epi < endpointCount; epi++)
		{
            OnLoadOneLog4LocalEndpointNoLock(history, loadLog4LocalArray + epi);
		}

		BfxRwLockWUnlock(&history->tcpLogLock);
	}

	history->loadCallback(history, history->loadCallbackUserData.userData);
}

static void LoadPeerHistoryCallback(const BdtHistory_PeerInfo* peerHistory, void* userData)
{
    Bdt_TunnelHistory* history = (Bdt_TunnelHistory*)userData;
    if (peerHistory != NULL && history->peerHistory == NULL)
    {
        BdtHistory_PeerHistoryAddRef(peerHistory);
        history->peerHistory = peerHistory;
    }
}

#endif //__BDT_TUNNEL_HISTORY_IMP_INL__
