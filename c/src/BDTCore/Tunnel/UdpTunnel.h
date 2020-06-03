#ifndef __BDT_UDP_TUNNEL_H__
#define __BDT_UDP_TUNNEL_H__
#include "../BdtCore.h"

typedef struct Bdt_UdpTunnel Bdt_UdpTunnel;

int32_t Bdt_UdpTunnelAddRef(Bdt_UdpTunnel* tunnel);
int32_t Bdt_UdpTunnelRelease(Bdt_UdpTunnel* tunnel);

#endif //__BDT_UDP_TUNNEL_H__
