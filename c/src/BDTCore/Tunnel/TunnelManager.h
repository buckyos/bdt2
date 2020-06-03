#ifndef __BDT_TUNNEL_MANAGER_H__
#define __BDT_TUNNEL_MANAGER_H__
#include "../BdtCore.h"
#include "./TunnelContainer.h"

typedef struct Bdt_TunnelContainerManager Bdt_TunnelContainerManager;

Bdt_TunnelContainerManager* Bdt_TunnelContainerManagerCreate(BdtStack* stack);

void Bdt_TunnelContainerManagerDestroy(Bdt_TunnelContainerManager* tunnelManager);

// thread safe
// add ref 
Bdt_TunnelContainer* BdtTunnel_GetContainerByRemotePeerid(
	Bdt_TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote
);

// thread safe
// add ref 
Bdt_TunnelContainer* BdtTunnel_CreateContainer(
	Bdt_TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote
);


#endif //__BDT_TUNNEL_MANAGER_H__
