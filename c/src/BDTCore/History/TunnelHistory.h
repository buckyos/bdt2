#ifndef __BDT_TUNNEL_HISTORY_H__
#define __BDT_TUNNEL_HISTORY_H__
#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "./Updater.h"

typedef struct Bdt_TunnelHistory Bdt_TunnelHistory;

typedef enum Bdt_TunnelHistoryTcpLogType
{
	// 连接失败
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_FAILED = 0,
	// 主动连接成功
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_ACTIVE = 1,
	// 被动连接成功(前提是握手包解包成功，否则无法记录)
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_PASSIVE = 2,

	// <TODO>决定合并策略的时候要考虑握手包和数据包，进一步明确需求后再处理；
	// 比如：被动连接成功可以吃掉握手包解密成功，但不可以吃掉数据包解密成功/失败
	// 解密失败
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_DECRYPT_FAILED = 3,
	// 解密成功
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_DECRYPT_OK = 4,

	// 收到逻辑错误包
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_FAILED = 5,
	// 收到逻辑正确包
	BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_OK = 6
} Bdt_TunnelHistoryTcpLogType;

typedef struct Bdt_TunnelHistoryTcpLog
{
	// 日志类型
	Bdt_TunnelHistoryTcpLogType type;
	// 连续发生的次数
	uint32_t times;
	// 最后一次发生的时间戳
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

// 收到对端发来的数据包，无法确定rto时置0
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
