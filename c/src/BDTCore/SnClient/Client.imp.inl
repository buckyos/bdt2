#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include <assert.h>
#include <SGLib/SGLib.h>
#include "./Client.inl"
#include "./CallSession.inl"
#include "./Ping.inl"

typedef struct CallSessionMap
{
	struct CallSessionMap* left;
	struct CallSessionMap* right;
	uint8_t color;
	uint32_t seq;
	CallSession* session;
} CallSessionMap, call_session_map;

static int call_session_map_compare(const call_session_map* left, const call_session_map* right)
{
	if (left->seq == right->seq)
	{
		return 0;
	}
	return left->seq > right->seq ? 1 : -1;
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(call_session_map, left, right, color, call_session_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(call_session_map, left, right, color, call_session_map_compare)

static void BFX_STDCALL SuperNodeClientTimerProcess(BDT_SYSTEM_TIMER, BdtSuperNodeClient* client);
static void SuperNodeClientSendCalledResp(BdtSuperNodeClient* client,
	Bdt_SnCalledPackage* calledReq,
	const BdtEndpoint* snEndpoint,
	const Bdt_UdpInterface* udpInterface,
	const uint8_t* aesKey);

static uint32_t SuperNodeClientInit(BdtStack* stack,
	bool encrypto,
	BdtSuperNodeClient* client)
{
	BLOG_INFO("stack:0x%p, client0x%p", stack, client);
	if (client == NULL)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	memset(client, 0, sizeof(BdtSuperNodeClient));

	BdtSystemFramework* framework = BdtStackGetFramework(stack);

	client->stack = stack;
	client->status = BDT_SUPER_NODE_CLIENT_STATUS_STOPPED;

	BfxRwLockInit(&client->seqLock);
	Bdt_SequenceInit(&client->seqCreator);

	client->pingMgr = PingManagerCreate(stack, encrypto, client);

	Bdt_TimerHelperInit(&client->timer, framework);

	return BFX_RESULT_SUCCESS;
}

static void SuperNodeClientUninit(BdtSuperNodeClient* client)
{
	BLOG_INFO("client:0x%p", client);
	if (client == NULL || client->status != BDT_SUPER_NODE_CLIENT_STATUS_STOPPED)
	{
		BLOG_WARN("client:0x%p, should stop first.", client);
		return;
	}

    // <TODO>因为timer的存在，销毁可能崩溃
	// Bdt_TimerHelperUninit(&client->timer);
	// PingManagerDestroy(client->pingMgr);
	// Bdt_SequenceUninit(&client->seqCreator);
	
	// BfxRwLockDestroy(&client->seqLock);

	BLOG_CHECK_EX(client->seqMap == NULL, "some call session not release.");
}

uint32_t BdtSnClient_Create(
	BdtStack* stack,
	bool encrypto,
	BdtSuperNodeClient** outClient)
{
	BdtSuperNodeClient* client = BFX_MALLOC_OBJ(BdtSuperNodeClient);
	uint32_t ret = SuperNodeClientInit(stack, encrypto, client);
	if (ret)
	{
		BfxFree(client);
		*outClient = NULL;
		return ret;
	}
	*outClient = client;
	return BFX_RESULT_SUCCESS;
}

uint32_t BdtSnClient_Start(BdtSuperNodeClient* client)
{
	//TODO:在多线程环境下，状态判断未加保护是有潜在隐患的，应该用atomic 比较
	if (client->status == BDT_SUPER_NODE_CLIENT_STATUS_RUNNING)
	{
		BLOG_WARN("sn client:0x%p, already started.", client);
		return BFX_RESULT_INVALID_STATE;
	}

	client->status = BDT_SUPER_NODE_CLIENT_STATUS_RUNNING;
	BfxUserData ud = { .userData = client, .lpfnAddRefUserData = NULL, .lpfnReleaseUserData = NULL };
	//TODO:会立刻运行一次SuperNodeClientTimerProcess，Timer这种写法？
	BLOG_VERIFY(Bdt_TimerHelperStart(
		&client->timer,
		(LPFN_TIMER_PROCESS)SuperNodeClientTimerProcess,
		&ud,
		100)
	);
	return BFX_RESULT_SUCCESS;
}

void BdtSnClient_Destroy(
	BdtSuperNodeClient* client
)
{
	SuperNodeClientUninit(client);
	BfxFree(client);
}

uint32_t BdtSnClient_Stop(BdtSuperNodeClient* client)
{
	if (client->status == BDT_SUPER_NODE_CLIENT_STATUS_STOPPED)
	{
		BLOG_WARN("client:0x%p, already stopped.", client);
		return BFX_RESULT_INVALID_STATE;
	}

	client->status = BDT_SUPER_NODE_CLIENT_STATUS_STOPPED;
	Bdt_TimerHelperStop(&client->timer);
	return BFX_RESULT_SUCCESS;
}

static uint32_t SuperNodeClientNextSeq(BdtSuperNodeClient* client)
{
	return Bdt_SequenceNext(&client->seqCreator);
}

uint32_t BdtSnClient_PushUdpPackage(BdtSuperNodeClient* client,
	const Bdt_Package* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	const uint8_t* aesKey,
	bool* outHandled)
{
	{
#ifndef BLOG_DISABLE
		char fromEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BLOG_INFO("client:0x%p,cmdtype:%d,from:%s", client,
			package->cmdtype,
			(BdtEndpointToString(fromEndpoint, fromEndpointStr), fromEndpointStr)
		);
#endif
	}
	
	switch (package->cmdtype)
	{
	case BDT_SN_PING_RESP_PACKAGE:
		{
			PingManagerPingResponsed(client->pingMgr, (Bdt_SnPingRespPackage*)package, fromEndpoint, fromInterface, outHandled);
			*outHandled = TRUE;
		}
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		{
			Bdt_SnCallRespPackage* callResp = (Bdt_SnCallRespPackage*)package;
			CallSession* callSession = NULL;
			BfxRwLockRLock(&client->seqLock);
			{
				CallSessionMap toFind;
				toFind.seq = callResp->seq;
				const CallSessionMap* found = sglib_call_session_map_find_member(client->seqMap, &toFind);
				if (found)
				{
					BdtSnClient_CallSessionAddRef(found->session);
					callSession = found->session;
				}
			}
			BfxRwLockRUnlock(&client->seqLock);
			if (callSession != NULL)
			{
				CallSessionPushUdpPackage(callSession, callResp, fromEndpoint, fromInterface, outHandled);
				*outHandled = TRUE;
				BdtSnClient_CallSessionRelease(callSession);
			}
		}
		break;
	case BDT_SN_CALLED_PACKAGE:
		{
			Bdt_SnCalledPackage* calledReq = (Bdt_SnCalledPackage*)package;
			SuperNodeClientSendCalledResp(client,
				calledReq,
				fromEndpoint,
				fromInterface,
				aesKey);
			Bdt_PushSuperNodeClientEvent(BdtStackGetEventCenter(client->stack),
				(void*)calledReq->peerInfo,
				BDT_SUPER_NODE_CLIENT_CALLED,
				(void*)calledReq);
			*outHandled = TRUE;
		}
		break;
	default:
		assert(FALSE);
	}
	return BFX_RESULT_SUCCESS;
}

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
	CallSession** outSession,
    Bdt_PackageWithRef** callPkg
)
{
	uint32_t nextSeq = Bdt_SequenceNext(&client->seqCreator);
	
	BLOG_HEX(remotePeerid->pid, 15, "client:0x%p, seq:%d", client, nextSeq);

	CallSession* callSession = CallSessionCreate(
        client,
        localPeerInfo,
		remotePeerid,
		reverseEndpointArray,
		reverseEndpointCount,
		snPeerInfo,
		nextSeq,
		alwaysCall,
		encrypto,
		callback,
		userData,
        callPkg);

	*outSession = callSession;

	CallSessionMap* node = BFX_MALLOC_OBJ(CallSessionMap);
	node->seq = nextSeq;
	node->session = callSession;
	BfxRwLockWLock(&client->seqLock);
	{
		sglib_call_session_map_add(&client->seqMap, node);
	}
	BfxRwLockWUnlock(&client->seqLock);

	return BFX_RESULT_SUCCESS;
}

static uint32_t SuperNodeClientRemoveCallSession(BdtSuperNodeClient* snClient, CallSession* session)
{
	BLOG_INFO("session:0x%p, seq:%d", session, session->seq);
	BLOG_CHECK(session->owner == snClient);

	CallSessionMap* del = NULL;
	BfxRwLockWLock(&snClient->seqLock);
	{
		CallSessionMap toFind;
		toFind.seq = session->seq;
		sglib_call_session_map_delete_if_member(&snClient->seqMap, &toFind, &del);
	}
	BfxRwLockWUnlock(&snClient->seqLock);
	if (del)
	{
		BfxFree(del);
	}
	return BFX_RESULT_SUCCESS;
}

static void BFX_STDCALL SuperNodeClientTimerProcess(BDT_SYSTEM_TIMER t, BdtSuperNodeClient* client)
{
	BLOG_CHECK(client != NULL);

	if (client->status == BDT_SUPER_NODE_CLIENT_STATUS_STOPPED)
	{
		return;
	}

	PingManagerOnTimer(client->pingMgr);

	BfxUserData ud = { .userData = client,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BLOG_VERIFY(Bdt_TimerHelperRestart(
		&client->timer,
		(LPFN_TIMER_PROCESS)SuperNodeClientTimerProcess,
		&ud,
		100)
		);
}

static void SuperNodeClientSendCalledResp(BdtSuperNodeClient* client,
	Bdt_SnCalledPackage* calledReq,
	const BdtEndpoint* snEndpoint,
	const Bdt_UdpInterface* udpInterface,
	const uint8_t* aesKey)
{
	BdtStack* stack = client->stack;
	BdtSystemFramework* framework = BdtStackGetFramework(stack);

	Bdt_SnCalledRespPackage* calledRespPkg = Bdt_SnCalledRespPackageCreate();
	Bdt_SnCalledRespPackageInit(calledRespPkg);
	calledRespPkg->cmdflags = BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ;

	calledRespPkg->seq = calledReq->seq;

	// Bdt_StackTlsData* tls = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	Bdt_StackTlsData tlsObj;
	Bdt_StackTlsData* tls = &tlsObj;
	size_t encodedSize = sizeof(tls->udpEncodeBuffer);
	if (aesKey == NULL)
	{
		Bdt_BoxUdpPackage(
			(const Bdt_Package**)(&calledRespPkg), 
			1, 
			NULL, 
			tls->udpEncodeBuffer, 
			&encodedSize);
	}
	else
	{
		Bdt_BoxEncryptedUdpPackage(
			(const Bdt_Package**)(&calledRespPkg), 
			1, 
			NULL, 
			aesKey, 
			tls->udpEncodeBuffer, 
			&encodedSize);
	}

	if (calledRespPkg != NULL)
	{
		Bdt_SnCalledRespPackageFree(calledRespPkg);
	}

	{
#ifndef BLOG_DISABLE
		char snEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BLOG_INFO("client:0x%p, snEp:%s, seq:%d.", client, (BdtEndpointToString(snEndpoint, snEpStr), snEpStr), calledReq->seq);
#endif
    }
	
	BdtUdpSocketSendTo(framework, Bdt_UdpInterfaceGetSocket(udpInterface), snEndpoint, tls->udpEncodeBuffer, encodedSize);
}
