#ifndef __BDT_SUPERNODE_CLIENT_H__
#define __BDT_SUPERNODE_CLIENT_H__

#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "./CallSession.h"
#include "../Interface/UdpInterface.h"

// trigger super node client event by push to event center

//client online 
//
#define BDT_SUPER_NODE_CLIENT_EVENT_ONLINE	1

//client offline
//
#define BDT_SUPER_NODE_CLIENT_EVENT_OFFLINE	2

//called
#define BDT_SUPER_NODE_CLIENT_CALLED 3


typedef struct BdtSuperNodeClient BdtSuperNodeClient;

uint32_t BdtSnClient_Create(
	BdtStack* stack,
	bool encrypto,
	BdtSuperNodeClient** outClient);
void BdtSnClient_Destroy(
	BdtSuperNodeClient* client
);

uint32_t BdtSnClient_Start(BdtSuperNodeClient* client);
uint32_t BdtSnClient_Stop(BdtSuperNodeClient* client);

uint32_t BdtSnClient_UpdateSNList(BdtSuperNodeClient* client, const BdtPeerInfo* snPeerInfos[], int count);

uint32_t BdtSnClient_PushUdpPackage(BdtSuperNodeClient* client,
	const Bdt_Package* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	const uint8_t* aesKey,
	bool* outHandled);

uint32_t BdtSnClient_CreateCallSession(
	BdtSuperNodeClient* client, 
    const BdtPeerInfo* localPeerInfo,
	const BdtPeerid* remotePeerid, 
	const BdtEndpoint* reverseEndpointArray,
	size_t reverseEndpointCount,
	const BdtPeerInfo* snPeerInfo,
	bool alwaysCall,
	bool encrypto,
	BdtSnClient_CallSessionCallback callback,
	const BfxUserData* userData,
	BdtSnClient_CallSession** session,
    Bdt_PackageWithRef** callPkg);

#endif //__BDT_SUPERNODE_CLIENT_H__