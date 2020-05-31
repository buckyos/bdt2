
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in Module.c of history module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include "../Storage.h"
#include <BuckyFramework/thread/bucky_thread.h>
#include <sqlite3.h>
#include <SGLib/SGLib.h>

#define ENDPOINT_LENGTH_V6 sizeof(BdtEndpoint)
#define ENDPOINT_LENGTH_V4 (ENDPOINT_LENGTH_V6 - 12)
#define ENDPOINT_LENGTH(ep) (((ep)->flag & BDT_ENDPOINT_IP_VERSION_4) != 0 ? ENDPOINT_LENGTH_V4 : ENDPOINT_LENGTH_V6)

#if defined(BFX_OS_WIN)

#define FORMAT64D	"%I64d"
#define PathStringCopy wcsncpy
#define bdt_sqlite3_open sqlite3_open16

#else // posix

#define FORMAT64D	"%lld"
#define PathStringCopy strncpy
#define bdt_sqlite3_open sqlite3_open

#endif // posix

typedef enum SqliteIOType
{
	// 优先级:close>load>add>update
	SQLITE_IO_TYPE_CLOSE = 1,
	SQLITE_IO_TYPE_LOAD = 2, // 包括open/create-table-if-not-exist/load/clean
	SQLITE_IO_TYPE_ADD_PEER = 3,
	SQLITE_IO_TYPE_SET_CONST_INFO = 4,
	SQLITE_IO_TYPE_ADD_CONNECT = 5,
	SQLITE_IO_TYPE_UPDATE_CONNECT = 6,
	SQLITE_IO_TYPE_ADD_SUPER_NODE = 7,
	SQLITE_IO_TYPE_UPDATE_SUPER_NODE = 8,
	SQLITE_IO_TYPE_ADD_KEY = 9,
	SQLITE_IO_TYPE_UPDATE_KEY = 10,
    SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID = 11,
    SQLITE_IO_TYPE_LOAD_HISTORY_CONNECT_INFO_BY_PEERID = 12,
    SQLITE_IO_TYPE_LOAD_SN_CLIENT_MAP_BY_CLIENT = 13,
	SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG = 14,
	SQLITE_IO_TYPE_ADD_TUNNEL_HISTORY_TCP_LOG = 15,
	SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG = 16,

	SQLITE_IO_TYPE_COUNT
} SqliteIOType;

static const char* _sqliteStorageSqls[SQLITE_IO_TYPE_COUNT] = {
	NULL, // 空
	NULL,
	NULL,
	"INSERT INTO bdt_history_peer (peerid,continent,country,carrier,city,inner,deviceId,publicKeyType,publicKey,flag) "
		"VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10)",
    "UPDATE bdt_history_peer SET continent=?2,country=?3,carrier=?4,city=?5,inner=?6,deviceId=?7,publicKeyType=?8,publicKey=?9,flag=?10 "
        "WHERE peerid=?1",
    "INSERT INTO bdt_history_connection (peerid,endpoint,localEndpoint,flags,lastConnectTime,rto,sendSpeed,recvSpeed,foundTime) "
		"VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9)",
	"UPDATE bdt_history_connection SET flags=?4,lastConnectTime=?5,rto=?6,sendSpeed=?7,recvSpeed=?8 "
		"WHERE peerid=?1 AND endpoint=?2 AND localEndpoint=?3",
	"INSERT INTO bdt_history_sn (peeridSN,peeridClient,lastResponceTime) "
		"VALUES (?1,?2,?3)",
	"UPDATE bdt_history_sn SET lastResponceTime=?3 "
		"WHERE peeridSN=?1 AND peeridClient=?2",
	"INSERT INTO bdt_history_aesKey (peerid,aesKey,flags,lastAccessTime,expireTime) "
		"VALUES (?1,?2,?3,?4,?5)",
	"UPDATE bdt_history_aesKey SET flags=?3,lastAccessTime=?4,expireTime=?5 "
		"WHERE peerid=?1 AND aesKey=?2",
    "SELECT peerid,continent,country,carrier,city,inner,deviceId,publicKeyType,publicKey,flag "
        "FROM bdt_history_peer WHERE peerid=?1",
    "SELECT peerid,endpoint,localEndpoint,flags,foundTime,lastConnectTime,rto,sendSpeed,recvSpeed "
        "FROM bdt_history_connection WHERE peerid=?1 ORDER BY lastConnectTime",
    "SELECT peeridSN,lastResponceTime "
        "FROM bdt_history_sn WHERE peeridClient=?1 ORDER BY lastResponceTime DESC",
	"SELECT localEndpoint,remoteEndpoint,type,times,timestamp "
		"FROM bdt_history_tcpTunnelLog WHERE remotePeerid=?1 ORDER BY localEndpoint,remoteEndpoint,timestamp",
	"INSERT INTO bdt_history_tcpTunnelLog (remotePeerid,localEndpoint,remoteEndpoint,type,times,timestamp) "
		"VALUES (?1,?2,?3,?4,?5,?6)",
	"UPDATE bdt_history_tcpTunnelLog SET times=?5,timestamp=?6 "
		"WHERE remotePeerid=?1 AND localEndpoint=?2 AND remoteEndpoint=?3 AND type=?4 AND timestamp=?7",
};

typedef struct SqliteStorage SqliteStorage;

typedef struct SqliteIOQueueNode
{
	struct SqliteIOQueueNode* next;

	SqliteIOType type;
} SqliteIOQueueNode, io_node;

typedef struct SqliteIOQueue
{
	SqliteIOQueueNode* header;
	SqliteIOQueueNode* tail;
} SqliteIOQueue;

static void SqliteStorageIOQueuePush(SqliteIOQueue* queue, SqliteIOQueueNode* node)
{
	if (queue->tail == NULL)
	{
		queue->tail = node;
		queue->header = node;
		node->next = NULL;
	}
	else
	{
		queue->tail->next = node;
		queue->tail = node;
		node->next = NULL;
	}
}

static SqliteIOQueueNode* SqliteStorageIOQueuePop(SqliteIOQueue* queue)
{
	SqliteIOQueueNode* nextNode = NULL;
	if (queue->header != NULL)
	{
		nextNode = queue->header;
		queue->header = nextNode->next;
		if (queue->header == NULL)
		{
			queue->tail = NULL;
		}
	}
	return nextNode;
}

typedef struct SqliteStorage
{
	BdtStorage base;

	BfxPathCharType filePath[BFX_PATH_MAX];
	BFX_THREAD_HANDLE threadHandle;
	sqlite3* database;
	sqlite3_stmt* sqlStmt[SQLITE_IO_TYPE_COUNT];

	BfxThreadLock ioQueuelock; // for ioQueue(closeIONode/loadIONode/addIOQueueHeader/addIOQueue/updateIOQueue)
	SqliteIOQueueNode* closeIONode;
	SqliteIOQueueNode* loadIONode;
	SqliteIOQueue addIOQueue;
	SqliteIOQueue updateIOQueue;
	uint32_t ioCount;

	uint32_t pendingLimit;

	int transactionIOCount;
	int64_t transactionTime;
	int transactionDepth;
} SqliteStorage;

static bool SqlitePushIONode(SqliteStorage* storage, SqliteIOQueueNode* ioNode);
static SqliteIOQueueNode* SqlitePopIONode(SqliteStorage* storage);
static void SqliteDeleteIONode(SqliteIOQueueNode* ioNode);

static void SqliteInitSqliteStorage(SqliteStorage* sqliteStorage,
	const BfxPathCharType* filePath,
	uint32_t pendingLimit
	)
{
	memset(sqliteStorage, 0, sizeof(SqliteStorage));

	sqliteStorage->pendingLimit = pendingLimit;
	PathStringCopy(sqliteStorage->filePath, filePath, BFX_PATH_MAX);
	BfxThreadLockInit(&sqliteStorage->ioQueuelock);
}

static void SqliteUninitSqliteStorage(SqliteStorage* sqliteStorage)
{
	BfxThreadLockDestroy(&sqliteStorage->ioQueuelock);
}

static uint32_t SqliteCreateSqliteThread(SqliteStorage* sqliteStorage)
{
	BfxThreadOptions opt = {.name = "bdt.storage.sqlite", .stackSize = 0x40000, .ssize = sizeof(BfxThreadOptions)};
	int ret = BfxCreateIOThread(opt, &sqliteStorage->threadHandle);
	if (ret != 0)
	{
		BLOG_WARN("create thread for sqlite failed:%d.", ret);
		return BFX_RESULT_FAILED;
	}
	return BFX_RESULT_SUCCESS;
}

typedef struct SqliteLoadIOQueueNode
{
    SqliteIOQueueNode base;
    int64_t historyValidTime;
    BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback;
    BfxUserData userData;
} SqliteLoadIOQueueNode;

// 加载节点历史通信信息
static uint32_t SqliteLoadAll(
	SqliteStorage* storage,
	int64_t historyValidTime, // 历史记录最早保留时间
	BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback,
	const BfxUserData* userData
)
{
	BLOG_CHECK(storage != NULL);
    BLOG_CHECK(callback != NULL);

	SqliteLoadIOQueueNode* loadIONode = BFX_MALLOC_OBJ(SqliteLoadIOQueueNode);
	memset(loadIONode, 0, sizeof(SqliteLoadIOQueueNode));
	loadIONode->historyValidTime = historyValidTime;
	loadIONode->base.type = SQLITE_IO_TYPE_LOAD;
	loadIONode->callback = callback;
	if (userData != NULL && callback != NULL)
	{
		loadIONode->userData = *userData;
		if (userData->lpfnAddRefUserData != NULL)
		{
			userData->lpfnAddRefUserData(userData->userData);
		}
	}

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)loadIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)loadIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteLoadPeerHistoryByPeeridIOQueueNode
{
    SqliteIOQueueNode base;
    BdtPeerid peerid;
    int64_t historyValidTime;
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback;
    BfxUserData userData;
} SqliteLoadPeerHistoryByPeeridIOQueueNode;

static uint32_t SqliteLoadPeerHistoryByPeerid(
    SqliteStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // 历史记录最早保留时间
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback,
    const BfxUserData* userData
    )
{
    BLOG_CHECK(storage != NULL);
    BLOG_CHECK(peerid != NULL);
    BLOG_CHECK(callback != NULL);

    SqliteLoadPeerHistoryByPeeridIOQueueNode* loadIONode = BFX_MALLOC_OBJ(SqliteLoadPeerHistoryByPeeridIOQueueNode);
    memset(loadIONode, 0, sizeof(SqliteLoadPeerHistoryByPeeridIOQueueNode));
    loadIONode->peerid = *peerid;
    loadIONode->historyValidTime = historyValidTime;
    loadIONode->base.type = SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID;
    loadIONode->callback = callback;
    if (userData != NULL && callback != NULL)
    {
        loadIONode->userData = *userData;
        if (userData->lpfnAddRefUserData != NULL)
        {
            userData->lpfnAddRefUserData(userData->userData);
        }
    }

    if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)loadIONode))
    {
        SqliteDeleteIONode((SqliteIOQueueNode*)loadIONode);
        return BFX_RESULT_ALREADY_OPERATION;
    }
    return BFX_RESULT_PENDING;
}

static void SqliteCloneHistoryInfo(
	const BdtHistory_PeerInfo* inHistoryInfo,
	BdtHistory_PeerInfo* outHistoryInfo);

typedef struct SqliteAddPeerIOQueueNode
{
	SqliteIOQueueNode base;
	BdtHistory_PeerInfo peerHistory;
} SqliteAddPeerIOQueueNode;

// 增加一个节点信息
static uint32_t SqliteAddPeerHistory(
	SqliteStorage* storage,
	const BdtHistory_PeerInfo* history
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(history != NULL);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(history->peerid.pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	if (history == NULL)
	{
		BLOG_ERROR("attempt to add peer with null.");
		return BFX_RESULT_INVALID_PARAM;
	}

	SqliteAddPeerIOQueueNode* addIONode = BFX_MALLOC_OBJ(SqliteAddPeerIOQueueNode);
	memset(addIONode, 0, sizeof(SqliteAddPeerIOQueueNode));
	addIONode->base.type = SQLITE_IO_TYPE_ADD_PEER;
	SqliteCloneHistoryInfo(history, &addIONode->peerHistory);

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)addIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)addIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteSetConstInfoIOQueueNode
{
    SqliteIOQueueNode base;
    BdtPeerid peerid;
    uint32_t flags;
    BdtPeerConstInfo constInfo;
} SqliteSetConstInfoIOQueueNode;

static uint32_t SqliteSetConstInfo(
    SqliteStorage* storage,
    const BdtHistory_PeerInfo* history
    )
{
    BLOG_CHECK(storage != NULL);
    BLOG_CHECK(history != NULL);

    if (storage->ioCount > storage->pendingLimit)
    {
        BLOG_HEX(history->peerid.pid, 15, "sqlite busy.");
        return BFX_RESULT_BUSY;
    }

    if (history == NULL)
    {
        BLOG_ERROR("attempt to set const info with NULL.");
        return BFX_RESULT_INVALID_PARAM;
    }

    if (!history->flags.hasConst)
    {
        return BFX_RESULT_SUCCESS;
    }

    SqliteSetConstInfoIOQueueNode* setConstInfoIONode = BFX_MALLOC_OBJ(SqliteSetConstInfoIOQueueNode);
    memset(setConstInfoIONode, 0, sizeof(SqliteSetConstInfoIOQueueNode));
    setConstInfoIONode->base.type = SQLITE_IO_TYPE_SET_CONST_INFO;
    setConstInfoIONode->peerid = history->peerid;
    setConstInfoIONode->constInfo = history->constInfo;
    setConstInfoIONode->flags = history->flagsU32;

    if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)setConstInfoIONode))
    {
        SqliteDeleteIONode((SqliteIOQueueNode*)setConstInfoIONode);
        return BFX_RESULT_ALREADY_OPERATION;
    }
    return BFX_RESULT_PENDING;
}

typedef struct SqliteConnectInfoIOQueueNode
{
	SqliteIOQueueNode base;
	BdtPeerid peerid;
	BdtHistory_PeerConnectInfo connectInfo;
	BdtEndpoint localEndpoint;
} SqliteConnectInfoIOQueueNode;

// 更新某节点的一条连接信息
static uint32_t SqliteUpdateConnectHistory(
	SqliteStorage* storage,
	const BdtPeerid* peerid,
	const BdtHistory_PeerConnectInfo* connectInfo,
	bool isNew
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(peerid != NULL);
	BLOG_CHECK(connectInfo != NULL);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(peerid->pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	if (peerid == NULL ||
		connectInfo == NULL)
	{
		BLOG_HEX(peerid->pid, 15, "attempt to add connection without no.");
		return BFX_RESULT_INVALID_PARAM;
	}

	SqliteConnectInfoIOQueueNode* connectIONode = BFX_MALLOC_OBJ(SqliteConnectInfoIOQueueNode);
	memset(connectIONode, 0, sizeof(SqliteConnectInfoIOQueueNode));
	connectIONode->base.type = isNew? SQLITE_IO_TYPE_ADD_CONNECT : SQLITE_IO_TYPE_UPDATE_CONNECT;
	connectIONode->peerid = *peerid;
	connectIONode->connectInfo = *connectInfo;
	if (connectInfo->localEndpoint != NULL)
	{
		connectIONode->localEndpoint = *connectInfo->localEndpoint;
		connectIONode->connectInfo.localEndpoint = &connectIONode->localEndpoint;
	}
	else
	{
		connectIONode->connectInfo.localEndpoint = NULL;
	}

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)connectIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)connectIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteSuperNodeIOQueueNode
{
	SqliteIOQueueNode base;
	BdtPeerid snPeerid;
	BdtPeerid clientPeerid;
	int64_t responseTime;
} SqliteSuperNodeIOQueueNode;

// 更新在某SuperNode上查询到某节点的时间
static uint32_t SqliteUpdateSuperNodeHistory(
	SqliteStorage* storage,
	const BdtPeerid* clientPeerid,
	const BdtPeerid* superNodePeerid,
	int64_t responseTime,
	bool isNew
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(clientPeerid != NULL);
	BLOG_CHECK(superNodePeerid != NULL);
	BLOG_CHECK(responseTime > 0);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(clientPeerid->pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	if (clientPeerid == NULL ||
		superNodePeerid == NULL ||
		responseTime == 0)
	{
		BLOG_HEX(clientPeerid->pid, 15, "attempt to add sn without no.");
		return BFX_RESULT_INVALID_PARAM;
	}

	SqliteSuperNodeIOQueueNode* superNodeIONode = BFX_MALLOC_OBJ(SqliteSuperNodeIOQueueNode);
	memset(superNodeIONode, 0, sizeof(SqliteSuperNodeIOQueueNode));
	superNodeIONode->base.type = isNew ? SQLITE_IO_TYPE_ADD_SUPER_NODE : SQLITE_IO_TYPE_UPDATE_SUPER_NODE;
	superNodeIONode->snPeerid = *superNodePeerid;
	superNodeIONode->clientPeerid = *clientPeerid;
	superNodeIONode->responseTime = responseTime;

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)superNodeIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)superNodeIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteAESKeyIOQueueNode
{
	SqliteIOQueueNode base;
	BdtPeerid peerid;
	uint8_t aesKey[BDT_AES_KEY_LENGTH];
	int64_t lastAccessTime;
	int64_t expireTime;
	uint32_t flags;
} SqliteAESKeyIOQueueNode;

// 增加一个节点通信的AES密钥
static uint32_t SqliteUpdateAesKey(
	SqliteStorage* storage,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(peerid != NULL);
	BLOG_CHECK(aesKey != NULL);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(peerid->pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	if (aesKey == NULL)
	{
		BLOG_HEX(peerid->pid, 15, "attempt to add key without (aesKey == NULL).");
		return BFX_RESULT_INVALID_PARAM;
	}

	SqliteAESKeyIOQueueNode* addIONode = BFX_MALLOC_OBJ(SqliteAESKeyIOQueueNode);
	memset(addIONode, 0, sizeof(SqliteAESKeyIOQueueNode));
	addIONode->base.type = isNew? SQLITE_IO_TYPE_ADD_KEY : SQLITE_IO_TYPE_UPDATE_KEY;
	addIONode->peerid = *peerid;
	memcpy(addIONode->aesKey, aesKey, BDT_AES_KEY_LENGTH);
	addIONode->expireTime = expireTime;
	addIONode->flags = flags;
	addIONode->lastAccessTime = lastAccessTime;

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)addIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)addIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteLoadTunnelHistoryTcpLogIOQueueNode
{
	SqliteIOQueueNode base;
	BdtPeerid remotePeerid;
	uint32_t invalidTime;
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback;
	BfxUserData userData;
} SqliteLoadTunnelHistoryTcpLogIOQueueNode;

static uint32_t SqliteLoadTunnelHistoryTcpLog(
	SqliteStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(remotePeerid != NULL);
	BLOG_CHECK(callback != NULL);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(remotePeerid->pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	if (callback == NULL)
	{
		BLOG_HEX(remotePeerid->pid, 15, "callback == NULL.");
		return BFX_RESULT_INVALID_PARAM;
	}

	SqliteLoadTunnelHistoryTcpLogIOQueueNode* addIONode = BFX_MALLOC_OBJ(SqliteLoadTunnelHistoryTcpLogIOQueueNode);
	memset(addIONode, 0, sizeof(SqliteLoadTunnelHistoryTcpLogIOQueueNode));
	addIONode->base.type = SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG;
	addIONode->remotePeerid = *remotePeerid;
	addIONode->invalidTime = invalidTime;
	addIONode->callback = callback;
	if (userData != NULL)
	{
		addIONode->userData = *userData;
		if (userData->lpfnAddRefUserData != NULL)
		{
			userData->lpfnAddRefUserData(userData->userData);
		}
	}

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)addIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)addIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

typedef struct SqliteUpdateTunnelHistoryTcpLogIOQueueNode
{
	SqliteIOQueueNode base;
	BdtPeerid remotePeerid;
	BdtEndpoint localEndpoint;
	BdtEndpoint remoteEndpoint;
	Bdt_TunnelHistoryTcpLogType type;
	uint32_t times;
	int64_t timestamp;
	int64_t oldTimeStamp;
} SqliteUpdateTunnelHistoryTcpLogIOQueueNode;

// 更新/追加TCP连接日志
static uint32_t SqliteUpdateTunnelHistoryTcpLog(
	SqliteStorage* storage,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type,
	uint32_t times,
	int64_t timestamp,
	int64_t oldTimeStamp,
	bool isNew
)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(remotePeerid != NULL);
	BLOG_CHECK(localEndpoint != NULL);
	BLOG_CHECK(remoteEndpoint != NULL);

	if (storage->ioCount > storage->pendingLimit)
	{
		BLOG_HEX(remotePeerid->pid, 15, "sqlite busy.");
		return BFX_RESULT_BUSY;
	}

	SqliteUpdateTunnelHistoryTcpLogIOQueueNode* addIONode = BFX_MALLOC_OBJ(SqliteUpdateTunnelHistoryTcpLogIOQueueNode);
	memset(addIONode, 0, sizeof(SqliteUpdateTunnelHistoryTcpLogIOQueueNode));
	addIONode->base.type = isNew ? SQLITE_IO_TYPE_ADD_TUNNEL_HISTORY_TCP_LOG : SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG;
	addIONode->remotePeerid = *remotePeerid;
	addIONode->localEndpoint = *localEndpoint;
	addIONode->remoteEndpoint = *remoteEndpoint;
	addIONode->type = type;
	addIONode->times = times;
	addIONode->timestamp = timestamp;
	addIONode->oldTimeStamp = oldTimeStamp;

	if (!SqlitePushIONode(storage, (SqliteIOQueueNode*)addIONode))
	{
		SqliteDeleteIONode((SqliteIOQueueNode*)addIONode);
		return BFX_RESULT_ALREADY_OPERATION;
	}
	return BFX_RESULT_PENDING;
}

// 销毁BdtStorage对象
static void SqliteDestroySelf(SqliteStorage* storage)
{
	/*
	1.close
	2.PostExitMessage
	3.join
	4.free
	*/

	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(storage->threadHandle != NULL);

	if (storage->threadHandle == NULL)
	{
		return;
	}

	SqliteIOQueueNode* closeIONode = BFX_MALLOC_OBJ(SqliteIOQueueNode);
	closeIONode->type = SQLITE_IO_TYPE_CLOSE;
	if (!SqlitePushIONode(storage, closeIONode))
	{
		SqliteDeleteIONode(closeIONode);
	}

	BFX_THREAD_HANDLE threadHandle = storage->threadHandle;
	storage->threadHandle = NULL;

	BfxPostQuit(threadHandle, 0);
	BfxThreadJoin(threadHandle);
	BfxThreadRelease(threadHandle);

	SqliteIOQueueNode* abortIONode = NULL;
	while ((abortIONode = SqlitePopIONode(storage)) != NULL)
	{
		SqliteDeleteIONode(abortIONode);
	}

	SqliteUninitSqliteStorage(storage);
	BFX_FREE(storage);
}

BDT_API(BdtStorage*) BdtCreateSqliteStorage(const BfxPathCharType* filePath, uint32_t pendingLimit)
{
	SqliteStorage* sqliteStorage = BFX_MALLOC_OBJ(SqliteStorage);
	SqliteInitSqliteStorage(sqliteStorage, filePath, pendingLimit);

	uint32_t errorCode = SqliteCreateSqliteThread(sqliteStorage);
	if (errorCode != BFX_RESULT_SUCCESS)
	{
		BFX_FREE(sqliteStorage);
		return NULL;
	}

	BdtStorage* base = &sqliteStorage->base;

	base->loadAll = (BDT_HISTORY_STORAGE_PROC_LOAD_ALL)SqliteLoadAll;
	base->addPeerHistory = (BDT_HISTORY_STORAGE_PROC_ADD_PEER_HISTORY)SqliteAddPeerHistory;
	base->setPeerConstInfo = (BDT_HISTORY_STORAGE_PROC_SET_CONST_INFO)SqliteSetConstInfo;
	base->updateConnectHistory = (BDT_HISTORY_STORAGE_PROC_UPDATE_CONNECT_HISTORY)SqliteUpdateConnectHistory;
	base->updateSuperNodeHistory = (BDT_HISTORY_STORAGE_PROC_UPDATE_SUPERNODE_HISTORY)SqliteUpdateSuperNodeHistory;
	base->updateAesKey = (BDT_HISTORY_STORAGE_PROC_UPDATE_AES_KEY)SqliteUpdateAesKey;
    base->loadPeerHistoryByPeerid = (BDT_HISTORY_STORAGE_PROC_LOAD_PEER_HISTORY_BY_PEERID)SqliteLoadPeerHistoryByPeerid;
	base->loadTunnelHistoryTcpLog = (BDT_HISTORY_STORAGE_PROC_LOAD_TUNNEL_HISTORY_TCP_LOG)SqliteLoadTunnelHistoryTcpLog;
	base->updateTunnelHistoryTcpLog = (BDT_HISTORY_STORAGE_PROC_UPDATE_TUNNEL_HISTORY_TCP_LOG)SqliteUpdateTunnelHistoryTcpLog;
	base->destroy = (BDT_HISTORY_STORAGE_PROC_DESTROY_SELF)SqliteDestroySelf;

	return base;
}

static void SqliteNextIOProcessor(void* lpUserData);
static bool SqlitePushIONode(SqliteStorage* storage, SqliteIOQueueNode* ioNode)
{
	BLOG_CHECK(storage != NULL);
	BLOG_CHECK(ioNode != NULL);

	bool success = true;

	BfxThreadLockLock(&storage->ioQueuelock);

	if (ioNode->type == SQLITE_IO_TYPE_ADD_CONNECT ||
		ioNode->type == SQLITE_IO_TYPE_ADD_PEER ||
		ioNode->type == SQLITE_IO_TYPE_ADD_SUPER_NODE || 
		ioNode->type == SQLITE_IO_TYPE_ADD_KEY ||
		ioNode->type == SQLITE_IO_TYPE_ADD_TUNNEL_HISTORY_TCP_LOG)
	{
		SqliteStorageIOQueuePush(&storage->addIOQueue, ioNode);
	}
	else if (ioNode->type == SQLITE_IO_TYPE_UPDATE_CONNECT ||
		ioNode->type == SQLITE_IO_TYPE_UPDATE_KEY ||
		ioNode->type == SQLITE_IO_TYPE_UPDATE_SUPER_NODE ||
		ioNode->type == SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG ||
		ioNode->type == SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG ||
        ioNode->type == SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID)
	{
		SqliteStorageIOQueuePush(&storage->updateIOQueue, ioNode);
	}
	else if (ioNode->type == SQLITE_IO_TYPE_LOAD)
	{
		if (storage->loadIONode == NULL)
		{
			storage->loadIONode = ioNode;
		}
		else
		{
			success = false;
		}
	}
	else if (ioNode->type == SQLITE_IO_TYPE_CLOSE)
	{
		if (storage->closeIONode == NULL)
		{
			storage->closeIONode = ioNode;
		}
		else
		{
			success = false;
		}
	}
	else
	{
		// open在初始化的时候直接添加
		BLOG_CHECK_EX(false, "unknown io type(%d)", ioNode->type);
		success = false;
	}

	if (success)
	{
		storage->ioCount++;
	}

	BfxThreadLockUnlock(&storage->ioQueuelock);

	if (success)
	{
		BfxUserData ud = {.lpfnAddRefUserData = NULL, .lpfnReleaseUserData = NULL, .userData = storage};
		BfxPostMessage(storage->threadHandle, SqliteNextIOProcessor, ud);
	}

	return success;
}

// 按优先级获取下一个IO操作
static SqliteIOQueueNode* SqlitePopIONode(SqliteStorage* storage)
{
	BLOG_CHECK(storage != NULL);

	SqliteIOQueueNode* nextIONode = NULL;

	BfxThreadLockLock(&storage->ioQueuelock);

	if (storage->closeIONode != NULL)
	{
		nextIONode = storage->closeIONode;
		storage->closeIONode = NULL;
	}
	else if (storage->loadIONode != NULL)
	{
		nextIONode = storage->loadIONode;
		storage->loadIONode = NULL;
	}
	else if (storage->addIOQueue.header != NULL)
	{
		nextIONode = SqliteStorageIOQueuePop(&storage->addIOQueue);
	}
	else if (storage->updateIOQueue.header != NULL)
	{
		nextIONode = SqliteStorageIOQueuePop(&storage->updateIOQueue);
	}

	if (nextIONode != NULL)
	{
		storage->ioCount--;
		BLOG_CHECK(storage->ioCount >= 0);
	}
	BfxThreadLockUnlock(&storage->ioQueuelock);

	return nextIONode;
}

static void SqliteLoadIOProcessor(SqliteStorage* storage, SqliteLoadIOQueueNode* loadIO);
static void SqliteLoadPeerHistoryByPeeridIOProcessor(SqliteStorage* storage, SqliteLoadPeerHistoryByPeeridIOQueueNode* nextIONode);
static void SqliteAddPeerIOProcessor(SqliteStorage* storage, SqliteAddPeerIOQueueNode* addPeerIO);
static void SqliteSetConstInfoIOProcessor(SqliteStorage* storage, SqliteSetConstInfoIOQueueNode* setConstInfoIO);
static void SqliteUpdateConnectIOProcessor(SqliteStorage* storage, SqliteConnectInfoIOQueueNode* connectIO);
static void SqliteUpdateSuperNodeIOProcessor(SqliteStorage* storage, SqliteSuperNodeIOQueueNode* superNodeIO);
static void SqliteUpdateAesKeyIOProcessor(SqliteStorage* storage, SqliteAESKeyIOQueueNode* aesKeyIO);
static void SqliteLoadTunnelHistoryTcpLogIOProcessor(SqliteStorage* storage, SqliteLoadTunnelHistoryTcpLogIOQueueNode* loadTunnelTcpLog);
static void SqliteUpdateTunnelHistoryTcpLogIOProcessor(SqliteStorage* storage, SqliteUpdateTunnelHistoryTcpLogIOQueueNode* updateTunnelTcpLog);
static void SqliteCloseIOProcessor(SqliteStorage* storage);
static int SqliteBeginTransaction(SqliteStorage* storage);
static void SqliteCommitTransaction(SqliteStorage* storage);

static void SqliteNextIOProcessor(void* lpUserData)
{
	SqliteStorage* storage = (SqliteStorage*)lpUserData;

	SqliteIOQueueNode* nextIONode = SqlitePopIONode(storage);
	if (nextIONode == NULL)
	{
		return;
	}

	// 多余1个IO，启动事务
	if (storage->ioCount > 1 &&
		storage->transactionIOCount == 0 &&
		storage->database != NULL)
	{
		if (SqliteBeginTransaction(storage) == SQLITE_OK)
		{
			storage->transactionIOCount = 1;
			storage->transactionTime = BfxTimeGetNow(false);
		}
	}
	else if (storage->transactionIOCount > 0)
	{
		storage->transactionIOCount++;
	}

	if (nextIONode->type == SQLITE_IO_TYPE_ADD_PEER)
	{
		SqliteAddPeerIOProcessor(storage, (SqliteAddPeerIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_SET_CONST_INFO)
	{
		SqliteSetConstInfoIOProcessor(storage, (SqliteSetConstInfoIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_UPDATE_CONNECT ||
		nextIONode->type == SQLITE_IO_TYPE_ADD_CONNECT)
	{
		SqliteUpdateConnectIOProcessor(storage, (SqliteConnectInfoIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_UPDATE_SUPER_NODE ||
		nextIONode->type == SQLITE_IO_TYPE_ADD_SUPER_NODE)
	{
		SqliteUpdateSuperNodeIOProcessor(storage, (SqliteSuperNodeIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_UPDATE_KEY ||
		nextIONode->type == SQLITE_IO_TYPE_ADD_KEY)
	{
		SqliteUpdateAesKeyIOProcessor(storage, (SqliteAESKeyIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_ADD_TUNNEL_HISTORY_TCP_LOG ||
		nextIONode->type == SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG)
	{
		SqliteUpdateTunnelHistoryTcpLogIOProcessor(storage, (SqliteUpdateTunnelHistoryTcpLogIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG)
	{
		SqliteLoadTunnelHistoryTcpLogIOProcessor(storage, (SqliteLoadTunnelHistoryTcpLogIOQueueNode*)nextIONode);
	}
    else if (nextIONode->type == SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID)
    {
        SqliteLoadPeerHistoryByPeeridIOProcessor(storage, (SqliteLoadPeerHistoryByPeeridIOQueueNode*)nextIONode);
    }
	else if (nextIONode->type == SQLITE_IO_TYPE_LOAD)
	{
		SqliteLoadIOProcessor(storage, (SqliteLoadIOQueueNode*)nextIONode);
	}
	else if (nextIONode->type == SQLITE_IO_TYPE_CLOSE)
	{
		SqliteCloseIOProcessor(storage);
	}
	else
	{
		BLOG_CHECK_EX(false, "unknown io type(%d).", nextIONode->type);
	}

	SqliteDeleteIONode(nextIONode);

	if (storage->transactionIOCount > 0)
	{
		// io队列空/事务数量超过100/事务持续时间超过10S
		if (storage->ioCount == 0 ||
			storage->transactionIOCount >= 100 ||
			BfxTimeGetNow(false) - storage->transactionTime >= 10000000)
		{
			SqliteCommitTransaction(storage);
			storage->transactionIOCount = 0;
			storage->transactionTime = 0;
		}
	}
}

typedef struct SqlitePeerHistoryPeeridMapNode
{
	struct SqlitePeerHistoryPeeridMapNode* left;
	struct SqlitePeerHistoryPeeridMapNode* right;
	int color;

	const BdtPeerid* peerid; // key
	BdtHistory_SuperNodeInfo* peerHistory; // value
} SqlitePeerHistoryPeeridMapNode;

typedef SqlitePeerHistoryPeeridMapNode sqlite_peerid_history_map;
static int SqliteStoragePeeridHistoryMapCompareor(const SqlitePeerHistoryPeeridMapNode* left,
	const SqlitePeerHistoryPeeridMapNode* right)
{
	return memcmp(left->peerid, right->peerid, sizeof(BdtPeerid));
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(sqlite_peerid_history_map, left, right, color, SqliteStoragePeeridHistoryMapCompareor)
SGLIB_DEFINE_RBTREE_FUNCTIONS(sqlite_peerid_history_map, left, right, color, SqliteStoragePeeridHistoryMapCompareor)

typedef struct SqliteKeyInfoVector
{
	BdtHistory_KeyInfo** keys;
	size_t size;
	size_t _allocsize;
} SqliteKeyInfoVector, sqlite_key_info;
BFX_VECTOR_DEFINE_FUNCTIONS(sqlite_key_info, BdtHistory_KeyInfo*, keys, size, _allocsize)

static uint32_t SqlitePrepareDataBase(SqliteStorage* storage);
static uint32_t SqliteLoadAllTable(SqliteStorage* storage,
	int64_t historyValidTime,
	int64_t keyValidTime,
	BdtHistory_PeerSuperNodeInfoVector* outPeerHistoryVector,
	SqliteKeyInfoVector* outKeyInfoVector);
static uint32_t SqliteCleanExpireRecode(SqliteStorage* storage,
	int64_t historyValidTime,
	int64_t keyValidTime);
static void SqliteLoadIOProcessor(SqliteStorage* storage, SqliteLoadIOQueueNode* loadIO)
{
	/*
	1.open and create table if not exist
	2.load all
	3.callback
	4.async(clean)
	*/
	if (storage->database != NULL)
	{
		BLOG_WARN("should not load data again.");
		loadIO->callback(BFX_RESULT_ALREADY_OPERATION, NULL, 0, NULL, 0, loadIO->userData.userData);
		return;
	}

	uint32_t result = SqlitePrepareDataBase(storage);
	if (result != BFX_RESULT_SUCCESS)
	{
		SqliteCloseIOProcessor(storage);
		BLOG_WARN("sqlite prepare failed(%u).", result);
		loadIO->callback(result, NULL, 0, NULL, 0, loadIO->userData.userData);
		return;
	}

	int64_t now = BfxTimeGetNow(false) / 1000000;
	BdtHistory_PeerSuperNodeInfoVector peerHistoryVector;
	SqliteKeyInfoVector keyInfoVector;

	bfx_vector_super_node_info_init(&peerHistoryVector);
	bfx_vector_sqlite_key_info_init(&keyInfoVector);

	result = SqliteLoadAllTable(storage,
		loadIO->historyValidTime,
		now,
		&peerHistoryVector,
		&keyInfoVector);
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("sqlite load failed(%u)", result);
		loadIO->callback(result, NULL, 0, NULL, 0, loadIO->userData.userData);
	}
	else
	{
		loadIO->callback(BFX_RESULT_SUCCESS,
			(BdtHistory_PeerInfo**)peerHistoryVector.snHistorys, peerHistoryVector.size,
			keyInfoVector.keys, keyInfoVector.size,
			loadIO->userData.userData
		);
	}

	for (size_t pi = 0; pi < peerHistoryVector.size; pi++)
	{
		BdtHistory_PeerInfo* peerHistory = &peerHistoryVector.snHistorys[pi]->peerInfo;
		for (size_t ci = 0; ci < peerHistory->connectVector.size; ci++)
		{
			BFX_FREE(peerHistory->connectVector.connections[ci]);
		}
		bfx_vector_history_connect_cleanup(&peerHistory->connectVector);
		bfx_vector_super_node_info_cleanup(&peerHistory->snVector);
		BFX_FREE(peerHistory);
	}

	for (size_t ki = 0; ki < keyInfoVector.size; ki++)
	{
		BFX_FREE(keyInfoVector.keys[ki]);
	}
	bfx_vector_super_node_info_cleanup(&peerHistoryVector);
	bfx_vector_sqlite_key_info_cleanup(&keyInfoVector);

	SqliteCleanExpireRecode(storage, loadIO->historyValidTime, now);
	return;
}

static uint32_t SqlitePrepareDataBase(SqliteStorage* storage)
{
	// open and create table if not exist
	int result = bdt_sqlite3_open(storage->filePath, &storage->database);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite open failed:%d(%s).", result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}
	BLOG_INFO("sqlite(%s) open success.", storage->filePath);

	// 不产生journal文件
	result = sqlite3_exec(storage->database, "PRAGMA journal_mode = MEMORY", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		// 这个操作失败了不关键，可以继续执行
		BLOG_WARN("sqlite(PRAGMA journal_mode = PERSIST) exec failed:%d(%s).", result, sqlite3_errstr(result));
	}

	if (SqliteBeginTransaction(storage) == SQLITE_OK)
	{
		storage->transactionIOCount = 1;
		storage->transactionTime = BfxTimeGetNow(false);
	}

	const char* sqls[] = {
		"CREATE TABLE IF NOT EXISTS bdt_history_peer ("
			"peerid BLOB UNIQUE NOT NULL PRIMARY KEY,"
            "flag INT,"
			"continent TINYINT,"
			"country TINYINT,"
			"carrier TINYINT,"
			"city SMALLINT,"
			"inner TINYINT,"
			"deviceId BLOB,"
			"publicKeyType TINYINT,"
			"publicKey BLOB"
			")",
		"CREATE TABLE IF NOT EXISTS bdt_history_connection ("
			"peerid BLOB,"
			"endpoint BLOB,"
			"localEndpoint BLOB,"
			"flags INT,"
			"foundTime BIGINT,"
			"lastConnectTime BIGINT,"
			"rto MEDIUMINT,"
			"sendSpeed MEDIUMINT,"
			"recvSpeed MEDIUMINT,"
			"PRIMARY KEY(peerid, endpoint, localEndpoint)"
			")",
		"CREATE TABLE IF NOT EXISTS bdt_history_sn ("
			"peeridSN BLOB,"
			"peeridClient BLOB,"
			"lastResponceTime BIGINT,"
			"PRIMARY KEY(peeridSN, peeridClient)"
			")",
		"CREATE TABLE IF NOT EXISTS bdt_history_aesKey ("
			"peerid BLOB,"
			"aesKey BLOB,"
			"flags INT,"
			"lastAccessTime BIGINT,"
			"expireTime BIGINT,"
			"PRIMARY KEY(peerid, aesKey)"
			")",
		"CREATE TABLE IF NOT EXISTS bdt_history_tcpTunnelLog ("
			"remotePeerid BLOB,"
			"localEndpoint BLOB,"
			"remoteEndpoint BLOB,"
			"type SMALLINT,"
			"times INT,"
			"timestamp BIGINT"
			")",
		"CREATE INDEX IF NOT EXISTS bdt_history_index_tcpTunnelLog "
			"ON bdt_history_tcpTunnelLog (remotePeerid)"
	};

	for (int i = 0; i < BFX_ARRAYSIZE(sqls); i++)
	{
		result = sqlite3_exec(storage->database, sqls[i], NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sqls[i], result, sqlite3_errstr(result));
			return BFX_RESULT_FAILED;
		}
	}

	for (int i = 0; i < BFX_ARRAYSIZE(_sqliteStorageSqls); i++)
	{
		if (_sqliteStorageSqls[i] != NULL)
		{
			const char* sqlTail = NULL;
			result = sqlite3_prepare(storage->database,
				_sqliteStorageSqls[i],
				-1,
				storage->sqlStmt + i,
				&sqlTail);

			if (result != SQLITE_OK)
			{
				BLOG_WARN("sqlite(%s) prepare failed:%d(%s).", _sqliteStorageSqls[i], result, sqlite3_errstr(result));
				return BFX_RESULT_FAILED;
			}
		}
		else
		{
			storage->sqlStmt[i] = NULL;
		}
	}
	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteLoadPeers(SqliteStorage* storage,
	BdtHistory_PeerSuperNodeInfoVector* outPeerHistoryVector);
static void SqliteMakeMapByPeerId(BdtHistory_PeerSuperNodeInfoVector* peerHistoryVector,
	SqlitePeerHistoryPeeridMapNode** tree);
static uint32_t SqliteLoadConnections(SqliteStorage* storage,
	int64_t validTime,
	SqlitePeerHistoryPeeridMapNode* historyTree);
static uint32_t SqliteLoadSuperNodes(SqliteStorage* storage,
	SqlitePeerHistoryPeeridMapNode* historyTree);
static uint32_t SqliteLoadAesKeys(SqliteStorage* storage,
	SqlitePeerHistoryPeeridMapNode* historyTree,
	int64_t expireTime,
	SqliteKeyInfoVector* outKeyInfoVector);
static uint32_t SqliteLoadAllTable(SqliteStorage* storage,
	int64_t historyValidTime,
	int64_t keyValidTime,
	BdtHistory_PeerSuperNodeInfoVector* outPeerHistoryVector,
	SqliteKeyInfoVector* outKeyInfoVector)
{
	uint32_t result = SqliteLoadPeers(storage, outPeerHistoryVector);
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("load peers failed.");
		return result;
	}

	SqlitePeerHistoryPeeridMapNode* tree = NULL;
	SqliteMakeMapByPeerId(outPeerHistoryVector, &tree);

	do
	{
		result = SqliteLoadConnections(storage, historyValidTime, tree);
		if (result != BFX_RESULT_SUCCESS)
		{
			BLOG_WARN("load connections failed.");
			break;
		}

		result = SqliteLoadSuperNodes(storage, tree);
		if (result != BFX_RESULT_SUCCESS)
		{
			BLOG_WARN("load super-nodes failed.");
			break;
		}

		result = SqliteLoadAesKeys(storage, tree, keyValidTime, outKeyInfoVector);
		if (result != BFX_RESULT_SUCCESS)
		{
			BLOG_WARN("load aes-keys failed.");
			break;
		}
	} while (false);

	SqlitePeerHistoryPeeridMapNode* node = NULL;
	while ((node = tree) != NULL)
	{
		sglib_sqlite_peerid_history_map_delete(&tree, node);
		BFX_FREE(node);
	}
	return result;
}

static uint32_t SqliteFillPeerInfoFromDataBase(sqlite3_stmt* stmt, BdtHistory_PeerInfo* history);
static uint32_t SqliteLoadPeers(SqliteStorage* storage,
	BdtHistory_PeerSuperNodeInfoVector* outPeerHistoryVector)
{
	const char* sql = "SELECT peerid,"
		"continent,country,carrier,city,inner,deviceId,publicKeyType,publicKey,flag "
		"FROM bdt_history_peer";

	sqlite3_stmt* stmt = NULL;
	const char* sqlTail = NULL;
	int result = sqlite3_prepare_v2(storage->database, sql, -1, &stmt, &sqlTail);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}
	BLOG_CHECK(stmt != NULL);

	bfx_vector_super_node_info_reserve(outPeerHistoryVector, 10240);

	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		// fill
		BdtHistory_SuperNodeInfo* history = BFX_MALLOC_OBJ(BdtHistory_SuperNodeInfo);
		memset(history, 0, sizeof(BdtHistory_SuperNodeInfo));
		bfx_vector_history_connect_init(&history->peerInfo.connectVector);
		bfx_vector_super_node_info_init(&history->peerInfo.snVector);

		if (SqliteFillPeerInfoFromDataBase(stmt, &history->peerInfo) == BFX_RESULT_SUCCESS)
		{
			history->peerInfo.flags.isStorage = 1;
			bfx_vector_super_node_info_push_back(outPeerHistoryVector, history);
		}
		else
		{
			BFX_FREE(history);
		}
	}

	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite load peers failed:%d(%s)", result, sqlite3_errstr(result));

	result = sqlite3_reset(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load peers(reset) failed:%d(%s)", result, sqlite3_errstr(result));
	result = sqlite3_finalize(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load peers(finalize) failed:%d(%s)", result, sqlite3_errstr(result));
	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteFillPeerInfoFromDataBase(sqlite3_stmt* stmt, BdtHistory_PeerInfo* peerHistory)
{
	int columnBytes = 0;

	columnBytes = sqlite3_column_bytes(stmt, 0);
	if (columnBytes != BDT_PEERID_LENGTH)
	{
		BLOG_WARN("peerid size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
    peerHistory->peerid = *((const BdtPeerid*)sqlite3_column_blob(stmt, 0));
	
	BdtPeerArea* area = &peerHistory->constInfo.areaCode;
	area->continent = (uint8_t)sqlite3_column_int(stmt, 1);
	area->country = (uint8_t)sqlite3_column_int(stmt, 2);
	area->carrier = (uint8_t)sqlite3_column_int(stmt, 3);
	area->city = (uint16_t)sqlite3_column_int(stmt, 4);
	area->inner = (uint8_t)sqlite3_column_int(stmt, 5);

	columnBytes = sqlite3_column_bytes(stmt, 6);
	if (columnBytes > BDT_MAX_DEVICE_ID_LENGTH)
	{
		BLOG_WARN("deviceid size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
	memcpy(peerHistory->constInfo.deviceId, sqlite3_column_blob(stmt, 6), columnBytes);

    peerHistory->constInfo.publicKeyType = (uint8_t)sqlite3_column_int(stmt, 7);

	columnBytes = sqlite3_column_bytes(stmt, 8);
	if (columnBytes > sizeof(peerHistory->constInfo.publicKey))
	{
		BLOG_WARN("publickey size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
	memcpy(&peerHistory->constInfo.publicKey, sqlite3_column_blob(stmt, 8), columnBytes);

    union {
        BdtHistory_PeerInfoFlag flags;
        uint32_t flagsU32;
    } flag;

    flag.flagsU32 = (uint32_t)sqlite3_column_int(stmt, 9);
    peerHistory->flags.hasConst = flag.flags.hasConst;

	return BFX_RESULT_SUCCESS;
}

#define RBTREE_COLOR_INVALID (-1)
static void SqliteMakeMapByPeerId(BdtHistory_PeerSuperNodeInfoVector* peerHistoryVector,
	SqlitePeerHistoryPeeridMapNode** tree)
{
	for (size_t i = 0; i < peerHistoryVector->size; i++)
	{
		SqlitePeerHistoryPeeridMapNode* node = BFX_MALLOC_OBJ(SqlitePeerHistoryPeeridMapNode);
		node->peerHistory = peerHistoryVector->snHistorys[i];
		node->peerid = &node->peerHistory->peerInfo.peerid;
		node->left = node->right = NULL;
		node->color = RBTREE_COLOR_INVALID;

		sglib_sqlite_peerid_history_map_add(tree, node);
		if (node->color == RBTREE_COLOR_INVALID)
		{
			BFX_FREE(node);
		}
	}
}

static BdtHistory_PeerConnectInfo* SqliteCreateConnectInfo()
{
	BdtHistory_PeerConnectInfo* connectInfo = BFX_MALLOC(sizeof(BdtHistory_PeerConnectInfo) + sizeof(BdtEndpoint));
	memset(connectInfo, 0, sizeof(BdtHistory_PeerConnectInfo) + sizeof(BdtEndpoint));
	connectInfo->localEndpoint = (BdtEndpoint*)(connectInfo + 1);
	return connectInfo;
}

static uint32_t SqliteFillConnectInfoFromDataBase(sqlite3_stmt* stmt,
	BdtHistory_PeerConnectInfo* connectInfo);
static uint32_t SqliteLoadConnections(SqliteStorage* storage,
	int64_t validTime,
	SqlitePeerHistoryPeeridMapNode* historyTree)
{
	const char* sql = "SELECT peerid,endpoint,localEndpoint,"
		"flags,foundTime,lastConnectTime,rto,sendSpeed,recvSpeed "
		"FROM bdt_history_connection";

	sqlite3_stmt* stmt = NULL;
	const char* sqlTail = NULL;
	int result = sqlite3_prepare_v2(storage->database, sql, -1, &stmt, &sqlTail);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}
	BLOG_CHECK(stmt != NULL);

	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		// fill
		size_t peeridSize = sqlite3_column_bytes(stmt, 0);
		if (peeridSize != BDT_PEERID_LENGTH)
		{
			BLOG_WARN("peerid size(%d) is invalid.", peeridSize);
            continue;
		}
		const BdtPeerid* peerid = (const BdtPeerid*)sqlite3_column_blob(stmt, 0);
		SqlitePeerHistoryPeeridMapNode targetNode = { .peerid = peerid };

		SqlitePeerHistoryPeeridMapNode* foundNode = sglib_sqlite_peerid_history_map_find_member(historyTree, &targetNode);

		if (foundNode != NULL)
		{
			BdtHistory_PeerConnectInfo* connectInfo = SqliteCreateConnectInfo();
			if (SqliteFillConnectInfoFromDataBase(stmt, connectInfo) == BFX_RESULT_SUCCESS &&
				(connectInfo->lastConnectTime > validTime || (connectInfo->lastConnectTime == 0 && connectInfo->foundTime > validTime)))
			{
				bfx_vector_history_connect_push_back(&foundNode->peerHistory->peerInfo.connectVector, connectInfo);
			}
			else
			{
				BFX_FREE(connectInfo);
			}
		}
	}

	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite load connects failed:%d(%s)", result, sqlite3_errstr(result));

	result = sqlite3_reset(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load connects(reset) failed:%d(%s)", result, sqlite3_errstr(result));
	result = sqlite3_finalize(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load connects(finalize) failed:%d(%s)", result, sqlite3_errstr(result));
	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteFillConnectInfoFromDataBase(sqlite3_stmt* stmt, BdtHistory_PeerConnectInfo* connectInfo)
{
	int columnBytes = 0;

	columnBytes = sqlite3_column_bytes(stmt, 1);
	if (columnBytes < ENDPOINT_LENGTH_V4 || columnBytes > ENDPOINT_LENGTH_V6)
	{
		BLOG_WARN("endpoint size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
	memcpy(&connectInfo->endpoint, sqlite3_column_blob(stmt, 1), columnBytes);

	columnBytes = sqlite3_column_bytes(stmt, 2);
	if (columnBytes != 0 && columnBytes != ENDPOINT_LENGTH_V4 && columnBytes != ENDPOINT_LENGTH_V6)
	{
		BLOG_WARN("local endpoint size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
	if (columnBytes > 0)
	{
		memcpy((BdtEndpoint*)connectInfo->localEndpoint, sqlite3_column_blob(stmt, 2), columnBytes);
		connectInfo->localEndpoint = NULL;
	}

	connectInfo->flags.u32 = (uint32_t)sqlite3_column_int(stmt, 3);
	connectInfo->foundTime = sqlite3_column_int64(stmt, 4);
	connectInfo->lastConnectTime = sqlite3_column_int64(stmt, 5);
	connectInfo->rto = (uint32_t)sqlite3_column_int(stmt, 6);
	connectInfo->sendSpeed = (uint32_t)sqlite3_column_int(stmt, 7);
	connectInfo->recvSpeed = (uint32_t)sqlite3_column_int(stmt, 8);

	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteLoadSuperNodes(SqliteStorage* storage,
	SqlitePeerHistoryPeeridMapNode* historyTree)
{
	const char* sql = "SELECT peeridSN,peeridClient,lastResponceTime "
		"FROM bdt_history_sn";

	sqlite3_stmt* stmt = NULL;
	const char* sqlTail = NULL;
	int result = sqlite3_prepare_v2(storage->database, sql, -1, &stmt, &sqlTail);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}
	BLOG_CHECK(stmt != NULL);

	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		// fill
		size_t peeridSize = sqlite3_column_bytes(stmt, 0);
		if (peeridSize != BDT_PEERID_LENGTH)
		{
			BLOG_WARN("sn-peerid size(%d) is invalid.", peeridSize);
			continue;
		}
		peeridSize = sqlite3_column_bytes(stmt, 1);
		if (peeridSize != BDT_PEERID_LENGTH)
		{
			BLOG_WARN("client-peerid size(%d) is invalid.", peeridSize);
			continue;
		}

		const BdtPeerid* snPeerid = (const BdtPeerid*)sqlite3_column_blob(stmt, 0);
		SqlitePeerHistoryPeeridMapNode targetSNNode = { .peerid = snPeerid };
		const BdtPeerid* clientPeerid = (const BdtPeerid*)sqlite3_column_blob(stmt, 1);
		SqlitePeerHistoryPeeridMapNode targetClientNode = { .peerid = clientPeerid };

		SqlitePeerHistoryPeeridMapNode* foundSnNode = sglib_sqlite_peerid_history_map_find_member(historyTree, &targetSNNode);
		SqlitePeerHistoryPeeridMapNode* foundClientNode = sglib_sqlite_peerid_history_map_find_member(historyTree, &targetClientNode);

		if (foundSnNode != NULL && foundClientNode != NULL)
		{
			foundSnNode->peerHistory->lastResponseTime = sqlite3_column_int64(stmt, 2);
			foundSnNode->peerHistory->peerInfo.flags.isSn = 1;
			bfx_vector_super_node_info_push_back(&foundClientNode->peerHistory->peerInfo.snVector,
				foundSnNode->peerHistory);
		}
	}

	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite load sn failed:%d(%s)", result, sqlite3_errstr(result));

	result = sqlite3_reset(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load sn(reset) failed:%d(%s)", result, sqlite3_errstr(result));
	result = sqlite3_finalize(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load sn(finalize) failed:%d(%s)", result, sqlite3_errstr(result));
	return BFX_RESULT_SUCCESS;

}

static uint32_t SqliteFillKeyInfoFromDataBase(sqlite3_stmt* stmt, BdtHistory_KeyInfo* keyInfo);
static uint32_t SqliteLoadAesKeys(SqliteStorage* storage,
	SqlitePeerHistoryPeeridMapNode* historyTree,
	int64_t expireTime,
	SqliteKeyInfoVector* outKeyInfoVector)
{
	const char* sql = "SELECT peerid,aesKey,flags,lastAccessTime,expireTime "
		"FROM bdt_history_aesKey";

	sqlite3_stmt* stmt = NULL;
	const char* sqlTail = NULL;
	int result = sqlite3_prepare_v2(storage->database, sql, -1, &stmt, &sqlTail);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}
	BLOG_CHECK(stmt != NULL);

	bfx_vector_sqlite_key_info_reserve(outKeyInfoVector, 10240);

	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		// fill
		size_t peeridSize = sqlite3_column_bytes(stmt, 0);
		if (peeridSize != BDT_PEERID_LENGTH)
		{
			BLOG_WARN("peerid size(%d) is invalid.", peeridSize);
			continue;
		}

		const BdtPeerid* peerid = (const BdtPeerid*)sqlite3_column_blob(stmt, 0);
		SqlitePeerHistoryPeeridMapNode targetNode = { .peerid = peerid };

		SqlitePeerHistoryPeeridMapNode* foundNode = sglib_sqlite_peerid_history_map_find_member(historyTree, &targetNode);

		if (foundNode != NULL)
		{
			BdtHistory_KeyInfo* keyInfo = BFX_MALLOC_OBJ(BdtHistory_KeyInfo);
			keyInfo->peerid = foundNode->peerid;
			if (SqliteFillKeyInfoFromDataBase(stmt, keyInfo) == BFX_RESULT_SUCCESS &&
				keyInfo->expireTime > expireTime)
			{
				bfx_vector_sqlite_key_info_push_back(outKeyInfoVector, keyInfo);
			}
			else
			{
				BFX_FREE(keyInfo);
			}
		}
	}

	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite load keys failed:%d(%s)", result, sqlite3_errstr(result));

	result = sqlite3_reset(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load keys(reset) failed:%d(%s)", result, sqlite3_errstr(result));
	result = sqlite3_finalize(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load keys(finalize) failed:%d(%s)", result, sqlite3_errstr(result));
	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteFillKeyInfoFromDataBase(sqlite3_stmt* stmt, BdtHistory_KeyInfo* keyInfo)
{
	int columnBytes = 0;

	columnBytes = sqlite3_column_bytes(stmt, 1);
	if (columnBytes != BDT_AES_KEY_LENGTH)
	{
		BLOG_WARN("aes-key size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}
	memcpy(&keyInfo->aesKey, sqlite3_column_blob(stmt, 1), columnBytes);

	keyInfo->flagU32 = (uint32_t)sqlite3_column_int(stmt, 2);
	keyInfo->lastAccessTime = sqlite3_column_int64(stmt, 3);
	keyInfo->expireTime = sqlite3_column_int64(stmt, 4);

	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteCleanExpireRecode(SqliteStorage* storage,
	int64_t historyValidTime,
	int64_t keyValidTime)
{
	bool transaction = true;
	if (SqliteBeginTransaction(storage) != SQLITE_OK)
	{
		transaction = false;
	}

	int result = SQLITE_OK;
	do
	{
		char sql[1024];
		
		sprintf(sql, "DELETE FROM bdt_history_connection "
			"WHERE (lastConnectTime < "FORMAT64D") AND (foundTime < "FORMAT64D")",
			historyValidTime, historyValidTime);
		result = sqlite3_exec(storage->database, sql, NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
			break;
		}

		sprintf(sql, "DELETE FROM bdt_history_aesKey "
			"WHERE expireTime < "FORMAT64D"", keyValidTime);
		result = sqlite3_exec(storage->database, sql, NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
			break;
		}

		const char* sqlConst = "DELETE FROM bdt_history_peer AS peerT "
			"WHERE peerT.peerid NOT IN ("
				"SELECT connectionT.peerid AS peerid FROM bdt_history_connection AS connectionT "
				" UNION ALL "
				"SELECT aesKeyT.peerid AS peerid FROM bdt_history_aesKey AS aesKeyT "
			")";

		result = sqlite3_exec(storage->database, sqlConst, NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sqlConst, result, sqlite3_errstr(result));
			break;
		}

		sqlConst = "DELETE FROM bdt_history_sn AS snT "
			"WHERE snT.peeridSN NOT IN ("
				"SELECT peerT.peerid AS peerid FROM bdt_history_peer AS peerT) "
				"OR "
				"snT.peeridClient NOT IN ("
				"SELECT peerT.peerid AS peerid FROM bdt_history_peer AS peerT)";

		result = sqlite3_exec(storage->database, sqlConst, NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sqlConst, result, sqlite3_errstr(result));
			break;
		}
	} while (false);

	if (transaction)
	{
		SqliteCommitTransaction(storage);
	}

	return result == SQLITE_OK? BFX_RESULT_SUCCESS : BFX_RESULT_FAILED;
}

static void SqliteCloneHistoryInfo(
	const BdtHistory_PeerInfo* inHistoryInfo,
	BdtHistory_PeerInfo* outHistoryInfo)
{
	outHistoryInfo->peerid = inHistoryInfo->peerid;
	outHistoryInfo->constInfo = inHistoryInfo->constInfo;
	outHistoryInfo->flagsU32 = inHistoryInfo->flagsU32;

	bfx_vector_history_connect_init(&outHistoryInfo->connectVector);
	bfx_vector_history_connect_resize(&outHistoryInfo->connectVector,
		inHistoryInfo->connectVector.size);

	for (size_t i = 0; i < inHistoryInfo->connectVector.size; i++)
	{
		BdtHistory_PeerConnectInfo* inConnectInfo = inHistoryInfo->connectVector.connections[i];
		BdtHistory_PeerConnectInfo* outConnectInfo = SqliteCreateConnectInfo();
		*outConnectInfo = *inConnectInfo;
		if (inConnectInfo->localEndpoint != NULL)
		{
			*((BdtEndpoint*)outConnectInfo->localEndpoint) = *(inConnectInfo->localEndpoint);
		}
		else
		{
			outConnectInfo->localEndpoint = NULL;
		}
		outHistoryInfo->connectVector.connections[i] = outConnectInfo;
	}
}

static void SqliteDeleteIONode(SqliteIOQueueNode* ioNode)
{
	BLOG_CHECK(ioNode != NULL);
	if (ioNode == NULL)
	{
		return;
	}

	if (ioNode->type == SQLITE_IO_TYPE_ADD_PEER)
	{
		SqliteAddPeerIOQueueNode* addPeerNode = (SqliteAddPeerIOQueueNode*)ioNode;
		for (size_t i = 0; i < addPeerNode->peerHistory.connectVector.size; i++)
		{
			BFX_FREE(addPeerNode->peerHistory.connectVector.connections[i]);
		}
		bfx_vector_history_connect_cleanup(&addPeerNode->peerHistory.connectVector);
		BLOG_CHECK(addPeerNode->peerHistory.snVector.size == 0);
	}
	else if (ioNode->type == SQLITE_IO_TYPE_LOAD)
	{
		SqliteLoadIOQueueNode* loadIONode = (SqliteLoadIOQueueNode*)ioNode;
		if (loadIONode->userData.lpfnReleaseUserData != NULL)
		{
			loadIONode->userData.lpfnReleaseUserData(loadIONode->userData.userData);
		}
	}
	else if (ioNode->type == SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG)
	{
		SqliteLoadTunnelHistoryTcpLogIOQueueNode* loadTcpLogNode = (SqliteLoadTunnelHistoryTcpLogIOQueueNode*)ioNode;
		if (loadTcpLogNode->userData.lpfnReleaseUserData != NULL)
		{
			loadTcpLogNode->userData.lpfnReleaseUserData(loadTcpLogNode->userData.userData);
		}
	}
    else if (ioNode->type == SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID)
    {
        SqliteLoadPeerHistoryByPeeridIOQueueNode* loadByPeeridNode = (SqliteLoadPeerHistoryByPeeridIOQueueNode*)ioNode;
        if (loadByPeeridNode->userData.lpfnReleaseUserData != NULL)
        {
            loadByPeeridNode->userData.lpfnReleaseUserData(loadByPeeridNode->userData.userData);
        }
    }

	BFX_FREE(ioNode);
}

static uint32_t SqliteLoadPeerInfoByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    int64_t validTime,
    BdtHistory_PeerInfo* outPeerHistory);
static void SqliteLoadConnectByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    int64_t validTime,
    BdtHistory_PeerConnectVector* outConnectVector);
static uint32_t SqliteLoadSNPeeridsByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    BdtPeerid outSNPeerids[16],
    size_t* outSNCount
);

static void SqliteLoadPeerHistoryByPeeridIOProcessor(SqliteStorage* storage,
    SqliteLoadPeerHistoryByPeeridIOQueueNode* nextIONode)
{
    /*
    1.加载基础信息
    2.加载连接信息
    3.加载SN-客户端映射关系
    */
    
    BdtHistory_SuperNodeInfo _superNodePeerHistory;
    BdtHistory_PeerInfo* peerHistory = &_superNodePeerHistory.peerInfo;
    memset(&_superNodePeerHistory, 0, sizeof(_superNodePeerHistory));

    bfx_vector_history_connect_init(&peerHistory->connectVector);
    bfx_vector_history_connect_reserve(&peerHistory->connectVector, 32);

    bfx_vector_super_node_info_init(&peerHistory->snVector);

    BdtPeerid snPeerids[16];
    size_t snCount = 0;

    uint32_t errorCode = SqliteLoadPeerInfoByPeerid(storage,
        &nextIONode->peerid,
        nextIONode->historyValidTime,
        peerHistory);

    if (errorCode == 0)
    {
        SqliteLoadConnectByPeerid(storage,
            &nextIONode->peerid,
            nextIONode->historyValidTime,
            &peerHistory->connectVector);

        SqliteLoadSNPeeridsByPeerid(storage,
            &nextIONode->peerid,
            snPeerids,
            &snCount);
    }

    nextIONode->callback(errorCode,
        peerHistory,
        snPeerids,
        snCount,
        nextIONode->userData.userData);

    for (size_t ci = 0; ci < peerHistory->connectVector.size; ci++)
    {
        BFX_FREE(peerHistory->connectVector.connections[ci]);
    }

    bfx_vector_history_connect_cleanup(&peerHistory->connectVector);
    bfx_vector_super_node_info_cleanup(&peerHistory->snVector);
}

static uint32_t SqliteLoadPeerInfoByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    int64_t validTime,
    BdtHistory_PeerInfo* outPeerHistory)
{
    if (storage->database == NULL)
    {
        BLOG_WARN("should load data first.");
        return BFX_RESULT_CLOSED;
    }

    sqlite3_stmt* stmt = storage->sqlStmt[SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID];
    BLOG_CHECK(stmt != NULL);

    int result = sqlite3_reset(stmt);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 reset stmt(SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID) failed:%d(%s).", result, sqlite3_errstr(result));
        return BFX_RESULT_FAILED;
    }

    result = sqlite3_bind_blob(stmt, 1, peerid->pid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(peerid->pid, 15, "sqlite3 stmt(SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID) bind peerid failed:%d(%s).",
            result, sqlite3_errstr(result));
        return BFX_RESULT_FAILED;
    }

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        if (result == SQLITE_DONE)
        {
            return BFX_RESULT_NOT_FOUND;
        }
        BLOG_WARN("sqlite3 stmt(SQLITE_IO_TYPE_LOAD_PEER_HISTORY_BY_PEERID) step failed:%d(%s).", result, sqlite3_errstr(result));
        return BFX_RESULT_FAILED;
    }
    
    return SqliteFillPeerInfoFromDataBase(stmt, outPeerHistory);
}

static void SqliteLoadConnectByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    int64_t validTime,
    BdtHistory_PeerConnectVector* outConnectVector)
{
    if (storage->database == NULL)
    {
        BLOG_WARN("should load data first.");
        return;
    }

    sqlite3_stmt* stmt = storage->sqlStmt[SQLITE_IO_TYPE_LOAD_HISTORY_CONNECT_INFO_BY_PEERID];
    BLOG_CHECK(stmt != NULL);

    int result = sqlite3_reset(stmt);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 reset stmt(SQLITE_IO_TYPE_LOAD_HISTORY_CONNECT_INFO_BY_PEERID) failed:%d(%s).", result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_blob(stmt, 1, peerid->pid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(peerid->pid, 15, "sqlite3 stmt(SQLITE_IO_TYPE_LOAD_HISTORY_CONNECT_INFO_BY_PEERID) bind peerid failed:%d(%s).",
            result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        BLOG_WARN("sqlite3 stmt(SQLITE_IO_TYPE_LOAD_HISTORY_CONNECT_INFO_BY_PEERID) step failed:%d(%s).", result, sqlite3_errstr(result));
        return;
    }

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        // fill
        size_t peeridSize = sqlite3_column_bytes(stmt, 0);
        if (peeridSize != BDT_PEERID_LENGTH)
        {
            BLOG_WARN("peerid size(%d) is invalid.", peeridSize);
            continue;
        }
        const BdtPeerid* peerid = (const BdtPeerid*)sqlite3_column_blob(stmt, 0);

        BdtHistory_PeerConnectInfo* connectInfo = SqliteCreateConnectInfo();
        if (SqliteFillConnectInfoFromDataBase(stmt, connectInfo) == BFX_RESULT_SUCCESS &&
            (connectInfo->lastConnectTime > validTime || (connectInfo->lastConnectTime == 0 && connectInfo->foundTime > validTime)))
        {
            bfx_vector_history_connect_push_back(outConnectVector, connectInfo);
        }
        else
        {
            BFX_FREE(connectInfo);
        }
    }

    return;
}

static uint32_t SqliteLoadSNPeeridsByPeerid(SqliteStorage* storage,
    const BdtPeerid* peerid,
    BdtPeerid outSNPeerids[16],
    size_t* outSNCount
    )
{
    if (storage->database == NULL)
    {
        BLOG_WARN("should load data first.");
        return BFX_RESULT_CLOSED;
    }

    sqlite3_stmt* stmt = storage->sqlStmt[SQLITE_IO_TYPE_LOAD_SN_CLIENT_MAP_BY_CLIENT];
    BLOG_CHECK(stmt != NULL);

    int result = sqlite3_reset(stmt);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 reset stmt(SQLITE_IO_TYPE_LOAD_SN_CLIENT_MAP_BY_CLIENT) failed:%d(%s).", result, sqlite3_errstr(result));
        return BFX_RESULT_FAILED;
    }

    result = sqlite3_bind_blob(stmt, 1, peerid->pid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(peerid->pid, 15, "sqlite3 stmt(SQLITE_IO_TYPE_LOAD_SN_CLIENT_MAP_BY_CLIENT) bind peerid failed:%d(%s).",
            result, sqlite3_errstr(result));
        return BFX_RESULT_FAILED;
    }

    for (*outSNCount = 0;
        (result = sqlite3_step(stmt)) == SQLITE_ROW && *outSNCount < 16;
        (*outSNCount)++)
    {
        // fill
        int columnBytes = 0;

        columnBytes = sqlite3_column_bytes(stmt, 0);
        if (columnBytes != BDT_PEERID_LENGTH)
        {
            BLOG_WARN("peerid size(%d) is invalid.", columnBytes);
            (*outSNCount)--;
            continue;
        }
        outSNPeerids[*outSNCount] = *((const BdtPeerid*)sqlite3_column_blob(stmt, 0));
    }

    BLOG_CHECK_EX(result == SQLITE_DONE || result == SQLITE_ROW,
        "sqlite stmt(SQLITE_IO_TYPE_LOAD_SN_CLIENT_MAP_BY_CLIENT) step failed:%d(%s)", result, sqlite3_errstr(result));

    return BFX_RESULT_SUCCESS;
}

static void SqliteUpdateConnectIOProcessorImpl(
    SqliteStorage* storage,
	SqliteIOType ioType,
	const BdtPeerid* peerid,
	BdtHistory_PeerConnectInfo* connectInfo);
static void SqliteSetConstInfoImpl(
    SqliteStorage* storage,
    SqliteIOType ioType,
    const BdtPeerid* peerid,
    uint32_t flag,
    const BdtPeerConstInfo* constInfo
);

static void SqliteAddPeerIOProcessor(SqliteStorage* storage, SqliteAddPeerIOQueueNode* addPeerIO)
{
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}

	BdtHistory_PeerInfo* history = &addPeerIO->peerHistory;

    SqliteSetConstInfoImpl(
        storage,
        addPeerIO->base.type,
        &history->peerid,
        history->flagsU32,
        &history->constInfo
    );

	for (size_t i = 0; i < history->connectVector.size; i++)
	{
		SqliteUpdateConnectIOProcessorImpl(storage,
			SQLITE_IO_TYPE_ADD_CONNECT,
			&history->peerid,
			history->connectVector.connections[i]);
	}

	return;
}

static void SqliteSetConstInfoIOProcessor(SqliteStorage* storage, SqliteSetConstInfoIOQueueNode* setConstInfoIO)
{
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}

    SqliteSetConstInfoImpl(
        storage,
        setConstInfoIO->base.type,
        &setConstInfoIO->peerid,
        setConstInfoIO->flags,
        &setConstInfoIO->constInfo
    );
}

static void SqliteSetConstInfoImpl(
    SqliteStorage* storage,
    SqliteIOType ioType,
    const BdtPeerid* peerid,
    uint32_t flag,
    const BdtPeerConstInfo* constInfo
)
{

    sqlite3_stmt* stmt = storage->sqlStmt[ioType];
    BLOG_CHECK(stmt != NULL);

    int result = sqlite3_reset(stmt);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 reset stmt(%d) failed:%d(%s).", ioType, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_blob(stmt, 1, peerid->pid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(peerid->pid, 15,
            "sqlite3 stmt(%d) bind peerid failed:%d(%s).",
            ioType, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 2, constInfo->areaCode.continent);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind continent(%d) failed:%d(%s).",
            ioType, (int)constInfo->areaCode.continent, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 3, constInfo->areaCode.country);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind country(%d) failed:%d(%s).",
            ioType, (int)constInfo->areaCode.country, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 4, constInfo->areaCode.carrier);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind carrier(%d) failed:%d(%s).",
            ioType, (int)constInfo->areaCode.carrier, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 5, constInfo->areaCode.city);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind city(%d) failed:%d(%s).",
            ioType, (int)constInfo->areaCode.city, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 6, constInfo->areaCode.inner);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind inner(%d) failed:%d(%s).",
            ioType, (int)constInfo->areaCode.inner, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_blob(stmt, 7, constInfo->deviceId, BDT_MAX_DEVICE_ID_LENGTH, SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(constInfo->deviceId, 15,
            "sqlite3 stmt(%d) bind deviceid failed:%d(%s).",
            ioType, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 8, constInfo->publicKeyType);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind publicKeyType(%d) failed:%d(%s).",
            ioType, (int)constInfo->publicKeyType, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_blob(stmt,
        9,
        &constInfo->publicKey,
        BdtGetPublicKeyLength(constInfo->publicKeyType),
        SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        BLOG_HEX(&constInfo->publicKey, 15,
            "sqlite3 stmt(%d) bind publickey failed:%d(%s).",
            ioType, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_bind_int(stmt, 10, (int)flag);
    if (result != SQLITE_OK)
    {
        BLOG_WARN("sqlite3 stmt(%d) bind flag(%u) failed:%d(%s).",
            ioType, flag, result, sqlite3_errstr(result));
        return;
    }

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        BLOG_WARN("sqlite3 stmt(%d) step failed:%d(%s).", ioType, result, sqlite3_errstr(result));
        return;
    }
}

static void SqliteUpdateConnectIOProcessor(SqliteStorage* storage, SqliteConnectInfoIOQueueNode* connectIO)
{
	BLOG_CHECK(connectIO->base.type == SQLITE_IO_TYPE_ADD_CONNECT ||
		connectIO->base.type == SQLITE_IO_TYPE_UPDATE_CONNECT);
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}
	SqliteUpdateConnectIOProcessorImpl(storage,
		connectIO->base.type,
		&connectIO->peerid,
		&connectIO->connectInfo);
}

static void SqliteUpdateConnectIOProcessorImpl(SqliteStorage* storage,
	SqliteIOType ioType,
	const BdtPeerid* peerid,
	BdtHistory_PeerConnectInfo* connectInfo)
{
	sqlite3_stmt* stmt = storage->sqlStmt[ioType];
	BLOG_CHECK(stmt != NULL);

	int result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 reset stmt(%d) failed:%d(%s).", ioType, result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt, 1, peerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(peerid->pid, 15, "sqlite3 stmt(%d) bind peerid failed:%d(%s).",
			ioType,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt,
		2,
		&connectInfo->endpoint,
		ENDPOINT_LENGTH(&connectInfo->endpoint),
		SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(&connectInfo->endpoint, 15,
			"sqlite3 stmt(%d) bind endpoint failed:%d(%s).",
			ioType,
			result, sqlite3_errstr(result));
		return;
	}

	if (connectInfo->localEndpoint != NULL)
	{
		result = sqlite3_bind_blob(stmt,
			3,
			&connectInfo->localEndpoint,
			ENDPOINT_LENGTH(connectInfo->localEndpoint),
			SQLITE_TRANSIENT);
		if (result != SQLITE_OK)
		{
			BLOG_HEX(&connectInfo->endpoint, 15,
				"sqlite3 stmt(%d) bind local endpoint failed:%d(%s).",
				ioType,
				result, sqlite3_errstr(result));
			return;
		}
	}
	else
	{
		result = sqlite3_bind_null(stmt, 3);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite3 stmt(%d) bind local endpoint(null) failed:%d(%s).",
				ioType,
				result, sqlite3_errstr(result));
			return;
		}
	}

	result = sqlite3_bind_int(stmt, 4, connectInfo->flags.u32);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind publicKeyType(%u) failed:%d(%s).",
			ioType,
			connectInfo->flags.u32,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int64(stmt, 5, connectInfo->lastConnectTime);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind foundTime("FORMAT64D") failed:%d(%s).",
			ioType,
			connectInfo->lastConnectTime,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 6, connectInfo->rto);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind rto(%u) failed:%d(%s).",
			ioType,
			connectInfo->rto,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 7, connectInfo->sendSpeed);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind sendSpeed(%u) failed:%d(%s).",
			ioType,
			connectInfo->sendSpeed,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 8, connectInfo->recvSpeed);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind recvSpeed(%u) failed:%d(%s).",
			ioType,
			connectInfo->recvSpeed,
			result, sqlite3_errstr(result));
		return;
	}

	if (ioType == SQLITE_IO_TYPE_ADD_CONNECT)
	{
		result = sqlite3_bind_int64(stmt, 9, connectInfo->foundTime);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite3 stmt(%d) bind foundTime("FORMAT64D") failed:%d(%s).",
				ioType,
				connectInfo->foundTime,
				result, sqlite3_errstr(result));
			return;
		}
	}

	result = sqlite3_step(stmt);
	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite3 stmt(%d) step failed:%d(%s).", ioType, result, sqlite3_errstr(result));
}

static void SqliteUpdateSuperNodeIOProcessor(SqliteStorage* storage, SqliteSuperNodeIOQueueNode* superNodeIO)
{
	BLOG_CHECK(superNodeIO->base.type == SQLITE_IO_TYPE_ADD_SUPER_NODE ||
		superNodeIO->base.type == SQLITE_IO_TYPE_UPDATE_SUPER_NODE);
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}

	sqlite3_stmt* stmt = storage->sqlStmt[superNodeIO->base.type];
	BLOG_CHECK(stmt != NULL);

	int result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 reset stmt(%d) failed:%d(%s).", superNodeIO->base.type, result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt, 1, &superNodeIO->snPeerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(superNodeIO->snPeerid.pid, 15, "sqlite3 stmt(%d) bind sn-peerid failed:%d(%s).",
			superNodeIO->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt, 2, &superNodeIO->clientPeerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(superNodeIO->clientPeerid.pid, 15, "sqlite3 stmt(%d) bind client-peerid failed:%d(%s).",
			superNodeIO->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int64(stmt, 3, superNodeIO->responseTime);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind responseTime("FORMAT64D") failed:%d(%s).",
			superNodeIO->base.type,
			superNodeIO->responseTime,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_step(stmt);
	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite3 stmt(%d) step failed:%d(%s).", superNodeIO->base.type, result, sqlite3_errstr(result));
}

static void SqliteUpdateAesKeyIOProcessor(SqliteStorage* storage, SqliteAESKeyIOQueueNode* aesKeyIO)
{
	BLOG_CHECK(aesKeyIO->base.type == SQLITE_IO_TYPE_ADD_KEY ||
		aesKeyIO->base.type == SQLITE_IO_TYPE_UPDATE_KEY);
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}

	sqlite3_stmt* stmt = storage->sqlStmt[aesKeyIO->base.type];
	BLOG_CHECK(stmt != NULL);

	int result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 reset stmt(%d) failed:%d(%s).", aesKeyIO->base.type, result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt, 1, &aesKeyIO->peerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(aesKeyIO->peerid.pid, 15, "sqlite3 stmt(%d) bind peerid failed:%d(%s).",
			aesKeyIO->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt,
		2,
		&aesKeyIO->aesKey,
		BDT_AES_KEY_LENGTH,
		SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(aesKeyIO->aesKey, 15,
			"sqlite3 stmt(%d) bind local aes-key failed:%d(%s).",
			aesKeyIO->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 3, aesKeyIO->flags);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind flags(%u) failed:%d(%s).",
			aesKeyIO->base.type,
			aesKeyIO->flags,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int64(stmt, 4, aesKeyIO->lastAccessTime);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind lastAccessTime("FORMAT64D") failed:%d(%s).",
			aesKeyIO->base.type,
			aesKeyIO->lastAccessTime,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int64(stmt, 5, aesKeyIO->expireTime);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind expireTime("FORMAT64D") failed:%d(%s).",
			aesKeyIO->base.type,
			aesKeyIO->expireTime,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_step(stmt);
	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite3 stmt(%d) step failed:%d(%s).", aesKeyIO->base.type, result, sqlite3_errstr(result));
}

typedef struct TcpTunnel4PeerLogVector
{
    Bdt_TunnelHistoryTcpLogForLocalEndpoint* log4Local;
	size_t size;
	size_t _allocSize;
} TcpTunnel4LocalLogVector, tcp_log4local;

BFX_VECTOR_DEFINE_FUNCTIONS(tcp_log4local, Bdt_TunnelHistoryTcpLogForLocalEndpoint, log4Local, size, _allocSize)
typedef tcp_tunnel_log sqlite_tcp_tunnel_log;
BFX_VECTOR_DEFINE_FUNCTIONS(sqlite_tcp_tunnel_log, Bdt_TunnelHistoryTcpLog, logs, size, _allocSize)
typedef tcp_tunnel_log_for_remote sqlite_tcp_tunnel_log_for_remote;
BFX_VECTOR_DEFINE_FUNCTIONS(sqlite_tcp_tunnel_log_for_remote, Bdt_TunnelHistoryTcpLogForRemoteEndpoint, logsForRemote, size, _allocSize)

static uint32_t SqliteLoadTcpTunnelLogs(SqliteStorage* storage,
	const BdtPeerid* remotePeerid,
	int64_t timeLimit,
    TcpTunnel4LocalLogVector* outTcpLog4PeerVector,
	size_t* outExpireCount);
static uint32_t SqliteCleanExpireTcpTunnelLog(SqliteStorage* storage,
	int64_t validTimeStamp);
static void SqliteLoadTunnelHistoryTcpLogIOProcessor(SqliteStorage* storage, SqliteLoadTunnelHistoryTcpLogIOQueueNode* loadTunnelTcpLog)
{
    TcpTunnel4LocalLogVector tcpLog4LocalVector;
	bfx_vector_tcp_log4local_init(&tcpLog4LocalVector);

	int64_t timeLimit = BfxTimeGetNow(false) - loadTunnelTcpLog->invalidTime;
	size_t expireCount = 0;
	uint32_t result = SqliteLoadTcpTunnelLogs(storage, &loadTunnelTcpLog->remotePeerid, timeLimit, &tcpLog4LocalVector, &expireCount);

	loadTunnelTcpLog->callback(result,
		&loadTunnelTcpLog->remotePeerid,
        tcpLog4LocalVector.log4Local,
        tcpLog4LocalVector.size,
		loadTunnelTcpLog->userData.userData
	);

	for (size_t i = 0; i < tcpLog4LocalVector.size; i++)
	{
		Bdt_TunnelHistoryTcpLogForLocalEndpoint* log4Local = tcpLog4LocalVector.log4Local + i;
        for (size_t ri = 0; ri < log4Local->logForRemoteVector.size; ri++)
        {
            bfx_vector_sqlite_tcp_tunnel_log_cleanup(&log4Local->logForRemoteVector.logsForRemote[ri].logVector);
        }
		bfx_vector_sqlite_tcp_tunnel_log_for_remote_cleanup(&log4Local->logForRemoteVector);
	}
	bfx_vector_tcp_log4local_cleanup(&tcpLog4LocalVector);

	if (expireCount > 0)
	{
		SqliteCleanExpireTcpTunnelLog(storage, timeLimit);
	}
}

static uint32_t SqliteFillTcpTunnelLogFromDataBase(
	sqlite3_stmt* stmt,
	BdtEndpoint* outLocalEndpoint,
	BdtEndpoint* outRemoteEndpoint,
	Bdt_TunnelHistoryTcpLog* outTcpTunnelLog);
static uint32_t SqliteLoadTcpTunnelLogs(SqliteStorage* storage,
	const BdtPeerid* remotePeerid,
	int64_t timeLimit,
    TcpTunnel4LocalLogVector* outTcpLog4LocalVector,
	size_t* outExpireCount)
{
	sqlite3_stmt* stmt = storage->sqlStmt[SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG];
	BLOG_CHECK(stmt != NULL);

	int result = sqlite3_bind_blob(stmt, 1, remotePeerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(remotePeerid->pid, 15, "sqlite3 stmt(SQLITE_IO_TYPE_LOAD_TUNNEL_HISTORY_TCP_LOG) bind peerid failed:%d(%s).",
			result, sqlite3_errstr(result));
		return BFX_RESULT_FAILED;
	}

	bfx_vector_tcp_log4local_reserve(outTcpLog4LocalVector, 4);

    Bdt_TunnelHistoryTcpLogForLocalEndpoint* lastLog4Local = NULL;
    Bdt_TunnelHistoryTcpLogForRemoteEndpoint* lastLog4Remote = NULL;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		// fill
		BdtEndpoint localEndpoint;
		BdtEndpoint remoteEndpoint;
		Bdt_TunnelHistoryTcpLog tcpTunnelLog;

		if (SqliteFillTcpTunnelLogFromDataBase(stmt, &localEndpoint, &remoteEndpoint, &tcpTunnelLog) == BFX_RESULT_SUCCESS)
		{
			if (tcpTunnelLog.timestamp < timeLimit)
			{
				(*outExpireCount)++;
				continue;
			}

            if (lastLog4Local == NULL ||
                BdtEndpointCompare(&lastLog4Local->local, &localEndpoint, true) != 0)
            {
                Bdt_TunnelHistoryTcpLogForLocalEndpoint newLog4Local = {
                    .local = localEndpoint
                };
                bfx_vector_sqlite_tcp_tunnel_log_for_remote_init(&newLog4Local.logForRemoteVector);
                bfx_vector_sqlite_tcp_tunnel_log_for_remote_reserve(&newLog4Local.logForRemoteVector, 4);

                bfx_vector_tcp_log4local_push_back(outTcpLog4LocalVector, newLog4Local);
                lastLog4Local = outTcpLog4LocalVector->log4Local + outTcpLog4LocalVector->size - 1;
                lastLog4Remote = NULL;
            }

			if (lastLog4Remote == NULL ||
				BdtEndpointCompare(&lastLog4Remote->remote, &remoteEndpoint, true) != 0)
			{
                Bdt_TunnelHistoryTcpLogForRemoteEndpoint newLog4Remote = {
					.remote = remoteEndpoint,
				};
				bfx_vector_sqlite_tcp_tunnel_log_init(&newLog4Remote.logVector);
				bfx_vector_sqlite_tcp_tunnel_log_reserve(&newLog4Remote.logVector, 1024);

				bfx_vector_sqlite_tcp_tunnel_log_for_remote_push_back(&lastLog4Local->logForRemoteVector, newLog4Remote);
                lastLog4Remote = lastLog4Local->logForRemoteVector.logsForRemote + lastLog4Local->logForRemoteVector.size - 1;
			}
			bfx_vector_sqlite_tcp_tunnel_log_push_back(&lastLog4Remote->logVector, tcpTunnelLog);
		}
	}

	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite load peers failed:%d(%s)", result, sqlite3_errstr(result));

	result = sqlite3_reset(stmt);
	BLOG_CHECK_EX(result == SQLITE_OK, "sqlite load peers(reset) failed:%d(%s)", result, sqlite3_errstr(result));

	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteFillTcpTunnelLogFromDataBase(
	sqlite3_stmt* stmt,
	BdtEndpoint* outLocalEndpoint,
	BdtEndpoint* outRemoteEndpoint,
	Bdt_TunnelHistoryTcpLog* outTcpTunnelLog)
{
	int columnBytes = 0;

	columnBytes = sqlite3_column_bytes(stmt, 0);
	if (columnBytes != ENDPOINT_LENGTH_V4 && columnBytes != ENDPOINT_LENGTH_V6)
	{
		BLOG_WARN("local endpoint size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}

	memcpy(outLocalEndpoint, sqlite3_column_blob(stmt, 0), columnBytes);

	columnBytes = sqlite3_column_bytes(stmt, 1);
	if (columnBytes != ENDPOINT_LENGTH_V4 && columnBytes != ENDPOINT_LENGTH_V6)
	{
		BLOG_WARN("remote endpoint size(%d) is invalid.", columnBytes);
		return BFX_RESULT_INVALID_LENGTH;
	}

	memcpy(outRemoteEndpoint, sqlite3_column_blob(stmt, 1), columnBytes);

	outTcpTunnelLog->type = (Bdt_TunnelHistoryTcpLogType)sqlite3_column_int(stmt, 2);
	outTcpTunnelLog->times = (uint32_t)sqlite3_column_int(stmt, 3);
	outTcpTunnelLog->timestamp = (int64_t)sqlite3_column_int64(stmt, 4);

	return BFX_RESULT_SUCCESS;
}

static uint32_t SqliteCleanExpireTcpTunnelLog(SqliteStorage* storage,
	int64_t validTimeStamp)
{
	int result = SQLITE_OK;

	char sql[1024];

	sprintf(sql, "DELETE FROM bdt_history_tcpTunnelLog "
		"WHERE (timestamp < "FORMAT64D")",
		validTimeStamp);
	result = sqlite3_exec(storage->database, sql, NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
	}

	return result == SQLITE_OK ? BFX_RESULT_SUCCESS : BFX_RESULT_FAILED;
}

static void SqliteUpdateTunnelHistoryTcpLogIOProcessor(SqliteStorage* storage, SqliteUpdateTunnelHistoryTcpLogIOQueueNode* updateTunnelTcpLog)
{
	BLOG_CHECK(updateTunnelTcpLog->base.type == SQLITE_IO_TYPE_ADD_TUNNEL_HISTORY_TCP_LOG ||
		updateTunnelTcpLog->base.type == SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG);
	if (storage->database == NULL)
	{
		BLOG_WARN("should load data first.");
		return;
	}

	sqlite3_stmt* stmt = storage->sqlStmt[updateTunnelTcpLog->base.type];
	BLOG_CHECK(stmt != NULL);

	int result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 reset stmt(%d) failed:%d(%s).", updateTunnelTcpLog->base.type, result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt, 1, &updateTunnelTcpLog->remotePeerid, BDT_PEERID_LENGTH, SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_HEX(updateTunnelTcpLog->remotePeerid.pid, 15, "sqlite3 stmt(%d) bind peerid failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt,
		2,
		&updateTunnelTcpLog->localEndpoint,
		ENDPOINT_LENGTH(&updateTunnelTcpLog->localEndpoint),
		SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind local local-endpoint failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_blob(stmt,
		3,
		&updateTunnelTcpLog->remoteEndpoint,
		ENDPOINT_LENGTH(&updateTunnelTcpLog->remoteEndpoint),
		SQLITE_TRANSIENT);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind local remote-endpoint failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 4, updateTunnelTcpLog->type);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind type(%d) failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			updateTunnelTcpLog->type,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int(stmt, 5, updateTunnelTcpLog->times);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind times(%d) failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			updateTunnelTcpLog->times,
			result, sqlite3_errstr(result));
		return;
	}

	result = sqlite3_bind_int64(stmt, 6, updateTunnelTcpLog->timestamp);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite3 stmt(%d) bind timestamp("FORMAT64D") failed:%d(%s).",
			updateTunnelTcpLog->base.type,
			updateTunnelTcpLog->timestamp,
			result, sqlite3_errstr(result));
		return;
	}

	if (updateTunnelTcpLog->base.type == SQLITE_IO_TYPE_UPDATE_TUNNEL_HISTORY_TCP_LOG)
	{
		result = sqlite3_bind_int64(stmt, 7, updateTunnelTcpLog->oldTimeStamp);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite3 stmt(%d) bind old-timestamp("FORMAT64D") failed:%d(%s).",
				updateTunnelTcpLog->base.type,
				updateTunnelTcpLog->oldTimeStamp,
				result, sqlite3_errstr(result));
			return;
		}
	}

	result = sqlite3_step(stmt);
	BLOG_CHECK_EX(result == SQLITE_DONE, "sqlite3 stmt(%d) step failed:%d(%s).", updateTunnelTcpLog->base.type, result, sqlite3_errstr(result));
}

static void SqliteCloseIOProcessor(SqliteStorage* storage)
{
	BLOG_INFO("sqlite will be closed.");

	if (storage->transactionIOCount > 0)
	{
		SqliteCommitTransaction(storage);
	}

	int result = SQLITE_OK;
	for (int i = 0; i < BFX_ARRAYSIZE(storage->sqlStmt); i++)
	{
		if (storage->sqlStmt[i] != NULL)
		{
			result = sqlite3_finalize(storage->sqlStmt[i]);
			if (result != SQLITE_OK)
			{
				BLOG_WARN("sqlite(%d) finalize failed:%d(%s).", i, result, sqlite3_errstr(result));
			}
			storage->sqlStmt[i] = NULL;
		}
	}

	if (storage->database != NULL)
	{
		result = sqlite3_close(storage->database);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite close failed:%d(%s)", result, sqlite3_errstr(result));
		}
		storage->database = NULL;
	}
}

static int SqliteBeginTransaction(SqliteStorage* storage)
{
	if (storage->transactionDepth > 0)
	{
		storage->transactionDepth++;
		return SQLITE_OK;
	}

	const char* sql = "BEGIN IMMEDIATE TRANSACTION";
	int result = sqlite3_exec(storage->database, sql, NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
	}
	else
	{
		storage->transactionDepth = 1;
	}
	return result;
}

static void SqliteCommitTransaction(SqliteStorage* storage)
{
	storage->transactionDepth--;
	BLOG_CHECK(storage->transactionDepth >= 0);

	if (storage->transactionDepth == 0)
	{
		const char* sql = "COMMIT TRANSACTION";
		int result = sqlite3_exec(storage->database, sql, NULL, NULL, NULL);
		if (result != SQLITE_OK)
		{
			BLOG_WARN("sqlite(%s) exec failed:%d(%s).", sql, result, sqlite3_errstr(result));
		}
	}
}