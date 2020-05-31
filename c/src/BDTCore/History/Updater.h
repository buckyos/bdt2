#ifndef __BDT_HISTORY_PEER_UPDATER_H__
#define __BDT_HISTORY_PEER_UPDATER_H__

#include "../BdtCore.h"
#include "./Storage.h"

typedef enum BdtHistory_PeerConnectType
{
	BDT_HISTORY_PEER_CONNECT_TYPE_UDP = 0,
	BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT = 1,
	BDT_HISTORY_PEER_CONNECT_TYPE_TCP_REVERSE = 2
} BdtHistory_PeerConnectType;

typedef struct BdtHistory_PeerUpdater BdtHistory_PeerUpdater;

typedef struct BdtHistory_PeerUpdaterTransferTask BdtHistory_PeerUpdaterTransferTask;

BdtHistory_PeerUpdater* BdtHistory_PeerUpdaterCreate(
	const BdtPeerInfo* localPeerInfo,
	bool isSuperNode,
	const BdtPeerHistoryConfig* config);

void BdtHistory_PeerUpdaterDestroy(BdtHistory_PeerUpdater* updater);

// 查询指定peerid节点的历史信息记录
// 如果在内存中搜索到历史记录直接同步回调
// <TODO>callback回调的peerHistory结构，虽然保证了生命周期，但其中包含有vector结构；
// 有概率因为重新分配内存导致地址改变，应该怎样保证，or 只能copy？
typedef void(*BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK)(const BdtHistory_PeerInfo* peerHistory, void* userData);
uint32_t BdtHistory_PeerUpdaterFindByPeerid(BdtHistory_PeerUpdater* updater,
	const BdtPeerid* peerid,
	bool searchInStorage,
	BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK callback,
	const BfxUserData* userData);

// 从SN响应ping
uint32_t BdtHistory_PeerUpdaterOnSuperNodePingResponse(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* snPeerInfo);

// 从SN查到PEER信息时调用
uint32_t BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* peerInfo,
	const BdtPeerInfo* snPeerInfo);

// 收到对端发来的数据包，无法确定rto时置0
uint32_t BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
	BdtHistory_PeerUpdater* updater,
	const BdtPeerid* remotePeerid,
	const BdtPeerInfo* remotePeerInfo, 
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint,
	BdtHistory_PeerConnectType connectType,
	uint32_t rto);

// 统计传输效率
// 开始和对端传输数据
BdtHistory_PeerUpdaterTransferTask* BdtHistory_PeerUpdaterOnStartTransfer(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* remotePeerInfo,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint);

// 开始测试和对端的响应时间
void BdtHistory_PeerUpdaterOnTransferRtoTestBegin(BdtHistory_PeerUpdaterTransferTask* task);
// 结束测试和对端的响应时间
void BdtHistory_PeerUpdaterOnTransferRtoTestEnd(BdtHistory_PeerUpdaterTransferTask* task);

// 有数据发出
void BdtHistory_PeerUpdaterOnTransferDataSend(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes);
// 发送暂停，因逻辑需求暂停发送或者发送缓冲区发完并全部被对方确认
void BdtHistory_PeerUpdaterOnTransferSendPause(BdtHistory_PeerUpdaterTransferTask* task);

// 有数据接收
void BdtHistory_PeerUpdaterOnTransferDataRecv(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes);
// 接收暂停，因逻辑需求暂停接收
void BdtHistory_PeerUpdaterOnTransferRecvPause(BdtHistory_PeerUpdaterTransferTask* task);

// 数据传输停止
void BdtHistory_PeerUpdaterOnTransferEnd(BdtHistory_PeerUpdaterTransferTask* task);

// get storage
BdtStorage* BdtHistory_PeerUpdaterGetStorage(BdtHistory_PeerUpdater* updater);

// 历史数据加载完成
void BdtHistory_PeerUpdaterOnLoadDone(
	BdtHistory_PeerUpdater* updater,
	BdtStorage* storage,
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount
);

// 加减PeerHistory的引用计数，但只能处理updater模块输出的对象
int32_t BdtHistory_PeerHistoryAddRef(const BdtHistory_PeerInfo* peerHistory);
int32_t BdtHistory_PeerHistoryRelease(BdtHistory_PeerUpdater* updater, const BdtHistory_PeerInfo* peerHistory);

#endif // __BDT_HISTORY_PEER_UPDATER_H__