#ifndef __BDT_TUNNEL_CONTIANER_H__
#define __BDT_TUNNEL_CONTIANER_H__
#include "../BdtCore.h"
#include "./SocketTunnel.h"
#include "./TcpTunnel.h"
#include "../BdtSystemFramework.h"
#include "../History/TunnelHistory.h"
#include "../Interface/UdpInterface.h"
#include "../Interface/TcpInterface.h"
#include "../Global/Module.h"


typedef struct Bdt_TunnelContainer Bdt_TunnelContainer;
// tunnel container event push to event center
// socket tunnels in tunnel container have changed
#define BDT_EVENT_DEFAULT_TUNNEL_CHANGED		1

int32_t Bdt_TunnelContainerAddRef(Bdt_TunnelContainer* container);
int32_t Bdt_TunnelContainerRelease(Bdt_TunnelContainer* container);
const char* Bdt_TunnelContainerGetName(Bdt_TunnelContainer* container);


const BdtPeerid* BdtTunnel_GetRemotePeerid(Bdt_TunnelContainer* container);
const BdtPeerid* BdtTunnel_GetLocalPeerid(Bdt_TunnelContainer* container);
Bdt_TunnelHistory* BdtTunnel_GetHistory(Bdt_TunnelContainer* container);
uint32_t BdtTunnel_GetNextSeq(Bdt_TunnelContainer* container);


// thread safe
uint32_t BdtTunnel_ConnectConnection(
	Bdt_TunnelContainer* container,
	BdtConnection* connection
);

uint32_t BdtTunnel_GetBestTcpReverseConnectEndpoint(
	Bdt_TunnelContainer* container, 
	const BdtEndpoint* remote, 
	BdtEndpoint* local
);

// thread safe
uint32_t BdtTunnel_PushUdpPackage(
	Bdt_TunnelContainer* container,
	Bdt_Package* package,
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote,
	bool* outHandled
);

uint32_t BdtTunnel_PushSessionPackage(
	Bdt_TunnelContainer* container,
	const Bdt_Package* package,
	bool* handled
);

uint32_t BdtTunnel_OnTcpFirstPackage(
	Bdt_TunnelContainer* container, 
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package* package
);

uint32_t BdtTunnel_OnTcpFirstResp(
	Bdt_TunnelContainer* container,
	Bdt_TcpTunnel* tunnel, 
	Bdt_TcpInterface* tcpInterface,
	const BdtPeerInfo* remoteInfo
);

typedef struct Bdt_TunnelIterator Bdt_TunnelIterator;
// thread safe but should not enter another lock or do long time operations between enumation
// no add ref
Bdt_SocketTunnel* BdtTunnel_EnumTunnels(
	Bdt_TunnelContainer* container, 
	Bdt_TunnelIterator** outIter);
// thread safe but should not enter another lock or do long time operations between enumation
// no add ref
Bdt_SocketTunnel* BdtTunnel_EnumTunnelsNext(
	Bdt_TunnelIterator* iter);
// thread safe but should not enter another lock or do long time operations between enumation
void BdtTunnel_EnumTunnelsFinish(
	Bdt_TunnelIterator* iter);

// thread safe
// add ref
Bdt_SocketTunnel* BdtTunnel_GetTunnelByEndpoint(
	Bdt_TunnelContainer* container, 
	const BdtEndpoint* localEndpoint, 
	const BdtEndpoint* remoteEndpoint);

// thread safe
// add ref
Bdt_SocketTunnel* BdtTunnel_GetDefaultTunnel(Bdt_TunnelContainer* container);
// thread safe
void BdtTunnel_OnFoundPeer(Bdt_TunnelContainer* container, const BdtPeerInfo* remotePeer);


struct Bdt_TunnelBuilder;
uint32_t Bdt_TunnelContainerCancelBuild(
	Bdt_TunnelContainer* container,
	struct Bdt_TunnelBuilder* builder
);

#define BDT_TUNNEL_SEND_FLAGS_DEFAULT_ONLY		0
#define BDT_TUNNEL_SEND_FLAGS_BUILD				(1<<0)
#define BDT_TUNNEL_SEND_FLAGS_UDP_ONLY			(1<<1)

typedef struct BdtTunnel_SendParams
{
	uint32_t flags;
	const BdtBuildTunnelParams* buildParams;
} BdtTunnel_SendParams;
// thread safe
// when buildParams is NULL, if no socket tunnel exits, packages will not send
// when buildParams is set, if no socket tunnel exists, tunnel builder will be created to build socket tunnels, 
//		and try to merge packages into hole punching packages
uint32_t BdtTunnel_Send(
	Bdt_TunnelContainer* container, 
	const Bdt_Package** packages, 
	size_t* inoutCount, 
	const BdtTunnel_SendParams* sendParams);


void BdtTunnel_OnSnCalled(BdtStack* stack,const Bdt_SnCalledPackage* pCalled);


typedef struct Bdt_TunnelEncryptOptions
{
	uint8_t key[BDT_AES_KEY_LENGTH];
	bool exchange;
	const BdtPeerConstInfo* remoteConst;
	const BdtPeerSecret* localSecret;
} Bdt_TunnelEncryptOptions;

uint32_t BdtTunnel_GetEnryptOptions(
	Bdt_TunnelContainer* container,
	Bdt_TunnelEncryptOptions* outOptions
);

uint32_t BdtTunnel_SetCachedResponse(
	Bdt_TunnelContainer* container, 
	uint32_t seq, 
	const Bdt_PackageWithRef* resp
);

uint32_t BdtTunnel_SendFirstTcpPackage(
	Bdt_StackTls* tls,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package** packages,
	uint8_t count,
	const Bdt_TunnelEncryptOptions* encrypt);
uint32_t BdtTunnel_SendFirstTcpResp(
	Bdt_StackTls* tls,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package** packages,
	uint8_t count
);

void Bdt_BuildTunnelParamsClone(const BdtBuildTunnelParams* src, BdtBuildTunnelParams* dst);
void Bdt_BuildTunnelParamsUninit(BdtBuildTunnelParams* params);

#endif //__BDT_TUNNEL_CONTIANER_H__
