#ifndef __BDT_TCP_TUNNEL_INL__
#define __BDT_TCP_TUNNEL_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./TcpTunnel.h"
#include "../Interface/NetManager.h"
#include "../Global/Module.h"
#include "../PeerFinder.h"

typedef Bdt_TcpTunnel TcpTunnel;
typedef Bdt_TcpTunnelState TcpTunnelState;

static TcpTunnel* TcpTunnelCreate(
	BdtSystemFramework* framework,
	Bdt_NetManager* netManager,
	Bdt_StackTls* tls,
	BdtPeerFinder* peerFinder,
	struct Bdt_TunnelContainer* owner,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	const BdtTcpTunnelConfig* config
);

static uint32_t TcpTunnelConnect(
	TcpTunnel* tunnel, 
	uint32_t seq, 
	const Bdt_Package** packages, 
	size_t* inoutCount, 
	TcpTunnelState* outState);

static uint32_t TcpTunnelBindTcpInterface(TcpTunnel* tunnel,Bdt_TcpInterface* tcpInterface);

static uint32_t TcpTunnelSend(
	TcpTunnel* tunnel,
	const Bdt_Package** packages,
	size_t packageCount
);

static uint32_t TcpTunnelOnTcpFirstResp(
	Bdt_TcpInterfaceOwner* owner, 
	Bdt_TcpInterface* ti, 
	const Bdt_Package* package);

static void TcpTunnelStartActivitor(Bdt_TcpTunnel* tunnel);
static void TcpTunnelStopActivitor(Bdt_TcpTunnel* tunnel);

#endif //__BDT_TCP_TUNNEL_INL__
