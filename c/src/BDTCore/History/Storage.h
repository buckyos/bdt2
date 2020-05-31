#ifndef __BDT_HISTORY_STORAGE_H__
#define __BDT_HISTORY_STORAGE_H__

#include "../BdtCore.h"
#include "../Global/CryptoToolKit.h"

typedef struct BdtHistory_PeerInfo BdtHistory_PeerInfo;
typedef struct BdtHistory_SuperNodeInfo BdtHistory_SuperNodeInfo;
typedef struct Bdt_TunnelHistoryTcpLogForLocalEndpoint Bdt_TunnelHistoryTcpLogForLocalEndpoint;
typedef enum Bdt_TunnelHistoryTcpLogType Bdt_TunnelHistoryTcpLogType;

typedef union BdtHistory_PeerConnectInfoFlags
{
	struct
	{
		uint32_t directTcp : 1;
		uint32_t reverseTcp : 1;
		uint32_t zero : 30;
	};
	uint32_t u32;
} BdtHistory_PeerConnectInfoFlags;

typedef struct BdtHistory_PeerConnectInfo
{
	BdtEndpoint endpoint; // 对端地址
	const BdtEndpoint* localEndpoint; // 本地地址
	int64_t foundTime; // 连接创建时间，就是第一次连接时间
	int64_t lastConnectTime; // 最后一次连接时间
	uint32_t rto;
	uint32_t sendSpeed;
	uint32_t recvSpeed;
	BdtHistory_PeerConnectInfoFlags flags;
} BdtHistory_PeerConnectInfo;

typedef struct BdtHistory_PeerConnectVector
{
	BdtHistory_PeerConnectInfo** connections;
	size_t size;
	size_t _allocsize;
} BdtHistory_PeerConnectVector;

typedef struct BdtHistory_PeerSuperNodeInfoVector
{
	BdtHistory_SuperNodeInfo** snHistorys;
	size_t size;
	size_t _allocsize;
} BdtHistory_PeerSuperNodeInfoVector;

typedef struct BdtHistory_PeerInfoFlag
{
    uint32_t isLocal : 1;
    uint32_t isSn : 1;
    uint32_t isStorage : 1;
    uint32_t hasConst : 1;
    uint32_t zero : 27;
} BdtHistory_PeerInfoFlag;

typedef struct BdtHistory_PeerInfo
{
	BdtPeerid peerid;

	BdtPeerConstInfo constInfo;
	union {
        BdtHistory_PeerInfoFlag flags;
		uint32_t flagsU32;
	};
	BdtHistory_PeerConnectVector connectVector;
	BdtHistory_PeerSuperNodeInfoVector snVector;
} BdtHistory_PeerInfo;

typedef struct BdtHistory_SuperNodeInfo
{
	BdtHistory_PeerInfo peerInfo;
	int64_t lastResponseTime;
} BdtHistory_SuperNodeInfo;

typedef struct BdtHistory_KeyInfo {
	uint8_t aesKey[BDT_AES_KEY_LENGTH];//256bits aeskey
	union {
		struct {
			uint32_t isConfirmed : 1;
			uint32_t isStorage : 1;
		} flags;
		uint32_t flagU32;
	};
	int64_t expireTime;
	int64_t lastAccessTime;
	const BdtPeerid* peerid;
} BdtHistory_KeyInfo;

typedef struct BdtStorage BdtStorage;

typedef void (*BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK)(
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount,
	void* userData
	);

#define BDT_STORAGE_EVENT_ERROR 1

/*
所有IO操作返回值:
立即完成：BFX_RESULT_SUCCESS
排队等待完成：BFX_RESULT_PENDING
重复操作：BFX_RESULT_ALREADY_OPERATION
其他值为失败
*/

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_ALL)(
	BdtStorage* storage,
	int64_t historyValidTime, // 历史记录最早保留时间
	BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback,
	const BfxUserData* userData
	);

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_ADD_PEER_HISTORY)(
	BdtStorage* storage,
	const BdtHistory_PeerInfo* history
	);

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_SET_CONST_INFO)(
    BdtStorage* storage,
    const BdtHistory_PeerInfo* history
    );

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_UPDATE_CONNECT_HISTORY)(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const BdtHistory_PeerConnectInfo* connectInfo,
	bool isNew
	);
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_UPDATE_SUPERNODE_HISTORY)(
	BdtStorage* storage,
	const BdtPeerid* clientPeerid,
	const BdtPeerid* superNodePeerid,
	int64_t responseTime,
	bool isNew
	);

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_UPDATE_AES_KEY)(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
	);

typedef void (*BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK)(
    uint32_t errorCode,
    const BdtHistory_PeerInfo* history,
    const BdtPeerid snPeerids[],
    size_t snCount,
    void* userData
    );

// 加载特定节点的历史记录
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_PEER_HISTORY_BY_PEERID)(
    BdtStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // 历史记录最早保留时间
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback,
    const BfxUserData* userData
    );

typedef void (*BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK)(
	uint32_t errorCode,
	const BdtPeerid* remotePeerid,
	const Bdt_TunnelHistoryTcpLogForLocalEndpoint* log4Endpoints,
	size_t endpointCount,
	void* userData
	);

// 加载TCP历史连接日志
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_TUNNEL_HISTORY_TCP_LOG)(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
	);

// 更新TCP历史连接日志
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_UPDATE_TUNNEL_HISTORY_TCP_LOG)(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type,
	uint32_t times,
	int64_t timestamp,
	int64_t oldTimestamp,
	bool isNew
	);

typedef void(*BDT_HISTORY_STORAGE_PROC_DESTROY_SELF)(BdtStorage* storage);

typedef struct BdtStorage
{
	// 加载节点历史通信信息
	BDT_HISTORY_STORAGE_PROC_LOAD_ALL loadAll;

	// 增加一个节点信息，保存其相关的连接信息，但不附带保存相关的SN信息
	BDT_HISTORY_STORAGE_PROC_ADD_PEER_HISTORY addPeerHistory;

    // 设置节点的constInfo，部分情况下节点信息保存时没有ConstInfo
    BDT_HISTORY_STORAGE_PROC_SET_CONST_INFO setPeerConstInfo;

	// 更新某节点的一条连接信息
	BDT_HISTORY_STORAGE_PROC_UPDATE_CONNECT_HISTORY updateConnectHistory;

	// 更新在某SuperNode上查询到某节点的时间
	BDT_HISTORY_STORAGE_PROC_UPDATE_SUPERNODE_HISTORY updateSuperNodeHistory;

	// 更新一个节点通信的AES密钥(访问时间/超时时间/标记等)
	BDT_HISTORY_STORAGE_PROC_UPDATE_AES_KEY updateAesKey;

    // 加载特定节点的历史记录
    BDT_HISTORY_STORAGE_PROC_LOAD_PEER_HISTORY_BY_PEERID loadPeerHistoryByPeerid;

	// 加载特定节点的TCP连接日志
	BDT_HISTORY_STORAGE_PROC_LOAD_TUNNEL_HISTORY_TCP_LOG loadTunnelHistoryTcpLog;

	// 更新/追加TCP连接日志
	BDT_HISTORY_STORAGE_PROC_UPDATE_TUNNEL_HISTORY_TCP_LOG updateTunnelHistoryTcpLog;

	// 销毁BdtStorage对象
	BDT_HISTORY_STORAGE_PROC_DESTROY_SELF destroy;

} BdtStorage;

void BdtHistory_StorageEvent(
	BdtStack* stack,
	BdtStorage* storage,
	uint32_t eventId,
	void* eventParam,
	const void* userData
);

// 加载节点历史通信信息
uint32_t BdtHistory_StorageLoadAll(
	BdtStorage* storage,
	int64_t historyValidTime, // 历史记录最早保留时间
	BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback,
	const BfxUserData* userData
);

// 增加一个节点信息，保存其相关的连接信息，但不附带保存相关的SN信息
uint32_t BdtHistory_StorageAddPeer(
	BdtStorage* storage,
	const BdtHistory_PeerInfo* history
);

// 设置节点的constInfo，部分情况下节点信息保存时没有ConstInfo
uint32_t BdtHistory_StorageSetConstInfo(
    BdtStorage* storage,
    const BdtHistory_PeerInfo* history
);

// 更新某节点的一条连接信息
uint32_t BdtHistory_StorageUpdateConnect(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const BdtHistory_PeerConnectInfo* connectInfo,
	bool isNew
);

// 更新在某SuperNode上查询到某节点的时间
uint32_t BdtHistory_StorageUpdateSuperNode(
	BdtStorage* storage,
	const BdtPeerid* clientPeerid,
	const BdtPeerid* superNodePeerid,
	int64_t responseTime,
	bool isNew
);

// 更新一个节点通信的AES密钥(访问时间/超时时间/标记等)
uint32_t BdtHistory_StorageUpdateAesKey(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
);

// 加载特定节点的历史记录
uint32_t BdtHistory_StorageLoadPeerHistoryByPeerid(
    BdtStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // 历史记录最早保留时间
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback,
    const BfxUserData* userData
);

// 加载特定节点的TCP连接日志
uint32_t BdtHistory_LoadTunnelHistoryTcpLog(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
	);

// 更新/追加TCP连接日志
uint32_t BdtHistory_UpdateTunnelHistoryTcpLog(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type,
	uint32_t times,
	int64_t timestamp,
	int64_t oldTimestamp,
	bool isNew
	);

// 销毁BdtStorage对象
void BdtHistory_StorageDestroy(BdtStorage* storage);

#endif // __BDT_HISTORY_STORAGE_H__