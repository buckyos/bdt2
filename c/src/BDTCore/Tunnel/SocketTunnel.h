#ifndef __BDT_SOCKET_TUNNEL_H__
#define __BDT_SOCKET_TUNNEL_H__
#include "../BdtCore.h"

#define BDT_SOCKET_TUNNEL_ADDREF(tunnel) \
do{if (Bdt_SocketTunnelIsUdp(tunnel)) Bdt_UdpTunnelAddRef((Bdt_UdpTunnel*)tunnel);\
else if (Bdt_SocketTunnelIsTcp(tunnel)) Bdt_TcpTunnelAddRef((Bdt_TcpTunnel*)tunnel);}while(false)

#define BDT_SOCKET_TUNNEL_RELEASE(tunnel) \
do{if (!(tunnel)) break; \
if (Bdt_SocketTunnelIsUdp(tunnel)) Bdt_UdpTunnelRelease((Bdt_UdpTunnel*)tunnel);\
else if (Bdt_SocketTunnelIsTcp(tunnel)) Bdt_TcpTunnelRelease((Bdt_TcpTunnel*)tunnel);}while(false)

typedef struct Bdt_SocketTunnel Bdt_SocketTunnel;

bool Bdt_SocketTunnelIsUdp(const Bdt_SocketTunnel* tunnel);
bool Bdt_SocketTunnelIsTcp(const Bdt_SocketTunnel* tunnel);

const BdtEndpoint* Bdt_SocketTunnelGetLocalEndpoint(const Bdt_SocketTunnel* tunnel);
const BdtEndpoint* Bdt_SocketTunnelGetRemoteEndpoint(const Bdt_SocketTunnel* tunnel);
const BdtPeerid* Bdt_SocketTunnelGetRemotePeerid(const Bdt_SocketTunnel* tunnel);

const char* Bdt_SocketTunnelGetName(const Bdt_SocketTunnel* tunnel);


#endif //__BDT_SOCKET_TUNNEL_H__