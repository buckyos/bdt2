#ifndef __BDT_UDP_TUNNEL_IMP_INL__
#define __BDT_UDP_TUNNEL_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include <assert.h>
#include "../BdtSystemFramework.h"
#include "./UdpTunnel.inl"
#include "./TunnelContainer.inl"

struct Bdt_UdpTunnel
{
	DECLARE_SOCKET_TUNNEL()
	int32_t refCount;
	BdtSystemFramework* framework;
	Bdt_StackTls* tls;
	const Bdt_UdpInterface* udpInterface;
	UdpTunnelConfig config;
	Bdt_TunnelContainer* owner;

	int32_t activeCount;
	uint64_t startPingLastRecv;
};


static UdpTunnel* UdpTunnelCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls, 
	Bdt_TunnelContainer* owner, 
	const BdtEndpoint* localEndpoint,
	const BdtPeerid* remote,
	const BdtEndpoint* remoteEndpoint,
	const UdpTunnelConfig* config, 
	int64_t lastRecv
)
{
	UdpTunnel* tunnel = BFX_MALLOC_OBJ(UdpTunnel);
	memset(tunnel, 0, sizeof(UdpTunnel));
	tunnel->flags = SOCKET_TUNNEL_FLAGS_UDP;
	tunnel->refCount = 1;
	tunnel->framework = framework;
	tunnel->tls = tls;
	// tofix: loop ref; should release in close method!!!
	Bdt_TunnelContainerAddRef(owner);
	tunnel->owner = owner;
	tunnel->config = *config;
	tunnel->localEndpoint = *localEndpoint;
	tunnel->remotePeerid = *remote;
	tunnel->remoteEndpoint = *remoteEndpoint;

	tunnel->lastRecv = lastRecv;
	BfxRwLockInit(&tunnel->lastRecvLock);
	
	const char* prename = "UdpTunnel";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(localEndpoint, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remoteEndpoint, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	tunnel->name = name;

	return tunnel;
}

static void UdpTunnelSetInterface(UdpTunnel* tunnel, const Bdt_UdpInterface* udpInterface)
{
	Bdt_UdpInterfaceAddRef(udpInterface);
	tunnel->udpInterface = udpInterface;
}

int32_t Bdt_UdpTunnelAddRef(Bdt_UdpTunnel* tunnel)
{
	return BfxAtomicInc32(&tunnel->refCount);
}


static void UdpTunnelUninit(UdpTunnel* tunnel)
{
	BfxRwLockDestroy(&tunnel->lastRecvLock);
	if (tunnel->udpInterface)
	{
		Bdt_UdpInterfaceRelease(tunnel->udpInterface);
		tunnel->udpInterface = NULL;
	}
	BFX_FREE(tunnel->name);
	tunnel->name = NULL;
}

int32_t Bdt_UdpTunnelRelease(Bdt_UdpTunnel* tunnel)
{
	int32_t refCount = BfxAtomicDec32(&tunnel->refCount);
	if (refCount <= 0)
	{
		UdpTunnelUninit(tunnel);
		BFX_FREE(tunnel);
		return 0;
	}
	return refCount;
}


static uint32_t UdpTunnelSend(
	UdpTunnel* tunnel,
	const Bdt_Package** packages,
	size_t* inoutCount,
	const Bdt_TunnelEncryptOptions* encrypt
)
{
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(tunnel->tls);
	size_t encodeLength = sizeof(tlsData->udpEncodeBuffer);
	uint32_t result = BFX_RESULT_SUCCESS;
	if (encrypt->exchange)
	{
		result = Bdt_BoxEncryptedUdpPackageStartWithExchange(
			packages,
			*inoutCount, 
			NULL, 
			encrypt->key,
			encrypt->remoteConst->publicKeyType,
			(const uint8_t*)&encrypt->remoteConst->publicKey,
			encrypt->localSecret->secretLength,
			(const uint8_t*)&encrypt->localSecret->secret,
			tlsData->udpEncodeBuffer, 
			&encodeLength
		);
	}
	else
	{
		result = Bdt_BoxEncryptedUdpPackage(
			packages,
			*inoutCount, 
			NULL, 
			encrypt->key,
			tlsData->udpEncodeBuffer,
			&encodeLength
		);
	}
	if (result)
	{
		BLOG_DEBUG("%s box udp package failed for %u", tunnel->name, result);
		*inoutCount = 0;
		return result;
	}
	result = BdtUdpSocketSendTo(
		tunnel->framework,
		Bdt_UdpInterfaceGetSocket(tunnel->udpInterface),
		&(tunnel->remoteEndpoint),
		tlsData->udpEncodeBuffer,
		encodeLength);
	if (result)
	{
		*inoutCount = 0;
		BLOG_DEBUG("%s send package failed for %u", tunnel->name, result);
	}
	return result;
}


static void BFX_STDCALL UdpTunnelAsRefUserDataAddRef(void* userData)
{
	Bdt_UdpTunnelAddRef((UdpTunnel*)userData);
}

static void BFX_STDCALL UdpTunnelAsRefUserDataRelease(void* userData)
{
	Bdt_UdpTunnelRelease((UdpTunnel*)userData);
}

static void UdpTunnelAsRefUserData(UdpTunnel* tunnel, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = UdpTunnelAsRefUserDataAddRef;
	userData->lpfnReleaseUserData = UdpTunnelAsRefUserDataRelease;
	userData->userData = tunnel;
}


static void BFX_STDCALL UdpTunnelOnPingTimeout(BDT_SYSTEM_TIMER timer, void* userData)
{
	UdpTunnel* tunnel = (UdpTunnel*)userData;
	if (tunnel->activeCount <= 0)
	{
		return ;
	}
	uint64_t now = BfxTimeGetNow(false) / 1000;
	uint64_t lastRecv = SocketTunnelGetLastRecv((SocketTunnel*)tunnel);

	if (lastRecv != tunnel->startPingLastRecv 
		&& (now - lastRecv) > tunnel->config.pingOvertime)
	{
		TunnelContainerOnTunnelDead(tunnel->owner, (SocketTunnel*)tunnel, tunnel->startPingLastRecv);
		return ;
	}
	Bdt_PingTunnelPackage ping;
	Bdt_PingTunnelPackageInit(&ping);
	const Bdt_Package* pkg = (Bdt_Package*)&ping;
	Bdt_TunnelEncryptOptions encrypt;
	BdtTunnel_GetEnryptOptions(tunnel->owner, &encrypt);
	size_t count = 1;
	uint32_t result = UdpTunnelSend(tunnel, &pkg, &count, &encrypt);
	//tofix: fill package id 
	BLOG_DEBUG("%s send ping", tunnel->name);

	BfxUserData udTimer;
	UdpTunnelAsRefUserData(tunnel, &udTimer);
	BdtCreateTimeout(
		tunnel->framework,
		UdpTunnelOnPingTimeout,
		&udTimer,
		tunnel->config.pingInterval
	);
}

static void UdpTunnelStartActivitor(UdpTunnel* tunnel)
{
	if (BfxAtomicDec32(&tunnel->activeCount) != 1)
	{
		return;
	}
	tunnel->startPingLastRecv = SocketTunnelGetLastRecv((SocketTunnel*)tunnel);
	BfxUserData userData;
	UdpTunnelAsRefUserData(tunnel, &userData);
	BdtCreateTimeout(
		tunnel->framework,
		UdpTunnelOnPingTimeout,
		&userData,
		tunnel->config.pingInterval
	);
}

static void UdpTunnelStopActivitor(UdpTunnel* tunnel)
{
	BfxAtomicDec32(&tunnel->activeCount);
}

#endif //__BDT_UDP_TUNNEL_IMP_INL__
