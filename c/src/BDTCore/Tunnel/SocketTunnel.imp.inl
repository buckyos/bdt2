#ifndef __BDT_SOCKET_TUNNEL_IMP_INL__
#define __BDT_SOCKET_TUNNEL_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./SocketTunnel.inl"

bool Bdt_SocketTunnelIsUdp(const SocketTunnel* tunnel)
{
	return tunnel->flags & SOCKET_TUNNEL_FLAGS_UDP;
}

bool Bdt_SocketTunnelIsTcp(const SocketTunnel* tunnel)
{
	return tunnel->flags & SOCKET_TUNNEL_FLAGS_TCP;
}

const BdtEndpoint* Bdt_SocketTunnelGetLocalEndpoint(const SocketTunnel* tunnel)
{
	return &(tunnel->localEndpoint);
}

const BdtEndpoint* Bdt_SocketTunnelGetRemoteEndpoint(const SocketTunnel* tunnel)
{
	return &(tunnel->remoteEndpoint);
}

const BdtPeerid* Bdt_SocketTunnelGetRemotePeerid(const SocketTunnel* tunnel)
{
	return &(tunnel->remotePeerid);
}


const char* Bdt_SocketTunnelGetName(const SocketTunnel* tunnel)
{
	return tunnel->name;
}

static int64_t SocketTunnelGetLastRecv(const SocketTunnel* tunnel)
{
	BfxRwLockRLock((BfxRwLock*)&tunnel->lastRecvLock);
	int64_t lastRecv = tunnel->lastRecv;
	BfxRwLockRUnlock((BfxRwLock*)&tunnel->lastRecvLock);

	return lastRecv;
}

static int64_t SocketTunnelUpdateLastRecv(SocketTunnel* tunnel)
{
	int64_t now = BfxTimeGetNow(false);
	BfxRwLockWLock(&tunnel->lastRecvLock);
	int64_t lastRecv = tunnel->lastRecv;
	tunnel->lastRecv = now;
	BfxRwLockWUnlock(&tunnel->lastRecvLock);

	return lastRecv;
}

static uint32_t SocketTunnelGetFlags(const SocketTunnel* tunnel)
{
	return tunnel->flags;
}

#endif //__BDT_SOCKET_TUNNEL_IMP_INL__