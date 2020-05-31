#ifndef __BDT_SUPERNODE_CLIENT_INL__
#define __BDT_SUPERNODE_CLIENT_INL__
#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include <BuckyBase/BuckyBase.h>
#include "./Client.h"
#include "../Stack.h"

#define BDT_MICRO_SECONDS_2_MS(microS)	((microS) / 1000)
#define BDT_MS_NOW						BDT_MICRO_SECONDS_2_MS(BfxTimeGetNow(FALSE))

typedef struct BdtStack BdtStack;
typedef struct CallSessionMap CallSessionMap;
typedef struct PingManager PingManager;
typedef struct BdtSnClient_CallSession BdtSnClient_CallSession;
typedef BdtSnClient_CallSession CallSession;

typedef enum BdtSuperNodeClientStatus
{
	BDT_SUPER_NODE_CLIENT_STATUS_STOPPED = 0,
	BDT_SUPER_NODE_CLIENT_STATUS_RUNNING = 1,
} BdtSuperNodeClientStatus;

struct BdtSuperNodeClient {
	BdtStack* stack;
	BdtSuperNodeClientStatus status;
	PingManager* pingMgr;

	Bdt_SequenceCreator seqCreator;

	BfxRwLock seqLock;
	// seqLock protected members begin
	CallSessionMap* seqMap;
	// seqLock protected members end

	Bdt_TimerHelper timer;
};

static uint32_t SuperNodeClientNextSeq(BdtSuperNodeClient* snClient);
static uint32_t SuperNodeClientRemoveCallSession(BdtSuperNodeClient* snClient, CallSession* session);

#endif // __BDT_SUPERNODE_CLIENT_INL__