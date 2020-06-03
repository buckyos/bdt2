#ifndef __BDT_NETMANAGER_IMP_INL__
#define __BDT_NETMANAGER_IMP_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__
#include <SGLib/SGLib.h>
#include "../Stack.h"
#include "./NetManager.inl"

typedef struct UdpInterfaceList
{
	UdpInterface** list;
	size_t size;
	size_t allocSize;
} UdpInterfaceList, udp_interface_list;
BFX_VECTOR_DEFINE_FUNCTIONS(udp_interface_list, UdpInterface*, list, size, allocSize)


typedef struct BoundAddrList {
	BdtEndpoint* list;
	size_t size;
	size_t allocSize;
} BoundAddrList, bound_addr_list;
BFX_VECTOR_DEFINE_FUNCTIONS(bound_addr_list, BdtEndpoint, list, size, allocSize)


typedef struct TcpListenerList {
	TcpListener** list;
	size_t size;
	size_t allocSize;
} TcpListenerList, tcp_listener_list;
#define NO_IMPL_COMPARE(left, right) 0
BFX_VECTOR_DEFINE_FUNCTIONS(tcp_listener_list, TcpListener*, list, size, allocSize)


struct Bdt_NetListener {
	UdpInterfaceList udpList;
	TcpListenerList tcpListenerList;
	BoundAddrList boundAddr;
	int32_t refCount;
	BdtSystemFramework* framework;
};

typedef Bdt_NetListener NetListener;

static NetListener* NetListenerCreate(BdtSystemFramework* framework)
{
	NetListener* listener = BFX_MALLOC_OBJ(NetListener);
	listener->refCount = 1;
	listener->framework = framework;
	bfx_vector_tcp_listener_list_init(&listener->tcpListenerList);
	bfx_vector_udp_interface_list_init(&listener->udpList);
	bfx_vector_bound_addr_list_init(&(listener->boundAddr));
	return listener;
}

static void UpdateBoundAddr(NetListener* listener, const BdtEndpoint* ep)
{
	BdtEndpoint ba = *ep;
	ba.flag &= ~(BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_PROTOCOL_UDP);
	ba.port = 0;
	for (size_t ix = 0; ix < listener->boundAddr.size; ++ix)
	{
		if (!BdtEndpointCompare(listener->boundAddr.list + ix, &ba, true))
		{
			return;
		}
	}
	bfx_vector_bound_addr_list_push_back(&listener->boundAddr, ba);
}

NetManager* Bdt_NetManagerCreate(
	BdtSystemFramework* framework,
	const BdtNetConfig* config
)
{
	NetManager* manager = BFX_MALLOC_OBJ(NetManager);
	memset(manager, 0, sizeof(NetManager));
	manager->config = config;
	manager->framework = framework;
	manager->listener = NetListenerCreate(framework);
	manager->localTcpId = 1;
	return manager;
}


static uint32_t NetListenerAddUdpInterface(
	NetListener* listener,
	const BdtEndpoint* localEndpoint
)
{
	char epstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(localEndpoint, epstr);
	BLOG_INFO("netmanager try open udp socket on %s", epstr);

	UdpInterface* udpInterface = BFX_MALLOC_OBJ(UdpInterface);
	BfxUserData userData = { .userData = udpInterface,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BDT_SYSTEM_UDP_SOCKET socket = BdtCreateUdpSocket(listener->framework, &userData);
	udpInterface->refCount = 1;
	udpInterface->socket = socket;
	udpInterface->framework = listener->framework;
	udpInterface->localEndpoint = *localEndpoint;
	uint32_t result = BdtUdpSocketOpen(listener->framework, socket, localEndpoint);
	if (result)
	{
		BLOG_ERROR("netmanager open udp socket on %s failed for %u", epstr, result);
		BFX_FREE(udpInterface);
		return result;
	}
	bfx_vector_udp_interface_list_push_back(&listener->udpList, udpInterface);
	UpdateBoundAddr(listener, localEndpoint);
	BLOG_INFO("netmanager open udp socket on %s success", epstr);
	return BFX_RESULT_SUCCESS;
}

int32_t Bdt_NetListenerAddRef(const NetListener* listener)
{
	NetListener* l = (NetListener*)listener;
	return BfxAtomicInc32(&l->refCount);
}

int32_t Bdt_NetListenerRelease(const NetListener* listener)
{
	NetListener* l = (NetListener*)listener;
	int32_t ref = BfxAtomicDec32(&l->refCount);
	if (ref <= 0)
	{
		// TODO
	}
	return ref;
}


static uint32_t NetListenerAddTcpListener(
	NetListener* listener,
	const BdtEndpoint* localEndpoint
)
{
	char epstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(localEndpoint, epstr);
	BLOG_INFO("netmanager try listen tcp socket on %s", epstr);
	TcpListener* tcpListener = TcpListenerCreate(listener->framework, localEndpoint);
	uint32_t result = TcpListenerListen(tcpListener);
	if (result)
	{
		BLOG_ERROR("netmanager listen tcp socket on %s failed for %u", epstr, result);
		BFX_FREE(tcpListener);
		return result;
	}
	UpdateBoundAddr(listener, localEndpoint);
	bfx_vector_tcp_listener_list_push_back(&listener->tcpListenerList, tcpListener);
	BLOG_INFO("netmanager listen tcp socket on %s success", epstr);
	return BFX_RESULT_SUCCESS;
}


const UdpInterface* const* Bdt_NetListenerListUdpInterface(const NetListener* listener, size_t* outLength)
{
	*outLength = listener->udpList.size;
	return listener->udpList.list;
}


const TcpListener* const* Bdt_NetListenerListTcpListener(
	const Bdt_NetListener* listener,
	size_t* outLength
)
{
	*outLength = listener->tcpListenerList.size;
	return listener->tcpListenerList.list;
}

const UdpInterface* Bdt_NetListenerGetUdpInterface(
	const NetListener* listener,
	const BdtEndpoint* endpoint
)
{
	for (size_t ix = 0; ix < listener->udpList.size; ++ix)
	{
		UdpInterface* ui = *(listener->udpList.list + ix);
		if (!BdtEndpointCompare(&(ui->localEndpoint), endpoint, false))
		{
			Bdt_UdpInterfaceAddRef(ui);
			return ui;
		}
	}
	return NULL;
}


int32_t Bdt_NetManagerStart(
	NetManager* manager, 
	const BdtEndpoint* epList,
	size_t length,
	uint32_t* outResults
)
{
	for (size_t ix = 0; ix < length; ++ix)
	{
		const BdtEndpoint* ep = epList + ix;
		if (ep->flag & BDT_ENDPOINT_PROTOCOL_UDP)
		{
			outResults[ix] = NetListenerAddUdpInterface(manager->listener, ep);
		}
		else
		{
			outResults[ix] = NetListenerAddTcpListener(manager->listener, ep);
		}
	}
	return BFX_RESULT_SUCCESS;
}

int32_t Bdt_NetManagerStop(
	NetManager* manager
)
{
	//TODO:
	return BFX_RESULT_NOT_IMPL;
}

const NetListener* Bdt_NetManagerGetListener(
	const NetManager* manager
)
{
	Bdt_NetListenerAddRef(manager->listener);
	return manager->listener;
}


static Bdt_TcpInterface* NetManagerAcceptTcpSocket(
	NetManager* manager,
	BDT_SYSTEM_TCP_SOCKET socket,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint
)
{
	int32_t localId = BfxAtomicInc32(&manager->localTcpId);
	Bdt_TcpInterface* tcpInterface = TcpInterfaceCreate(
		manager->framework, 
		localEndpoint, 
		remoteEndpoint, 
		&manager->config->tcp, 
		socket, 
		localId);

	return tcpInterface;
}


uint32_t Bdt_NetManagerCreateTcpInterface(
	NetManager* manager,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TcpInterface** outInterface
)
{
	int32_t localId = BfxAtomicInc32(&manager->localTcpId);
	Bdt_TcpInterface* tcpInterface = TcpInterfaceCreate(
		manager->framework, 
		localEndpoint, 
		remoteEndpoint, 
		&manager->config->tcp, 
		NULL, 
		localId);

	*outInterface = tcpInterface;
	return BFX_RESULT_SUCCESS;
}

const BdtEndpoint* Bdt_NetListenerGetBoundAddr(
	const NetListener* listener,
	size_t* outLen
)
{
	*outLen = listener->boundAddr.size;
	return listener->boundAddr.list;
}


BDT_API(BdtNetModifier*) BdtCreateNetModifier(
	BdtStack* stack
)
{
	return BFX_MALLOC_OBJ(NetModifier);
}

BDT_API(uint32_t) BdtChangeLocalAddress(
	BdtNetModifier* modifier,
	const BdtEndpoint* src,
	const BdtEndpoint* dst)
{
	return BFX_RESULT_NOT_IMPL;
}

BDT_API(uint32_t) BdtAddLocalEndpoint(
	BdtNetModifier* modifier,
	const BdtEndpoint* toAdd
)
{
	return BFX_RESULT_NOT_IMPL;
}

BDT_API(uint32_t) BdtRemoveLocalEndpoint(
	BdtNetModifier* modifier,
	const BdtEndpoint* toRemove
)
{
	return BFX_RESULT_NOT_IMPL;
}

BDT_API(uint32_t) BdtApplyBdtNetModifier(
	BdtStack* stack,
	BdtNetModifier* modifier
)
{
	BfxFree(modifier);
	return BFX_RESULT_NOT_IMPL;
}

#endif //__BDT_NETMANAGER_IMP_INL__