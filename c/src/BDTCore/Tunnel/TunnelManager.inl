#ifndef __BDT_TUNNEL_MANAGER_INL__
#define __BDT_TUNNEL_MANAGER_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__
#include "./TunnelManager.h"
#include "../Stack.h"
#include "./TunnelContainer.inl"

typedef Bdt_TunnelContainerManager TunnelContainerManager;

// not thread safe
static void TunnelContainerManagerInit(TunnelContainerManager* tunnelManager, BdtStack* stack);
// not thread safe
static void TunnelContainerManagerUninit(TunnelContainerManager* tunnelManager);

// thread safe
static void TunnelContainerManagerRemoveTunnelContainer(
	TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote
);


#endif //__BDT_TUNNEL_MANAGER_INL__
