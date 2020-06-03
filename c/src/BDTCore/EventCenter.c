#include <assert.h>
#include "./EventCenter.h"
#include "./Stack.h"
#include "./SnClient/Client.h"
#include "./Tunnel/TunnelContainer.h"
#include "./Tunnel/TunnelManager.h"
#include "BDTCore/Connection/Connection.h"
#include "./Connection/ConnectionManager.h"

void Bdt_EventCenterInit(Bdt_EventCenter* eventCenter, BdtStack* stack)
{
	eventCenter->stack = stack;
}

void Bdt_EventCenterUninit(Bdt_EventCenter* eventCenter)
{
	eventCenter->stack = NULL;
}

typedef struct BdtConnectionVector
{
	BdtConnection** list;
	size_t size;
	size_t allocSize;
} BdtConnectionVector, connection_vector;
BFX_VECTOR_DEFINE_FUNCTIONS(connection_vector, BdtConnection*, list, size, allocSize)

void Bdt_PushTunnelContainerEvent(
	Bdt_EventCenter* eventCenter,
	void* from,
	uint32_t eventId,
	void* eventParam)
{
	Bdt_TunnelContainer* tunnel = (Bdt_TunnelContainer*)from;
	if (eventId == BDT_EVENT_DEFAULT_TUNNEL_CHANGED)
	{
		
	}
}


void Bdt_PushPeerFinderEvent(
	Bdt_EventCenter* eventCenter,
	void* from,
	uint32_t eventId,
	void* eventParam
)
{
	BdtPeerFinder* peerFinder = (BdtPeerFinder*)from;
	if (eventId == BDT_PEER_FINDER_EVENT_UPDATE_CACHE)
	{
		const BdtPeerid* peerid = (const BdtPeerid*)eventParam;
		const BdtPeerInfo* peerInfo = BdtPeerFinderGetCached(peerFinder, peerid);
		assert(peerInfo);
		if (peerInfo)
		{
			Bdt_TunnelContainer* container = BdtTunnel_GetContainerByRemotePeerid(
				BdtStackGetTunnelManager(eventCenter->stack), peerid);
			if (container)
			{
				BdtTunnel_OnFoundPeer(container, peerInfo);
				Bdt_TunnelContainerRelease(container);
			}
		}
	}
}


void Bdt_PushSuperNodeClientEvent(
	Bdt_EventCenter* eventCenter,
	void* from,
	uint32_t eventId,
	void* eventParam
)
{
	if (eventId == BDT_SUPER_NODE_CLIENT_EVENT_ONLINE)
	{
		const BdtPeerInfo* snPeerInfo = (const BdtPeerInfo*)from;
		
	}
	else if (eventId == BDT_SUPER_NODE_CLIENT_EVENT_OFFLINE)
	{
		const BdtPeerInfo* snPeerInfo = (const BdtPeerInfo*)from;

	}
	else if (eventId == BDT_SUPER_NODE_CLIENT_CALLED)
	{
		//const BdtPeerInfo* peerInfo = (const BdtPeerInfo*)from;
		Bdt_SnCalledPackage* calledReq = (Bdt_SnCalledPackage*)eventParam;
		BdtTunnel_OnSnCalled(eventCenter->stack,calledReq);
	}
}