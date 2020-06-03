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
	BdtEndpoint endpoint; // �Զ˵�ַ
	const BdtEndpoint* localEndpoint; // ���ص�ַ
	int64_t foundTime; // ���Ӵ���ʱ�䣬���ǵ�һ������ʱ��
	int64_t lastConnectTime; // ���һ������ʱ��
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
����IO��������ֵ:
������ɣ�BFX_RESULT_SUCCESS
�Ŷӵȴ���ɣ�BFX_RESULT_PENDING
�ظ�������BFX_RESULT_ALREADY_OPERATION
����ֵΪʧ��
*/

typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_ALL)(
	BdtStorage* storage,
	int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��
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

// �����ض��ڵ����ʷ��¼
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_PEER_HISTORY_BY_PEERID)(
    BdtStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��
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

// ����TCP��ʷ������־
typedef uint32_t(*BDT_HISTORY_STORAGE_PROC_LOAD_TUNNEL_HISTORY_TCP_LOG)(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
	);

// ����TCP��ʷ������־
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
	// ���ؽڵ���ʷͨ����Ϣ
	BDT_HISTORY_STORAGE_PROC_LOAD_ALL loadAll;

	// ����һ���ڵ���Ϣ����������ص�������Ϣ����������������ص�SN��Ϣ
	BDT_HISTORY_STORAGE_PROC_ADD_PEER_HISTORY addPeerHistory;

    // ���ýڵ��constInfo����������½ڵ���Ϣ����ʱû��ConstInfo
    BDT_HISTORY_STORAGE_PROC_SET_CONST_INFO setPeerConstInfo;

	// ����ĳ�ڵ��һ��������Ϣ
	BDT_HISTORY_STORAGE_PROC_UPDATE_CONNECT_HISTORY updateConnectHistory;

	// ������ĳSuperNode�ϲ�ѯ��ĳ�ڵ��ʱ��
	BDT_HISTORY_STORAGE_PROC_UPDATE_SUPERNODE_HISTORY updateSuperNodeHistory;

	// ����һ���ڵ�ͨ�ŵ�AES��Կ(����ʱ��/��ʱʱ��/��ǵ�)
	BDT_HISTORY_STORAGE_PROC_UPDATE_AES_KEY updateAesKey;

    // �����ض��ڵ����ʷ��¼
    BDT_HISTORY_STORAGE_PROC_LOAD_PEER_HISTORY_BY_PEERID loadPeerHistoryByPeerid;

	// �����ض��ڵ��TCP������־
	BDT_HISTORY_STORAGE_PROC_LOAD_TUNNEL_HISTORY_TCP_LOG loadTunnelHistoryTcpLog;

	// ����/׷��TCP������־
	BDT_HISTORY_STORAGE_PROC_UPDATE_TUNNEL_HISTORY_TCP_LOG updateTunnelHistoryTcpLog;

	// ����BdtStorage����
	BDT_HISTORY_STORAGE_PROC_DESTROY_SELF destroy;

} BdtStorage;

void BdtHistory_StorageEvent(
	BdtStack* stack,
	BdtStorage* storage,
	uint32_t eventId,
	void* eventParam,
	const void* userData
);

// ���ؽڵ���ʷͨ����Ϣ
uint32_t BdtHistory_StorageLoadAll(
	BdtStorage* storage,
	int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��
	BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback,
	const BfxUserData* userData
);

// ����һ���ڵ���Ϣ����������ص�������Ϣ����������������ص�SN��Ϣ
uint32_t BdtHistory_StorageAddPeer(
	BdtStorage* storage,
	const BdtHistory_PeerInfo* history
);

// ���ýڵ��constInfo����������½ڵ���Ϣ����ʱû��ConstInfo
uint32_t BdtHistory_StorageSetConstInfo(
    BdtStorage* storage,
    const BdtHistory_PeerInfo* history
);

// ����ĳ�ڵ��һ��������Ϣ
uint32_t BdtHistory_StorageUpdateConnect(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const BdtHistory_PeerConnectInfo* connectInfo,
	bool isNew
);

// ������ĳSuperNode�ϲ�ѯ��ĳ�ڵ��ʱ��
uint32_t BdtHistory_StorageUpdateSuperNode(
	BdtStorage* storage,
	const BdtPeerid* clientPeerid,
	const BdtPeerid* superNodePeerid,
	int64_t responseTime,
	bool isNew
);

// ����һ���ڵ�ͨ�ŵ�AES��Կ(����ʱ��/��ʱʱ��/��ǵ�)
uint32_t BdtHistory_StorageUpdateAesKey(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
);

// �����ض��ڵ����ʷ��¼
uint32_t BdtHistory_StorageLoadPeerHistoryByPeerid(
    BdtStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback,
    const BfxUserData* userData
);

// �����ض��ڵ��TCP������־
uint32_t BdtHistory_LoadTunnelHistoryTcpLog(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
	);

// ����/׷��TCP������־
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

// ����BdtStorage����
void BdtHistory_StorageDestroy(BdtStorage* storage);

#endif // __BDT_HISTORY_STORAGE_H__