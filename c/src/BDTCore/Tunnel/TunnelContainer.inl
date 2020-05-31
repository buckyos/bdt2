#ifndef __BDT_TUNNEL_CONTIANER_INL__
#define __BDT_TUNNEL_CONTIANER_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./TunnelContainer.h"
#include "../Connection/ConnectionManager.h"
#include "./TunnelBuilder.inl"
#include "./SocketTunnel.inl"

typedef Bdt_TunnelContainer TunnelContainer;

static TunnelContainer* TunnelContainerCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls,
	const BdtPeerid* localPeerid,
	const BdtPeerSecret* localSecret,
	Bdt_NetManager* netManager,
	Bdt_EventCenter* eventCenter,
	BdtPeerFinder* peerFinder,
	BdtHistory_KeyManager* keyManager,
	BdtSuperNodeClient* snClient,
	Bdt_ConnectionManager* connectionManager, 
    BdtHistory_PeerUpdater* updater,
	const BdtPeerid* remote,
	const BdtTunnelConfig* config
);

static const BdtTunnelConfig* TunnelContainerGetConfig(Bdt_TunnelContainer* container);

static uint32_t TunnelContainerCreateTunnel(
	TunnelContainer* container,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	uint64_t lastRecv, 
	SocketTunnel** outTunnel
);

static uint32_t TunnelContainerRemoveTunnel(
	TunnelContainer* container, 
	SocketTunnel* tunnel, 
	int64_t lastRecv
);

static uint32_t TunnelContainerSendFromTunnel(
	TunnelContainer* container,
	SocketTunnel* tunnel,
	const Bdt_Package** packages,
	size_t* inoutCount);

static Bdt_ConnectionManager* TunnelContainerGetConnectionManager(
	TunnelContainer* container
);

static BdtPeerFinder* TunnelContainerGetPeerFinder(TunnelContainer* container);

static BdtSuperNodeClient* TunnelContainerGetSnClient(TunnelContainer* container);

static const Bdt_PackageWithRef* TunnelContainerGetCachedResponse(Bdt_TunnelContainer* container, uint32_t seq);

static uint32_t TunnelContainerOnTcpTunnelReverseConnected(TunnelContainer* container, TcpTunnel* tunnel);

static SocketTunnel* TunnelContainerUpdateDefaultTunnel(TunnelContainer* container);

static void TunnelContainerOnTunnelDead(
	TunnelContainer* container, 
	Bdt_SocketTunnel* tunnel,
	uint64_t lastRecv
);

static uint32_t TunnelContainerResetBuilder(
	Bdt_TunnelContainer* container,
	TunnelBuilder* oldBuilder);

#endif //__BDT_TUNNEL_CONTIANER_INL__