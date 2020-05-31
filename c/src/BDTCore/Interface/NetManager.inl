#ifndef __BDT_NETMANAGER_INL__
#define __BDT_NETMANAGER_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__
#include "./NetManager.h"
#include "./TcpInterface.inl"
#include "./UdpInterface.inl"

struct Bdt_NetManager 
{
	BdtSystemFramework* framework;
	const BdtNetConfig* config;
	Bdt_NetListener* listener;

	// just to name tcp interface local id
	int32_t localTcpId;
};
typedef Bdt_NetManager NetManager;

struct BdtNetModifier 
{
	void* resolved;
};
typedef BdtNetModifier NetModifier;

// thread safe
// add ref
static Bdt_TcpInterface* NetManagerAcceptTcpSocket(
	NetManager* manager,
	BDT_SYSTEM_TCP_SOCKET socket,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint
);

#endif //__BDT_NETMANAGER_INL__
