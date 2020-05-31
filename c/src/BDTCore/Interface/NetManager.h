#ifndef __BDT_NETWORK_MANAGER_H__
#define __BDT_NETWORK_MANAGER_H__
#include "../BdtCore.h"
#include "../BdtSystemFramework.h"
#include "../Protocol/Package.h"
#include "./UdpInterface.h"
#include "./TcpInterface.h"

typedef struct Bdt_NetListener Bdt_NetListener;
int32_t Bdt_NetListenerAddRef(const Bdt_NetListener* listener);
int32_t Bdt_NetListenerRelease(const Bdt_NetListener* listener);

// thread safe
// no add ref
// should not hold returned pointer beyond Bdt_NetListener instance released
const BdtEndpoint* Bdt_NetListenerGetBoundAddr(
	const Bdt_NetListener* listener,
	size_t* outLen
);
// thread safe
// no add ref
// should not hold returned pointer beyond Bdt_NetListener instance released
const Bdt_UdpInterface* const* Bdt_NetListenerListUdpInterface(
	const Bdt_NetListener* listener,
	size_t* outLength);

// thread safe
// add ref
const Bdt_UdpInterface* Bdt_NetListenerGetUdpInterface(
	const Bdt_NetListener* listener,
	const BdtEndpoint* endpoint
);

// thread safe
// no add ref
// should not hold returned pointer beyond Bdt_NetListener instance released
const Bdt_TcpListener* const* Bdt_NetListenerListTcpListener(
	const Bdt_NetListener* listener,
	size_t* outLength
);

typedef struct Bdt_NetManager Bdt_NetManager;

Bdt_NetManager* Bdt_NetManagerCreate(
	BdtSystemFramework* framework,
	const BdtNetConfig* config);

int32_t Bdt_NetManagerStart(
	Bdt_NetManager* manager,
	const BdtEndpoint* epList,
	size_t length, 
	uint32_t* outResults
);

int32_t Bdt_NetManagerStop(
	Bdt_NetManager* manager
);


uint32_t Bdt_NetManagerCreateTcpInterface(
	Bdt_NetManager* manager,
	const BdtEndpoint* localEndpoint, 
	const BdtEndpoint* remoteEndpoint,
	Bdt_TcpInterface** outInterface
);

// thread safe
// add ref
const Bdt_NetListener* Bdt_NetManagerGetListener(
	const Bdt_NetManager* manager
);


#endif //__BDT_NETWORK_MANAGER_H__