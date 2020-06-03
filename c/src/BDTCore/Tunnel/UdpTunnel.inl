#ifndef __BDT_UDP_TUNNEL_INL__
#define __BDT_UDP_TUNNEL_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "UdpTunnel.h"
#include "./SocketTunnel.inl"
#include "../Interface/UdpInterface.h"
#include "../BdtSystemFramework.h"
#include "../Global/Module.h"

typedef Bdt_UdpTunnel UdpTunnel;
typedef struct Bdt_TunnelContainer Bdt_TunnelContainer;

static UdpTunnel* UdpTunnelCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls,
	Bdt_TunnelContainer* owner,
	const BdtEndpoint* localEndpoint,
	const BdtPeerid* remote,
	const BdtEndpoint* remoteEndpoint,
	const UdpTunnelConfig* config, 
	int64_t lastRecv
);

static void UdpTunnelSetInterface(
	UdpTunnel* tunnel, 
	const Bdt_UdpInterface* udpInterface);

static uint32_t UdpTunnelSend(
	UdpTunnel* tunnel,
	const Bdt_Package** packages,
	size_t* inoutCount,
	const Bdt_TunnelEncryptOptions* encrypt
);

static void UdpTunnelStartActivitor(Bdt_UdpTunnel* tunnel);
static void UdpTunnelStopActivitor(Bdt_UdpTunnel* tunnel);

#endif //__BDT_UDP_TUNNEL_INL__
