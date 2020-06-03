#ifndef __BDT_EVENT_CENTER_H__
#define __BDT_EVENT_CENTER_H__
#include "./BdtCore.h"

typedef struct Bdt_EventCenter
{
	BdtStack* stack;
} Bdt_EventCenter;

// all inner logic event trigger type BdtPushxxxEvent
void Bdt_EventCenterInit(Bdt_EventCenter* eventCenter, BdtStack* stack);
void Bdt_EventCenterUninit(Bdt_EventCenter* Bdt_EventCenter);

void Bdt_PushPeerFinderEvent(
	Bdt_EventCenter* eventCenter,
	void* from,
	uint32_t eventId,
	void* eventParam
);


void Bdt_PushTunnelContainerEvent(
	Bdt_EventCenter* eventCenter,
	void* from, 
	uint32_t eventId, 
	void* eventParam);


void Bdt_PushSuperNodeClientEvent(
	Bdt_EventCenter* eventCenter,
	void* from,
	uint32_t eventId,
	void* eventParam
);

#endif //__BDT_EVENT_CENTER_H__