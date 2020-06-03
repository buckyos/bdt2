#ifndef __BDT_SOCKET_TUNNEL_INL__
#define __BDT_SOCKET_TUNNEL_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./SocketTunnel.h"
#include "../History/KeyManager.h"

#define SOCKET_TUNNEL_FLAGS_UDP			(1 << 0)
#define SOCKET_TUNNEL_FLAGS_TCP			(1 << 1)

#define SOCKET_TUNNEL_START_ACTIVITOR(tunnel) \
do{if (Bdt_SocketTunnelIsUdp(tunnel)) UdpTunnelStartActivitor((UdpTunnel*)tunnel);\
else if (Bdt_SocketTunnelIsTcp(tunnel)) TcpTunnelStartActivitor((TcpTunnel*)tunnel);}while(false)

#define SOCKET_TUNNEL_STOP_ACTIVITOR(tunnel) \
do{if (Bdt_SocketTunnelIsUdp(tunnel)) UdpTunnelStopActivitor((UdpTunnel*)tunnel);\
else if (Bdt_SocketTunnelIsTcp(tunnel)) TcpTunnelStopActivitor((TcpTunnel*)tunnel);}while(false)


#define DECLARE_SOCKET_TUNNEL() \
	uint32_t flags;\
	BdtPeerid remotePeerid;\
	BdtEndpoint localEndpoint;\
	BdtEndpoint remoteEndpoint;\
	char* name;\
	int64_t lastRecv;\
	BfxRwLock lastRecvLock; \


typedef struct Bdt_SocketTunnel
{
	DECLARE_SOCKET_TUNNEL()
} Bdt_SocketTunnel;

typedef Bdt_SocketTunnel SocketTunnel;


static int64_t SocketTunnelGetLastRecv(const SocketTunnel* tunnel);
// return old last recv
static int64_t SocketTunnelUpdateLastRecv(SocketTunnel* tunnel);

static uint32_t SocketTunnelGetFlags(const SocketTunnel* tunnel);

#endif //__BDT_SOCKET_TUNNEL_INL__
