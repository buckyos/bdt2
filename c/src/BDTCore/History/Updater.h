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

// ��ѯָ��peerid�ڵ����ʷ��Ϣ��¼
// ������ڴ�����������ʷ��¼ֱ��ͬ���ص�
// <TODO>callback�ص���peerHistory�ṹ����Ȼ��֤���������ڣ������а�����vector�ṹ��
// �и�����Ϊ���·����ڴ浼�µ�ַ�ı䣬Ӧ��������֤��or ֻ��copy��
typedef void(*BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK)(const BdtHistory_PeerInfo* peerHistory, void* userData);
uint32_t BdtHistory_PeerUpdaterFindByPeerid(BdtHistory_PeerUpdater* updater,
	const BdtPeerid* peerid,
	bool searchInStorage,
	BDT_HISTORY_PEER_UPDATER_FIND_CALLBACK callback,
	const BfxUserData* userData);

// ��SN��Ӧping
uint32_t BdtHistory_PeerUpdaterOnSuperNodePingResponse(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* snPeerInfo);

// ��SN�鵽PEER��Ϣʱ����
uint32_t BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* peerInfo,
	const BdtPeerInfo* snPeerInfo);

// �յ��Զ˷��������ݰ����޷�ȷ��rtoʱ��0
uint32_t BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
	BdtHistory_PeerUpdater* updater,
	const BdtPeerid* remotePeerid,
	const BdtPeerInfo* remotePeerInfo, 
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint,
	BdtHistory_PeerConnectType connectType,
	uint32_t rto);

// ͳ�ƴ���Ч��
// ��ʼ�ͶԶ˴�������
BdtHistory_PeerUpdaterTransferTask* BdtHistory_PeerUpdaterOnStartTransfer(BdtHistory_PeerUpdater* updater,
	const BdtPeerInfo* remotePeerInfo,
	const BdtEndpoint* remoteEndpoint,
	const BdtEndpoint* localEndpoint);

// ��ʼ���ԺͶԶ˵���Ӧʱ��
void BdtHistory_PeerUpdaterOnTransferRtoTestBegin(BdtHistory_PeerUpdaterTransferTask* task);
// �������ԺͶԶ˵���Ӧʱ��
void BdtHistory_PeerUpdaterOnTransferRtoTestEnd(BdtHistory_PeerUpdaterTransferTask* task);

// �����ݷ���
void BdtHistory_PeerUpdaterOnTransferDataSend(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes);
// ������ͣ�����߼�������ͣ���ͻ��߷��ͻ��������겢ȫ�����Է�ȷ��
void BdtHistory_PeerUpdaterOnTransferSendPause(BdtHistory_PeerUpdaterTransferTask* task);

// �����ݽ���
void BdtHistory_PeerUpdaterOnTransferDataRecv(BdtHistory_PeerUpdaterTransferTask* task,
	uint32_t bytes);
// ������ͣ�����߼�������ͣ����
void BdtHistory_PeerUpdaterOnTransferRecvPause(BdtHistory_PeerUpdaterTransferTask* task);

// ���ݴ���ֹͣ
void BdtHistory_PeerUpdaterOnTransferEnd(BdtHistory_PeerUpdaterTransferTask* task);

// get storage
BdtStorage* BdtHistory_PeerUpdaterGetStorage(BdtHistory_PeerUpdater* updater);

// ��ʷ���ݼ������
void BdtHistory_PeerUpdaterOnLoadDone(
	BdtHistory_PeerUpdater* updater,
	BdtStorage* storage,
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount
);

// �Ӽ�PeerHistory�����ü�������ֻ�ܴ���updaterģ������Ķ���
int32_t BdtHistory_PeerHistoryAddRef(const BdtHistory_PeerInfo* peerHistory);
int32_t BdtHistory_PeerHistoryRelease(BdtHistory_PeerUpdater* updater, const BdtHistory_PeerInfo* peerHistory);

#endif // __BDT_HISTORY_PEER_UPDATER_H__