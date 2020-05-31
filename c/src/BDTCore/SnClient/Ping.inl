#ifndef __BDT_SUPERNODE_CLIENT_PING_INL__
#define __BDT_SUPERNODE_CLIENT_PING_INL__
#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include <BuckyBase/BuckyBase.h>

typedef struct BdtStack BdtStack;
typedef struct BdtEndpoint BdtEndpoint;
typedef struct Bdt_UdpInterface Bdt_UdpInterface;
typedef struct Bdt_SnPingRespPackage Bdt_SnPingRespPackage;
typedef struct BdtPeerInfo BdtPeerInfo;
typedef struct BdtSuperNodeClient BdtSuperNodeClient;
typedef struct PingManager PingManager;
typedef struct PingClient PingClient;

static PingManager* PingManagerCreate(
	BdtStack* stack, 
	bool encrypto,
	BdtSuperNodeClient* owner);
static void PingManagerDestroy(PingManager* manager);
static uint32_t PingManagerPingResponsed(PingManager* mgr,
	Bdt_SnPingRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	bool* outHandled);

static void PingManagerOnTimer(PingManager* manager);
static int PingManagerGetOnlineSuperNodes(PingManager* manager, const BdtPeerInfo** snPeerInfos, int snPeerInfosSize);

#endif // __BDT_SUPERNODE_CLIENT_PING_INL__