#ifndef __BDT_SUPERNODE_CLIENT_CALL_SESSION_INL__
#define __BDT_SUPERNODE_CLIENT_CALL_SESSION_INL__
#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include "../BdtCore.h"
#include "./CallSession.h"

typedef struct BdtPeerid BdtPeerid;
typedef struct BdtPeerInfo BdtPeerInfo;
typedef struct BfxUserData BfxUserData;
typedef struct BdtSuperNodeClient BdtSuperNodeClient;
typedef struct BdtSnClient_CallClient BdtSnClient_CallClient;
typedef BdtSnClient_CallClient CallClient;
typedef struct BdtSnClient_CallSession BdtSnClient_CallSession;

typedef struct BdtSnClient_CallSession
{
	BdtSuperNodeClient* owner;
	bool encrypto;
	uint32_t seq;
	BdtPeerid remotePeerid;
	BdtEndpointArray reverseEndpointArray;
	bool alwaysCall;
	BFX_BUFFER_HANDLE payload;

	int64_t endTime;
	Bdt_TimerHelper timer;

	volatile BdtSnClient_CallSessionCallback callback;
	BfxUserData userData;

	size_t snCount;
	CallClient* callClients;

    Bdt_PackageWithRef* pkgs[2];
    const BdtPeerInfo* localPeerInfo;

	volatile int32_t refCount;
} BdtSnClient_CallSession, CallSession;

static CallSession* CallSessionCreate(
    BdtSuperNodeClient* client,
    const BdtPeerInfo* localPeerInfo,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* reverseEndpointArray,
	size_t reverseEndpointCount,
	const BdtPeerInfo* snPeerInfo,
	uint32_t startSeq,
	bool alwaysCall,
	bool encrypto,
	BdtSnClient_CallSessionCallback callback,
	const BfxUserData* userData,
    Bdt_PackageWithRef** callPkg
);

static uint32_t CallSessionPushUdpPackage(CallSession* session,
	Bdt_SnCallRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	bool* outHandled);

#endif //__BDT_SUPERNODE_CLIENT_CALL_SESSION_INL__