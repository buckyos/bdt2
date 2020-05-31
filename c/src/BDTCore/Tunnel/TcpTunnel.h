#ifndef __BDT_TCP_TUNNEL_H__
#define __BDT_TCP_TUNNEL_H__
#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "./SocketTunnel.h"
#include "../Interface/TcpInterface.h"

typedef enum Bdt_TcpTunnelState
{
	BDT_TCP_TUNNEL_STATE_UNKNOWN = 0,
	BDT_TCP_TUNNEL_STATE_CONNECTING = 1, 
	BDT_TCP_TUNNEL_STATE_ALIVE = 4,
	BDT_TCP_TUNNEL_STATE_DEAD = 5,
} Bdt_TcpTunnelState;

typedef struct Bdt_TcpTunnel Bdt_TcpTunnel;

int32_t Bdt_TcpTunnelAddRef(Bdt_TcpTunnel* tunnel);
int32_t Bdt_TcpTunnelRelease(Bdt_TcpTunnel* tunnel);
Bdt_TcpTunnelState Bdt_TcpTunnelGetState(Bdt_TcpTunnel* tunnel);
void Bdt_TcpTunnelMarkDead(Bdt_TcpTunnel* tunnel);

#endif //__BDT_TCP_TUNNEL_H__


