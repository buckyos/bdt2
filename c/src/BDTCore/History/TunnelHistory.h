#ifndef __BDT_TUNNEL_HISTORY_H__
#define __BDT_TUNNEL_HISTORY_H__
#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "./Updater.h"

typedef struct Bdt_TunnelHistory Bdt_TunnelHistory;

typedef enum Bdt_TunnelHistoryTcpLogType
{
	// ����ʧ��
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_FAILED = 0,
	// �������ӳɹ�
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_ACTIVE = 1,
	// �������ӳɹ�(ǰ�������ְ�����ɹ��������޷���¼)
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_PASSIVE = 2,

	// <TODO>�����ϲ����Ե�ʱ��Ҫ�������ְ������ݰ�����һ����ȷ������ٴ���
	// ���磺�������ӳɹ����ԳԵ����ְ����ܳɹ����������ԳԵ����ݰ����ܳɹ�/ʧ��
	// ����ʧ��
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_DECRYPT_FAILED = 3,
	// ���ܳɹ�
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_DECRYPT_OK = 4,

	// �յ��߼������
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_FAILED = 5,
	// �յ��߼���ȷ��
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_OK = 6
} Bdt_TunnelHistoryTcpLogType;

typedef struct Bdt_TunnelHistoryTcpLog
{
	// ��־����
	Bdt_TunnelHistoryTcpLogType type;
	// ���������Ĵ���
	uint32_t times;
	// ���һ�η�����ʱ���
	int64_t timestamp;
} Bdt_TunnelHistoryTcpLog;

typedef struct Bdt_TunnelHistoryTcpLogVector
{
	Bdt_TunnelHistoryTcpLog* logs; // FIFO
	size_t size;
	size_t _allocSize;
} Bdt_TunnelHistoryTcpLogVector, tcp_tunnel_log;

typedef struct Bdt_TunnelHistoryTcpLogForRemoteEndpoint
{
	BdtEndpoint remote;
	Bdt_TunnelHistoryTcpLogVector logVector;
} Bdt_TunnelHistoryTcpLogForRemoteEndpoint;

typedef struct Bdt_TunnelHistoryTcpLogForRemoteEndpointVector
{
    Bdt_TunnelHistoryTcpLogForRemoteEndpoint* logsForRemote;
    size_t size;
    size_t _allocSize;
} Bdt_TunnelHistoryTcpLogForRemoteEndpointVector, tcp_tunnel_log_for_remote;

typedef struct Bdt_TunnelHistoryTcpLogForLocalEndpoint
{
    BdtEndpoint local;
    Bdt_TunnelHistoryTcpLogForRemoteEndpointVector logForRemoteVector;
} Bdt_TunnelHistoryTcpLogForLocalEndpoint;
//
//typedef struct Bdt_TunnelHistoryUdpLogForRemoteEndpoint
//{
//    BdtEndpoint remote;
//    int64_t lastConnectTime;
//} Bdt_TunnelHistoryUdpLogForRemoteEndpoint;
//
//typedef struct Bdt_TunnelHistoryUdpLogForRemoteEndpointVector
//{
//    Bdt_TunnelHistoryUdpLogForRemoteEndpoint* logsForRemote;
//    size_t size;
//    size_t _allocSize;
//} Bdt_TunnelHistoryUdpLogForRemoteEndpointVector;

typedef struct Bdt_TunnelHistoryUdpLogForLocalEndpoint
{
    BdtEndpoint local;
    BdtHistory_PeerConnectVector logForRemoteVector;
} Bdt_TunnelHistoryUdpLogForLocalEndpoint;

typedef void (*Bdt_TunnelHistoryOnLoadFromStorage)(Bdt_TunnelHistory* history, void* userData);

Bdt_TunnelHistory* Bdt_TunnelHistoryCreate(
	const BdtPeerid* remotePeerid,
    BdtHistory_PeerUpdater* updater,
	const BdtTunnelHistoryConfig* config, 
	Bdt_TunnelHistoryOnLoadFromStorage loadCallback,
	const BfxUserData* userData
);

int32_t Bdt_TunnelHistoryAddRef(Bdt_TunnelHistory* history);
int32_t Bdt_TunnelHistoryRelease(Bdt_TunnelHistory* history);

uint32_t Bdt_TunnelHistoryAddTcpLog(
	Bdt_TunnelHistory* history,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type
);

uint32_t Bdt_TunnelHistoryGetTcpLog(
	Bdt_TunnelHistory* history, 
	const BdtEndpoint localEndpoint[],
    Bdt_TunnelHistoryTcpLogForLocalEndpoint outLogs[],
	size_t count
);

void Bdt_ReleaseTunnelHistoryTcpLogs(
    Bdt_TunnelHistoryTcpLogForLocalEndpoint logs[],
    size_t count);

// �յ��Զ˷��������ݰ����޷�ȷ��rtoʱ��0
uint32_t Bdt_TunnelHistoryOnPackageFromRemotePeer(
    Bdt_TunnelHistory* history, 
    const BdtPeerInfo* remotePeerInfo,
    const BdtEndpoint* remoteEndpoint,
    const BdtEndpoint* localEndpoint,
    uint32_t rto);

uint32_t Bdt_TunnelHistoryGetUdpLog(
    Bdt_TunnelHistory* history,
    const BdtEndpoint localEndpoint[],
    Bdt_TunnelHistoryUdpLogForLocalEndpoint outLogs[],
    size_t count
);

void Bdt_ReleaseTunnelHistoryUdpLogs(
    const Bdt_TunnelHistoryUdpLogForLocalEndpoint logs[],
    size_t count);

#endif //__BDT_TUNNEL_HISTORY_H__
