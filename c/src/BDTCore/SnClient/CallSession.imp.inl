#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include <assert.h>
#include "../BdtCore.h"
#include "../Stack.h"
#include "../BdtSystemFramework.h"
#include "./SnClientModule.h"
#include "./CallSession.inl"
#include "./Client.inl"
#include "../Protocol/Package.h"
#include "../Protocol/PackageEncoder.h"
#include "../Interface/NetManager.h"
#include "../PeerFinder.h"

typedef struct TcpInterfaceVector
{
	Bdt_TcpInterface** tcpInterfaces;
	size_t size;
	size_t _allocsize;
} TcpInterfaceVector, tcp_interface;

BFX_VECTOR_DEFINE_FUNCTIONS(tcp_interface, Bdt_TcpInterface*, tcpInterfaces, size, _allocsize)

typedef struct BdtSnClient_CallClient
{
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	CallSession* callSession;

	const BdtPeerInfo* snPeerInfo;
	int64_t lastCallTime;	
	
	uint32_t resendInterval;
	uint8_t aesKey[BDT_AES_KEY_LENGTH];

	BdtEndpoint usingEndpoint;
	Bdt_UdpInterface* volatile usingUdpInterface;

	BfxThreadLock lock;
	// lock protected members begin
	TcpInterfaceVector tcpInterfaceVector;
	int64_t lastRespTime;
	// lock protected members end

} BdtSnClient_CallClient, CallClient;

static void CallClientTryResendOnTimer(CallClient* callClient, int64_t now);
static void CallClientTryResendCallAllUdp(CallClient* callClient);
static void CallClientResponceUdp(CallClient* callClient,
	Bdt_SnCallRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface);
static void BFX_STDCALL CallSessionTimerProcess(BDT_SYSTEM_TIMER t, CallSession* session);
static void CallSessionDestroy(CallSession* callSession);
static void BFX_STDCALL CallSessionAddRefAsUserData(void* userData);
static void BFX_STDCALL CallSessionReleaseAsUserData(void* userData);
static void CallClientSendCallTcp(CallClient* callClient,
	Bdt_TcpInterface* tcpInterface,
	Bdt_PackageWithRef** pkgs
);
static void CallClientClearTcpInterfaces(CallClient* callClient);
static void CallClientTryConnectAllTcp(CallClient* callClient);
static int CallSessionMakeCallPackage(CallSession* callSession, Bdt_PackageWithRef* pkgs[2]);
static void CallClientUpdateResendInterval(CallClient* callClient);
static void CallClientTryConnectTcp(CallClient* callClient,
	const BdtEndpoint** localEndpointArray,
	size_t localEndpointCount,
	const BdtEndpoint** remoteEndpointArray,
	size_t remoteEndpointCount);
static int CallClientCallPriorityEndpoints(CallClient* callClient);
static void CallClientOnTimeout(CallClient* callClient);

static CallSession* CallSessionCreate(
    BdtSuperNodeClient* owner,
    const BdtPeerInfo* localPeerInfo,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* reverseEndpointArray,
	size_t reverseEndpointCount,
	const BdtPeerInfo* snPeerInfo,
	uint32_t seq,
	bool alwaysCall,
	bool encrypto,
	BdtSnClient_CallSessionCallback callback,
	const BfxUserData* userData,
    Bdt_PackageWithRef** callPkg
)
{
	BLOG_CHECK(snPeerInfo != NULL);
	BLOG_CHECK(remotePeerid != NULL);
	BLOG_CHECK(callback != NULL);

	const BdtPeerInfo* snPeerInfos[1] = { snPeerInfo };
	uint32_t snCount = 1;
	const BdtSuperNodeClientConfig* config = &BdtStackGetConfig(owner->stack)->snClient;
	const int64_t now = BfxTimeGetNow(false);

	size_t allocSize = sizeof(CallSession) + snCount * sizeof(CallClient);
	CallSession* session = (CallSession*)BfxMalloc(allocSize);
	memset(session, 0, allocSize);
	session->owner = owner;
	session->callClients = (CallClient*)(session + 1);
	memcpy(&session->remotePeerid, remotePeerid, sizeof(BdtPeerid));
	BdtEndpointArrayInit(&session->reverseEndpointArray, reverseEndpointCount);
	if (reverseEndpointCount > 0 && reverseEndpointArray != NULL)
	{
		memcpy(&session->reverseEndpointArray.list, reverseEndpointArray, sizeof(BdtEndpoint) * reverseEndpointCount);
		session->reverseEndpointArray.size = reverseEndpointCount;
	}

	BLOG_HEX(remotePeerid->pid, 15, "session:0x%p,snCount:%d,seq:%u",
		session,
		(int)snCount,
		seq
	);

	session->seq = seq;
	session->snCount = snCount;
	session->encrypto = encrypto;
	session->alwaysCall = alwaysCall;
	session->endTime = now + (int64_t)config->callTimeout * 1000;

	Bdt_TimerHelperInit(&session->timer, BdtStackGetFramework(owner->stack));

	session->callback = callback;
	if (userData != NULL)
	{
		session->userData = *userData;
		if (userData->lpfnAddRefUserData != NULL &&
			userData->userData != NULL)
		{
			userData->lpfnAddRefUserData(userData->userData);
		}
	}

	for (uint32_t i = 0; i < snCount; i++)
	{
		CallClient* client = &session->callClients[i];

		BLOG_HEX(BdtPeerInfoGetPeerid(snPeerInfos[i])->pid, 15, "client:0x%p.", client);

		bfx_vector_tcp_interface_init(&client->tcpInterfaceVector);
		client->callSession = session;
		BdtPeerInfoBuilder* infoBuilder = BdtPeerInfoBuilderBegin(0);
		BdtPeerInfoClone(infoBuilder, snPeerInfos[i]);
		client->snPeerInfo = BdtPeerInfoBuilderFinish(infoBuilder, false);
		client->resendInterval = config->callInterval;
		BfxThreadLockInit(&client->lock);

		if (encrypto)
		{
			bool isNewKey = true;
			bool isConfirmed = true;
			BdtHistory_KeyManagerGetKeyByRemote(
				BdtStackGetKeyManager(owner->stack),
				BdtPeerInfoGetPeerid(client->snPeerInfo),
				client->aesKey,
				&isNewKey,
				&isConfirmed,
				TRUE);
		}
	}

    session->localPeerInfo = localPeerInfo;
    BdtAddRefPeerInfo(session->localPeerInfo);

    CallSessionMakeCallPackage(session, session->pkgs);

    if (session->pkgs[1] != NULL)
    {
        *callPkg = session->pkgs[1];
    }
    else
    {
        BLOG_CHECK(session->pkgs[0] != NULL);
        *callPkg = session->pkgs[0];
    }
    Bdt_PackageAddRef(*callPkg);

	session->refCount = 1;

	return session;
}

uint32_t BdtSnClient_CallSessionStart(
    CallSession* session,
    BFX_BUFFER_HANDLE payload
)
{
	BLOG_DEBUG("session:0x%p, seq:%d", session, session->seq);

    if (payload != NULL)
    {
        session->payload = payload;
        BfxBufferAddRef(payload);
    }

    Bdt_SnCalledPackage* callPkg = NULL;
    if (session->pkgs[1] != NULL)
    {
        callPkg = (Bdt_SnCalledPackage*)Bdt_PackageWithRefGet(session->pkgs[1]);
        BLOG_CHECK(callPkg->cmdtype == BDT_SN_CALL_PACKAGE);
    }
    else
    {
        callPkg = (Bdt_SnCalledPackage*)Bdt_PackageWithRefGet(session->pkgs[0]);
        BLOG_CHECK(callPkg->cmdtype == BDT_SN_CALL_PACKAGE);
    }

    if (payload != NULL)
    {
        callPkg->payload = payload;
        BfxBufferAddRef(payload);
    }

	for (size_t i = 0; i < session->snCount; i++)
	{
		CallClient* client = &session->callClients[i];
		if (CallClientCallPriorityEndpoints(client) <= 0)
		{
			// 没有优先启用地址就全地址广播
			CallClientTryResendCallAllUdp(client);
			CallClientTryConnectAllTcp(client);
		}
	}

	BfxUserData ud = {
		.userData = session,
		.lpfnAddRefUserData = CallSessionAddRefAsUserData,
		.lpfnReleaseUserData = CallSessionReleaseAsUserData
	};

	BLOG_VERIFY(Bdt_TimerHelperStart(&session->timer,
		(LPFN_TIMER_PROCESS)CallSessionTimerProcess,
		&ud,
		100));

	return BFX_RESULT_SUCCESS;
}

int32_t BdtSnClient_CallSessionAddRef(const CallSession* _session)
{
	CallSession* session = (CallSession*)_session;
	return BfxAtomicInc32(&session->refCount);
}

static void BFX_STDCALL CallSessionAddRefAsUserData(void* userData)
{
	BdtSnClient_CallSessionAddRef((CallSession*)userData);
}

int32_t BdtSnClient_CallSessionRelease(const CallSession* _session)
{
	CallSession* session = (CallSession*)_session;
	int32_t refCount = BfxAtomicDec32(&session->refCount);
	if (refCount == 0)
	{
		SuperNodeClientRemoveCallSession(session->owner, session);
		CallSessionDestroy(session);
	}
	return refCount;
}

static void BFX_STDCALL CallSessionReleaseAsUserData(void* userData)
{
	BdtSnClient_CallSessionRelease((CallSession*)userData);
}

static void BFX_STDCALL CallClientAddRef(CallClient* callClient)
{
	BdtSnClient_CallSessionAddRef(callClient->callSession);
}

static void BFX_STDCALL CallClientRelease(CallClient* callClient)
{
	BdtSnClient_CallSessionRelease(callClient->callSession);
}

static uint32_t CallSessionPushUdpPackage(CallSession* session,
	Bdt_SnCallRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	bool* outHandled)
{
	{
#ifndef BLOG_DISABLE
		char fromEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BLOG_HEX(package->snPeerid.pid, 15,
			"session:0x%p, seq:%d, cmdtype:%d, ep:%s, udpInterface:0x%p",
			session,
			session->seq,
			package->cmdtype,
			(BdtEndpointToString(fromEndpoint, fromEndpointStr), fromEndpointStr),
			fromInterface);
#endif
	}
	

	CallClient* targetCallClient = NULL;
	for (uint32_t i = 0; i < session->snCount; i++)
	{
		CallClient* client = &session->callClients[i];
		if (memcmp(BdtPeerInfoGetPeerid(client->snPeerInfo), &package->snPeerid, sizeof(BdtPeerid)) == 0)
		{
			targetCallClient = client;
			break;
		}
	}

	if (targetCallClient != NULL)
	{
		CallClientResponceUdp(targetCallClient, package, fromEndpoint, fromInterface);
	}

	return BFX_RESULT_SUCCESS;
}

static void CallSessionCallbackNotify(CallSession* callSession,
	Bdt_SnCallRespPackage* package)
{
	BdtSnClient_CallSessionCallback callback = (BdtSnClient_CallSessionCallback)BfxAtomicExchangePointer((volatile BfxAtomicPointerType*)&callSession->callback, NULL);
	if (callback)
	{
		if (package != NULL)
		{
			uint32_t errorCode = BFX_RESULT_UNKNOWN;
			if (package->result == 0)
			{
				errorCode = BFX_RESULT_SUCCESS;
			}
			else if (package->result == 12)
			{
				errorCode = BFX_RESULT_NOT_FOUND;
			}
			else if (package->result == 2)
			{
				errorCode = BFX_RESULT_TOO_LARGE;
			}
			//TODO：这里传的SNCallResp里的PeerInfo，其EndPointList部分可能没有签名（是SN看到的）
			callback(errorCode,
				callSession,
				package->toPeerInfo,
				callSession->userData.userData
			);
		}
		else
		{
			callback(BFX_RESULT_TIMEOUT,
				callSession,
				NULL,
				callSession->userData.userData);
		}
	}
}

static uint32_t CallClientOnTcpEstablish(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	CallClient* callClient = (CallClient*)owner;
    CallSession* callSession = callClient->callSession;

	{
#ifndef BLOG_DISABLE
		char fromEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BLOG_INFO("session:0x%p, seq:%d, callClient:0x%p fromEp:%s, tcpInterface:0x%p",
			callClient->callSession,
			callClient->callSession->seq,
			callClient,
			(BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), fromEndpointStr), fromEndpointStr),
			tcpInterface);
#endif
	}
	

	BLOG_CHECK(tcpInterface != NULL);

	Bdt_TcpInterfaceSetState(tcpInterface, BDT_TCP_INTERFACE_STATE_ESTABLISH, BDT_TCP_INTERFACE_STATE_PACKAGE);
	Bdt_TcpInterfaceResume(tcpInterface);

	// TCP不需要重试，连接后发送一次就好了
	CallClientSendCallTcp(callClient, tcpInterface, callSession->pkgs);

	return BFX_RESULT_SUCCESS;
}

static void BFX_STDCALL TcpInterfaceReleaseImmediate(void* userData)
{
	Bdt_TcpInterfaceRelease((Bdt_TcpInterface*)userData);
}

static void CallClientOnTcpClosed(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	CallClient* callClient = (CallClient*)owner;
	BLOG_INFO("session:0x%p, seq:%d, callClient:0x%p tcpInterface:0x%p",
		callClient->callSession,
		callClient->callSession->seq,
		callClient,
		tcpInterface);

	bool newRemove = false;

	{
		BfxThreadLockLock(&callClient->lock);
		for (size_t j = 0; j < callClient->tcpInterfaceVector.size; j++)
		{
			if (callClient->tcpInterfaceVector.tcpInterfaces[j] == tcpInterface)
			{
				callClient->tcpInterfaceVector.tcpInterfaces[j] = NULL;
				newRemove = true;
				break;
			}
		}
		BfxThreadLockUnlock(&callClient->lock);
	}

	BdtSystemFramework* framework = BdtStackGetFramework(callClient->callSession->owner->stack);

	CallClientRelease(callClient);

	// vector保留一个计数，连接本身保留一个计数
	if (newRemove)
	{
		Bdt_TcpInterfaceRelease(tcpInterface);
	}

	BfxUserData ud = {
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL,
		.userData = (void*)tcpInterface
	};
	BdtImmediate(framework, TcpInterfaceReleaseImmediate, &ud);
}

static uint32_t CallClientOnTcpPackage(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package** packages)
{
	CallClient* callClient = (CallClient*)owner;
	CallSession* callSession = callClient->callSession;
#ifndef BLOG_DISABLE
    char fromEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
#endif

	size_t ix = 0;
	
	while (packages[ix] != NULL)
	{
		if (packages[ix]->cmdtype != BDT_SN_CALL_RESP_PACKAGE)
		{
			continue;
		}

		Bdt_SnCallRespPackage* package = (Bdt_SnCallRespPackage*)packages[ix];

		{
#ifndef BLOG_DISABLE
			char toPeerStr[1024];
			size_t strLength = 1024;
			
			BLOG_HEX(package->snPeerid.pid, 15,
				"session:0x%p, seq:%d client:0x%p, cmdtype:%d, ep:%s, tcpInterface:0x%p, toPeerInfo:%p(%s)",
				callSession,
				callSession->seq,
				callClient,
				package->cmdtype,
				(BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), fromEndpointStr), fromEndpointStr),
				tcpInterface,
				package->toPeerInfo,
				BdtPeerInfoFormat(package->toPeerInfo, toPeerStr, &strLength));
#endif
		}

		if (package->toPeerInfo != NULL)
		{
			BdtPeerFinder* peerFinder = BdtStackGetPeerFinder(callSession->owner->stack);
			BdtPeerFinderAddCached(peerFinder, package->toPeerInfo, 0);
		}

		// 持久化
		BdtHistory_PeerUpdater* historyUpdater = BdtStackGetHistoryPeerUpdater(callSession->owner->stack);
		if (tcpInterface != NULL)
		{
			const BdtEndpoint* localEndpoint = Bdt_TcpInterfaceGetLocal(tcpInterface);
            BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
				historyUpdater,
				BdtPeerInfoGetPeerid(callClient->snPeerInfo),
				callClient->snPeerInfo,
				Bdt_TcpInterfaceGetRemote(tcpInterface),
				localEndpoint,
				BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT,
				0);
			if (package->toPeerInfo != NULL)
			{
				BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(historyUpdater,
					package->toPeerInfo,
					callClient->snPeerInfo);
			}
		}

		// 已经响应就清理tcp连接，防止连通后再次发出不必要的请求
		uint64_t msNow = BDT_MS_NOW;
		uint64_t lastRespTime = 0;
		BfxThreadLockLock(&callClient->lock);
		lastRespTime = callClient->lastRespTime;
		callClient->lastRespTime = msNow;
		BfxThreadLockUnlock(&callClient->lock);

		if (lastRespTime == 0)
		{
			CallClientClearTcpInterfaces(callClient);
			Bdt_TimerHelperStop(&callSession->timer);
			CallSessionCallbackNotify(callSession, package);
		}

		++ix;
	}
	
	return BFX_RESULT_SUCCESS;
}

static int32_t CallClientAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	return BdtSnClient_CallSessionAddRef(((CallClient*)owner)->callSession);
}

static int32_t CallClientAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	return BdtSnClient_CallSessionRelease(((CallClient*)owner)->callSession);
}

static Bdt_TcpInterfaceOwner* CallClientInitAsTcpInterfaceOwner(CallClient* callClient)
{
	callClient->addRef = CallClientAsTcpInterfaceOwnerAddRef;
	callClient->release = CallClientAsTcpInterfaceOwnerRelease;
	callClient->onEstablish = CallClientOnTcpEstablish;
	callClient->onPackage = CallClientOnTcpPackage;
	callClient->onClosed = CallClientOnTcpClosed;
	return (Bdt_TcpInterfaceOwner*)callClient;
}

static void CallSessionOnTimer(CallSession* session)
{
	int64_t now = BDT_MS_NOW;

	for (uint32_t i = 0; i < session->snCount; i++)
	{
		CallClient* callClient = &session->callClients[i];
		CallClientTryResendOnTimer(callClient, now);
	}
}

static void CallClientTryResendOnTimer(CallClient* callClient, int64_t now)
{
	if ((uint32_t)(now - callClient->lastCallTime) < callClient->resendInterval)
	{
		return;
	}

	// no need to lock
	if (callClient->lastRespTime == 0)
	{
		CallClientTryResendCallAllUdp(callClient);
		CallClientUpdateResendInterval(callClient);
	}
}

static void CallClientResponceUdp(CallClient* callClient,
	Bdt_SnCallRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface)
{
	CallSession* callSession = callClient->callSession;

    {
#ifndef BLOG_DISABLE
        char toPeerStr[1024];
        size_t strLength = 1024;
	    BLOG_DEBUG("session:0x%p, seq:%u, client:0x%p, toPeerInfo:0x%p(%s),result:%u",
		    callSession,
		    callSession->seq,
		    callClient,
            package->toPeerInfo,
            BdtPeerInfoFormat(package->toPeerInfo, toPeerStr, &strLength),
            (uint32_t)package->result
            );
#endif
    }


	if (BfxAtomicCompareAndSwapPointer(&callClient->usingUdpInterface, NULL, (Bdt_UdpInterface*)fromInterface) == NULL)
	{
		memcpy(&callClient->usingEndpoint, fromEndpoint, sizeof(BdtEndpoint));
		Bdt_UdpInterfaceAddRef(fromInterface);
	}

	if (package->toPeerInfo != NULL)
	{
		BdtPeerFinder* peerFinder = BdtStackGetPeerFinder(callSession->owner->stack);
		BdtPeerFinderAddCached(peerFinder, package->toPeerInfo, 0);
	}

	// 持久化
	BdtHistory_PeerUpdater* historyUpdater = BdtStackGetHistoryPeerUpdater(callSession->owner->stack);
    BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
		historyUpdater,
		BdtPeerInfoGetPeerid(callClient->snPeerInfo),
		callClient->snPeerInfo,
		fromEndpoint,
		Bdt_UdpInterfaceGetLocal(fromInterface),
		BDT_HISTORY_PEER_CONNECT_TYPE_UDP,
		0);
	if (package->toPeerInfo != NULL)
	{
		BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(historyUpdater,
			package->toPeerInfo,
			callClient->snPeerInfo);
	}

	uint64_t msNow = BDT_MS_NOW;
	uint64_t lastRespTime = 0;
	BfxThreadLockLock(&callClient->lock);
	lastRespTime = callClient->lastRespTime;
	callClient->lastRespTime = msNow;
	BfxThreadLockUnlock(&callClient->lock);

	if (lastRespTime == 0)
	{
		CallClientClearTcpInterfaces(callClient);
		Bdt_TimerHelperStop(&callSession->timer);
		CallSessionCallbackNotify(callSession, package);
	}
}

static int CallSessionMakeCallPackage(CallSession* callSession, Bdt_PackageWithRef* pkgs[2])
{
	BdtStack* stack = callSession->owner->stack;

	// fromPeerid被peerinfo字段包含，忽略它
	Bdt_ExchangeKeyPackage* exchgKey = NULL;
	int pkgCount = 0;
	if (callSession->encrypto)
	{
        pkgs[0] = Bdt_PackageCreateWithRef(BDT_EXCHANGEKEY_PACKAGE);
		exchgKey = (Bdt_ExchangeKeyPackage*)Bdt_PackageWithRefGet(pkgs[0]);
		Bdt_ExchangeKeyPackageInit(exchgKey);

		exchgKey->cmdflags = BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQ |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQANDKEYSIGN |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_SENDTIME;

		exchgKey->peerInfo = callSession->localPeerInfo;
        BdtAddRefPeerInfo(exchgKey->peerInfo);
		memcpy(&exchgKey->fromPeerid, BdtPeerInfoGetPeerid(exchgKey->peerInfo), sizeof(BdtPeerid));
		exchgKey->seq = callSession->seq;
		exchgKey->sendTime = 0;
		pkgCount++;
	}

    pkgs[pkgCount] = Bdt_PackageCreateWithRef(BDT_SN_CALL_PACKAGE);
	Bdt_SnCallPackage* callPkg = (Bdt_SnCallPackage*)Bdt_PackageWithRefGet(pkgs[pkgCount]);
	Bdt_SnCallPackageInit(callPkg);
	callPkg->cmdflags = BDT_SN_CALL_PACKAGE_FLAG_SEQ |
		BDT_SN_CALL_PACKAGE_FLAG_TOPEERID |
		BDT_SN_CALL_PACKAGE_FLAG_PEERINFO;

	if (callSession->alwaysCall)
	{
		callPkg->cmdflags |= BDT_SN_CALL_PACKAGE_FLAG_ALWAYSCALL;
	}

	callPkg->seq = callSession->seq;
	callPkg->peerInfo = callSession->localPeerInfo;
    BdtAddRefPeerInfo(callPkg->peerInfo);
	memcpy(&callPkg->fromPeerid, BdtPeerInfoGetPeerid(callPkg->peerInfo), sizeof(BdtPeerid));
	memcpy(&callPkg->toPeerid, &callSession->remotePeerid, sizeof(BdtPeerid));
	if (callSession->payload != NULL)
	{
		callPkg->payload = callSession->payload;
		BfxBufferAddRef(callPkg->payload);
		callPkg->cmdflags |= BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD;
	}

	if (callSession->reverseEndpointArray.size > 0)
	{
		BdtEndpointArrayClone(&callSession->reverseEndpointArray, &callPkg->reverseEndpointArray);
		callPkg->cmdflags |= BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY;
	}
	pkgCount++;
	return pkgCount;
}

static bool CallSessionIsSameEndpoint(const BdtEndpoint* ep1, const BdtEndpoint* ep2)
{
	bool isIpv4 = ((ep1->flag & BDT_ENDPOINT_IP_VERSION_4) != 0);
	return ep1->port == ep2->port &&
		((ep2->flag & BDT_ENDPOINT_IP_VERSION_4) != 0) == isIpv4 &&
		memcmp(ep1->address, ep2->address, isIpv4 ? sizeof(ep1->address) : sizeof(ep2->addressV6)) == 0;
}

static void CallClientTryConnectAllTcp(CallClient* callClient)
{
    CallSession* callSession = callClient->callSession;
	BdtStack* stack = callSession->owner->stack;

	const BdtTcpTunnelConfig* config = &BdtStackGetConfig(stack)->tunnel.tcpTunnel;

	size_t remoteEpCount = 0;
	const BdtEndpoint* remoteEpList = BdtPeerInfoListEndpoint(callClient->snPeerInfo, &remoteEpCount);

	size_t remoteTcpEpCount = 0;
	const BdtEndpoint* remoteTcpEpList[32];

	BdtEndpoint excludeRemoteEp = { .port = 0 };

	BLOG_CHECK(callClient->tcpInterfaceVector.size <= 1);
	{
		BfxThreadLockLock(&callClient->lock);
		if (callClient->tcpInterfaceVector.size == 1)
		{
			const Bdt_TcpInterface* tcpInterface = callClient->tcpInterfaceVector.tcpInterfaces[0];
			excludeRemoteEp = *Bdt_TcpInterfaceGetRemote(tcpInterface);
		}
		BfxThreadLockUnlock(&callClient->lock);
	}

	for (size_t i = 0; i < remoteEpCount && remoteTcpEpCount < BFX_ARRAYSIZE(remoteTcpEpList); i++)
	{
		const BdtEndpoint* remoteEp = remoteEpList + i;
		if ((remoteEp->flag & BDT_ENDPOINT_PROTOCOL_TCP) != 0 &&
			!CallSessionIsSameEndpoint(remoteEp, &excludeRemoteEp))
		{
			remoteTcpEpList[remoteTcpEpCount] = remoteEp;
			remoteTcpEpCount++;
		}
	}

	const BdtEndpoint* localTcpEpList[32];
	size_t localTcpEpCount = 0;

	const BdtPeerInfo* localPeer = callSession->localPeerInfo;
    BdtAddRefPeerInfo(localPeer);

#if 1
	size_t localEpCount = 0;
	const BdtEndpoint* localEpList = BdtPeerInfoListEndpoint(localPeer, &localEpCount);

	for (size_t i = 0; i < localEpCount && localTcpEpCount < BFX_ARRAYSIZE(localTcpEpList); i++)
	{
		const BdtEndpoint* localEp = localEpList + i;
		if (localEp->flag & BDT_ENDPOINT_PROTOCOL_TCP)
		{
			localTcpEpList[localTcpEpCount] = localEp;
			localTcpEpCount++;
		}
	}
#endif

	const BdtEndpoint endpointAny = {
		.flag = BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 0,
		.address = {0, 0, 0, 0}
	};
	if (localTcpEpCount == 0)
	{
		localTcpEpList[localTcpEpCount++] = &endpointAny;
	}

	CallClientTryConnectTcp(callClient,
		localTcpEpList,
		localTcpEpCount,
		remoteTcpEpList,
		remoteTcpEpCount);

	BdtReleasePeerInfo(localPeer);
}

static void CallClientTryConnectTcp(CallClient* callClient,
	const BdtEndpoint** localEndpointArray,
	size_t localEndpointCount,
	const BdtEndpoint** remoteEndpointArray,
	size_t remoteEndpointCount)
{
	BdtStack* stack = callClient->callSession->owner->stack;
	Bdt_NetManager* netManager = BdtStackGetNetManager(stack);

	CallSession* callSession = callClient->callSession;
	const BdtPeerid* snPeerid = BdtPeerInfoGetPeerid(callClient->snPeerInfo);

	TcpInterfaceVector createTcpInterfaceVector;
	bfx_vector_tcp_interface_init(&createTcpInterfaceVector);
	bfx_vector_tcp_interface_reserve(&createTcpInterfaceVector, remoteEndpointCount * localEndpointCount);

	for (size_t ri = 0; ri < remoteEndpointCount; ri++)
	{
		const BdtEndpoint* remoteEp = remoteEndpointArray[ri];

		for (size_t li = 0; li < localEndpointCount; li++)
		{
			Bdt_TcpInterface* tcpInterface = NULL;
			uint32_t errorCode = Bdt_NetManagerCreateTcpInterface(
				netManager,
				localEndpointArray[li],
				remoteEp,
				&tcpInterface);
			if (errorCode == BFX_RESULT_SUCCESS && tcpInterface != NULL)
			{
				bfx_vector_tcp_interface_push_back(&createTcpInterfaceVector, tcpInterface);
			}
		}
	}

	if (createTcpInterfaceVector.size > 0)
	{
		BfxThreadLockLock(&callClient->lock);

		for (size_t i = 0; i < createTcpInterfaceVector.size; i++)
		{
			Bdt_TcpInterface* tcpInterface = createTcpInterfaceVector.tcpInterfaces[i];
			Bdt_TcpInterfaceAddRef(tcpInterface);
			bfx_vector_tcp_interface_push_back(&callClient->tcpInterfaceVector, tcpInterface);
		}

		BfxThreadLockUnlock(&callClient->lock);
	}

	int failedCount = 0;
	TcpInterfaceVector* failedTcpInterfaceVector = &createTcpInterfaceVector;

	for (size_t ti = 0; ti < createTcpInterfaceVector.size; ti++)
	{
		Bdt_TcpInterface* tcpInterface = createTcpInterfaceVector.tcpInterfaces[ti];

		if (callSession->encrypto)
		{
			Bdt_TcpInterfaceSetAesKey(tcpInterface, callClient->aesKey);
		}

		Bdt_TcpInterfaceState state = BDT_TCP_INTERFACE_STATE_NONE;
		Bdt_TcpInterfaceSetOwner(tcpInterface, CallClientInitAsTcpInterfaceOwner(callClient));

		CallClientAddRef(callClient);
		uint32_t connectResult = Bdt_TcpInterfaceConnect(tcpInterface);

		{
#ifndef BLOG_DISABLE
			char snEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
			char localEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
			BLOG_HEX(snPeerid->pid,
				15,
				"callsession.Connect, session:0x%p, seq:%d, client:0x%p, snEp:%s, localEp:%s, tcpInterface:0x%p.",
				callClient->callSession,
				callClient->callSession->seq,
				callClient,
				(BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), snEndpointStr), snEndpointStr),
				(BdtEndpointToString(Bdt_TcpInterfaceGetLocal(tcpInterface), localEndpointStr), localEndpointStr),
				tcpInterface);
#endif
		}
		

		if (connectResult != BFX_RESULT_SUCCESS && connectResult != BFX_RESULT_PENDING)
		{
			CallClientRelease(callClient);
			Bdt_TcpInterfaceRelease(tcpInterface); // 连接本身持有的计数
			failedCount++;
		}
		else
		{
			failedTcpInterfaceVector->tcpInterfaces[ti] = NULL;
		}
	}

	if (failedCount > 0)
	{
		// 检视callClient->tcpInterfaceVector，搜索其中失败的连接并记录到clearRefTcpInterfaceVector
		TcpInterfaceVector* clearRefTcpInterfaceVector = failedTcpInterfaceVector;

		// 清理tcp连接列表中的数据
		BfxThreadLockLock(&callClient->lock);

		for (size_t i = 0; i < callClient->tcpInterfaceVector.size; i++)
		{
			Bdt_TcpInterface* tcpInterface = callClient->tcpInterfaceVector.tcpInterfaces[i];
			if (failedTcpInterfaceVector->tcpInterfaces[i] != NULL)
			{
				if (tcpInterface != NULL)
				{
					callClient->tcpInterfaceVector.tcpInterfaces[i] = NULL;
				}
				else
				{
					// 已经清理过(可能收到close或者收到响应全部处理)，不需要再处理
					clearRefTcpInterfaceVector->tcpInterfaces[i] = NULL;
				}
			}
		}

		BfxThreadLockUnlock(&callClient->lock);

		for (size_t fi = 0; fi < clearRefTcpInterfaceVector->size; fi++)
		{
			Bdt_TcpInterface* tcpInterface = clearRefTcpInterfaceVector->tcpInterfaces[fi];
			if (tcpInterface != NULL)
			{
				Bdt_TcpInterfaceRelease(tcpInterface); // 清理callClient->tcpInterfaceVector持有的计数
			}
		}
	}
}

static void CallClientSendCallUdp(CallClient* callClient,
	Bdt_PackageWithRef** pkgsWithRef,
	const Bdt_UdpInterface* const* fromInterfaceArray,
	size_t fromInterfaceCount,
	const BdtEndpoint* toEndpointArray,
	size_t toEndpointCount
)
{
	CallSession* callSession = callClient->callSession;
	BdtStack* stack = callSession->owner->stack;
	BdtSystemFramework* framework = BdtStackGetFramework(stack);

	uint32_t result = BFX_RESULT_SUCCESS;
	// Bdt_StackTlsData* tls = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	Bdt_StackTlsData tlsObj;
	Bdt_StackTlsData* tls = &tlsObj;
	size_t encodedSize = sizeof(tls->udpEncodeBuffer);

    BLOG_CHECK(pkgsWithRef[0] != NULL);

    Bdt_Package* pkgs[2] = { Bdt_PackageWithRefGet(pkgsWithRef[0]), NULL };
    if (pkgsWithRef[1] != NULL) {
        pkgs[1] = Bdt_PackageWithRefGet(pkgsWithRef[1]);
    }

	if (pkgs[0]->cmdtype != BDT_EXCHANGEKEY_PACKAGE)
	{
        BLOG_CHECK(pkgs[1] == NULL);
		if (callSession->encrypto)
		{
			result = Bdt_BoxEncryptedUdpPackage(
				(const Bdt_Package**)pkgs, 
				1, 
				NULL, 
				callClient->aesKey, 
				tls->udpEncodeBuffer, 
				&encodedSize);
		}
		else
		{
			result = Bdt_BoxUdpPackage(
				(const Bdt_Package**)pkgs, 
				1, 
				NULL, 
				tls->udpEncodeBuffer, 
				&encodedSize);
		}
	}
	else
	{
		BLOG_CHECK(callSession->encrypto);
        BLOG_CHECK(pkgs[1] != NULL);

        Bdt_ExchangeKeyPackage* exchgPkg = (Bdt_ExchangeKeyPackage*)pkgs[0];
        exchgPkg->sendTime = BfxTimeGetNow(false);

		const BdtPeerSecret* secret = BdtStackGetSecret(stack);
		const BdtPeerConstInfo* snPeerConstInfo = BdtPeerInfoGetConstInfo(callClient->snPeerInfo);
		result = Bdt_BoxEncryptedUdpPackageStartWithExchange(
			pkgs,
			2,
			NULL, 
			callClient->aesKey,
			snPeerConstInfo->publicKeyType,
			(const uint8_t*)& snPeerConstInfo->publicKey,
			secret->secretLength,
			(const uint8_t*)secret,
			tls->udpEncodeBuffer,
			&encodedSize);
	}

	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("session:0x%p, seq:%d, client:0x%p, udp package box failed:result:%d",
			callSession, callSession->seq, callClient, result);
		return;
	}

	for (size_t i = 0; i < fromInterfaceCount; i++)
	{
		const Bdt_UdpInterface* volatile udpInterface = fromInterfaceArray[i];
		for (size_t j = 0; j < toEndpointCount; j++)
		{
			{
#ifndef BLOG_DISABLE
				char snEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
				char localEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
				BLOG_INFO("session:0x%p, seq:%d, client:0x%p, send from:%s, to:%s.",
					callSession,
					callSession->seq,
					callClient,
					(BdtEndpointToString(Bdt_UdpInterfaceGetLocal(udpInterface), localEndpointStr), localEndpointStr),
					(BdtEndpointToString(&toEndpointArray[j], snEndpointStr), snEndpointStr)
				);
#endif
			}

			BdtUdpSocketSendTo(framework, Bdt_UdpInterfaceGetSocket(udpInterface), &toEndpointArray[j], tls->udpEncodeBuffer, encodedSize);
		}
	}
}

static void CallClientSendCallTcp(CallClient* callClient,
	Bdt_TcpInterface* tcpInterface,
	Bdt_PackageWithRef** pkgsWithRef
)
{
	BdtStack* stack = callClient->callSession->owner->stack;

	uint32_t result = BFX_RESULT_SUCCESS;
	// Bdt_StackTlsData* tls = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	Bdt_StackTlsData tlsObj;
	Bdt_StackTlsData* tls = &tlsObj;
	size_t encodedSize = sizeof(tls->udpEncodeBuffer);

    BLOG_CHECK(pkgsWithRef[0] != NULL);

    Bdt_Package* pkgs[2] = { Bdt_PackageWithRefGet(pkgsWithRef[0]), NULL };
    size_t pkgCount = 1;
    if (pkgsWithRef[1] != NULL)
    {
        pkgs[1] = Bdt_PackageWithRefGet(pkgsWithRef[1]);
        pkgCount = 2;
    }

    if (pkgs[0]->cmdtype == BDT_EXCHANGEKEY_PACKAGE)
    {
        Bdt_ExchangeKeyPackage* exchgPkg = (Bdt_ExchangeKeyPackage*)pkgs[0];
        exchgPkg->sendTime = BfxTimeGetNow(false);
    }

	size_t pkgSize = 0;
	const BdtPeerConstInfo* snPeerConstInfo = BdtPeerInfoGetConstInfo(callClient->snPeerInfo);
	const BdtPeerSecret* secret = BdtStackGetSecret(stack);
	result = Bdt_TcpInterfaceBoxFirstPackage(tcpInterface,
		pkgs,
		pkgCount,
		tls->udpEncodeBuffer,
		encodedSize,
		&pkgSize,
		snPeerConstInfo->publicKeyType,
		(const uint8_t*)& snPeerConstInfo->publicKey,
		secret->secretLength,
		(const uint8_t*)& secret->secret);


	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("session:0x%p, seq:%d, client:0x%p, pkgCount:%d, tcp package box failed:result:%d",
			callClient->callSession, callClient->callSession->seq, callClient, pkgCount, result);
		return;
	}

	{
#ifndef BLOG_DISABLE
		char snEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		char localEndpointStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BLOG_INFO("session:0x%p, seq:%d, client:0x%p, send from:%s, to:%s, pkgCount:%d.",
			callClient->callSession,
			callClient->callSession->seq,
			callClient,
			(BdtEndpointToString(Bdt_TcpInterfaceGetLocal(tcpInterface), localEndpointStr), localEndpointStr),
			(BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), snEndpointStr), snEndpointStr),
			pkgCount);
#endif
	}
	

	size_t sent = 0;
	uint32_t errorCode = Bdt_TcpInterfaceSend(tcpInterface, tls->udpEncodeBuffer, pkgSize, &sent);

	BLOG_CHECK(errorCode == BFX_RESULT_SUCCESS || errorCode == BFX_RESULT_PENDING || callClient->lastRespTime != 0);
}

static void CallClientTryResendCallAllUdp(CallClient* callClient)
{
	size_t toEndpointCount = 1;
	const BdtEndpoint* toEndpointArray = &callClient->usingEndpoint;
	size_t interfaceCount = 0;
	const Bdt_NetListener* netListener = NULL;
	const Bdt_UdpInterface* usingUdpInterface = callClient->usingUdpInterface;
	const Bdt_UdpInterface* const* interfaceList = NULL;
    CallSession* callSession = callClient->callSession;

	if (usingUdpInterface)
	{
		interfaceList = &usingUdpInterface;
		interfaceCount = 1;
	}
	else
	{
		BdtStack* stack = callClient->callSession->owner->stack;
		netListener = Bdt_NetManagerGetListener(BdtStackGetNetManager(stack));
		toEndpointArray = BdtPeerInfoListEndpoint(callClient->snPeerInfo, &toEndpointCount);
		interfaceList = Bdt_NetListenerListUdpInterface(netListener, &interfaceCount);
		if (interfaceCount == 0 || toEndpointCount == 0)
		{
			BLOG_ERROR("session:0x%p, seq:%d client:0x%p, no endpoint found: listenCount:%d, toCount:%d",
				callClient->callSession, callClient->callSession->seq, callClient, interfaceCount, toEndpointCount);
		}
	}

	if (interfaceCount > 0 && toEndpointCount > 0)
	{
		CallClientSendCallUdp(callClient,
			callSession->pkgs,
			interfaceList,
			interfaceCount,
			toEndpointArray,
			toEndpointCount);

	}

	if (netListener)
	{
		Bdt_NetListenerRelease(netListener);
	}
}

static void BFX_STDCALL CallSessionTimerProcess(BDT_SYSTEM_TIMER t, CallSession* session)
{
	Bdt_TimerHelperStop(&session->timer);

	if (session->callback == NULL)
	{
		return;
	}

	int64_t now = BfxTimeGetNow(false);
	if (now >= session->endTime)
	{
		for (size_t i = 0; i < session->snCount; i++)
		{
			CallClientOnTimeout(session->callClients + i);
		}

		CallSessionCallbackNotify(session, NULL);
		return;
	}

	BLOG_CHECK(session != NULL);

	BfxUserData ud = {
		.userData = session,
		.lpfnAddRefUserData = CallSessionAddRefAsUserData,
		.lpfnReleaseUserData = CallSessionReleaseAsUserData
	};

	Bdt_TimerHelperStart(&session->timer,
		(LPFN_TIMER_PROCESS)CallSessionTimerProcess,
		&ud,
		100);
}

static void CallSessionDestroy(CallSession* callSession)
{
	if (callSession == NULL)
	{
		BLOG_WARN("session:0x%p, maybe already destroyed.", callSession);
		return;
	}

	for (uint32_t i = 0; i < callSession->snCount; i++)
	{
		CallClient* client = &callSession->callClients[i];
		BdtReleasePeerInfo(client->snPeerInfo);
		CallClientClearTcpInterfaces(client);

		if (client->usingUdpInterface != NULL)
		{
			Bdt_UdpInterfaceRelease(client->usingUdpInterface);
		}
		BfxThreadLockDestroy(&client->lock);
	}
    callSession->snCount = 0;

	if (callSession->payload != NULL)
	{
		BfxBufferRelease(callSession->payload);
		callSession->payload = NULL;
	}
	
	BdtEndpointArrayUninit(&callSession->reverseEndpointArray);
	Bdt_TimerHelperUninit(&callSession->timer);

	if (callSession->userData.userData != NULL &&
		callSession->userData.lpfnReleaseUserData != NULL)
	{
		callSession->userData.lpfnReleaseUserData(callSession->userData.userData);
	}

    for (size_t pi = 0; pi < BFX_ARRAYSIZE(callSession->pkgs); pi++)
    {
        if (callSession->pkgs[pi] != NULL)
        {
            Bdt_PackageRelease(callSession->pkgs[pi]);
        }
    }

    BdtReleasePeerInfo(callSession->localPeerInfo);

	BfxFree(callSession);
}

static void CallClientClearTcpInterfaces(CallClient* callClient)
{
	BLOG_CHECK(callClient != NULL);
	BLOG_WARN("session:0x%p, seq:%d, client:0x%p.",
		callClient->callSession, callClient->callSession->seq, callClient);

	BfxThreadLockLock(&callClient->lock);
	TcpInterfaceVector tcpInterfaceVector;
	memcpy(&tcpInterfaceVector, &callClient->tcpInterfaceVector, sizeof(TcpInterfaceVector));
	bfx_vector_tcp_interface_init(&callClient->tcpInterfaceVector);
	BfxThreadLockUnlock(&callClient->lock);

	Bdt_TcpInterface* tcpInterface = NULL;
	for (size_t j = 0; j < tcpInterfaceVector.size; j++)
	{
		tcpInterface = tcpInterfaceVector.tcpInterfaces[j];
		if (tcpInterface != NULL)
		{
			Bdt_TcpInterfaceReset(tcpInterface);
			Bdt_TcpInterfaceRelease(tcpInterface);
			tcpInterfaceVector.tcpInterfaces[j] = NULL;
		}
	}
	bfx_vector_tcp_interface_cleanup(&tcpInterfaceVector);
}

static void CallClientUpdateResendInterval(CallClient* callClient)
{
	callClient->resendInterval <<= 1;
}

typedef struct FindPriorityEndpointPairFromHistoryUserData
{
	BdtEndpoint* local;
	BdtEndpoint* remote;
	bool found;
} FindPriorityEndpointPairFromHistoryUserData;

static void CallClientFindHistoryCallback(const BdtHistory_PeerInfo* peerHistory, void* userData)
{
	FindPriorityEndpointPairFromHistoryUserData* epPair = (FindPriorityEndpointPairFromHistoryUserData*)userData;
	epPair->found = false;
	
	if (peerHistory == NULL)
	{
		return;
	}

	int64_t lastConnectTime = 0;
	int64_t selectConnectTime = 0;
	bool isSelectTcp = false;
	// 选取最后成功连接前半秒内最早响应的节点(优先TCP)
	for (size_t i = 0; i < peerHistory->connectVector.size; i++)
	{
		const BdtHistory_PeerConnectInfo* connectInfo = peerHistory->connectVector.connections[i];
		if (connectInfo != NULL)
		{
			if (connectInfo->lastConnectTime > lastConnectTime)
			{
				lastConnectTime = connectInfo->lastConnectTime;
			}
			if (lastConnectTime - connectInfo->lastConnectTime > 500)
			{
				continue;
			}

			if (isSelectTcp)
			{
				if (!connectInfo->flags.directTcp)
				{
					continue;
				}
			}
			else if (connectInfo->flags.directTcp)
			{
				epPair->found = true;
				isSelectTcp = true;
				selectConnectTime = connectInfo->lastConnectTime;
				*epPair->remote = connectInfo->endpoint;
				if (connectInfo->localEndpoint != NULL)
				{
					*epPair->local = *connectInfo->localEndpoint;
				}
				else
				{
					epPair->local->port = 0;
				}
			}

			if (connectInfo->lastConnectTime < selectConnectTime)
			{
				epPair->found = true;
				selectConnectTime = connectInfo->lastConnectTime;
				if (connectInfo->localEndpoint != NULL)
				{
					*epPair->local = *connectInfo->localEndpoint;
				}
				else
				{
					epPair->local->port = 0;
				}
			}
		}
	}
}

static bool CallClientFindPriorityEndpointByHistory(CallClient* callClient, BdtEndpoint* local, BdtEndpoint* remote)
{
	BdtStack* stack = callClient->callSession->owner->stack;
	BdtHistory_PeerUpdater* updater = BdtStackGetHistoryPeerUpdater(stack);

	FindPriorityEndpointPairFromHistoryUserData epPair = { .local = local,.remote = remote, .found = false };
	BfxUserData ud = {
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL,
		.userData = &epPair
	};

	uint32_t result = BdtHistory_PeerUpdaterFindByPeerid(updater,
		BdtPeerInfoGetPeerid(callClient->snPeerInfo),
		false,
		CallClientFindHistoryCallback,
		&ud);

	return result == BFX_RESULT_SUCCESS && epPair.found;
}

static int CallClientTryCallPriorityEndpointTcp(CallClient* callClient,
	const BdtEndpoint* local,
	const BdtEndpoint* remote)
{
    CallSession* callSession = callClient->callSession;
	BdtStack* stack = callSession->owner->stack;

	const BdtEndpoint* localTargetEpArray[32];
	const BdtEndpoint** localPtrPtr = localTargetEpArray;
	size_t targetLocalEndpointCount = 0;
	size_t localEndpointTotalCount = 0;

	// 尝试搜索和历史记录匹配的本地地址，如果找不到就用本地所有地址测试
	const BdtPeerInfo* localPeerInfo = callSession->localPeerInfo;
    BdtAddRefPeerInfo(localPeerInfo);
	const BdtEndpoint* localEndpointList = BdtPeerInfoListEndpoint(localPeerInfo, &localEndpointTotalCount);

	for (size_t i = 0;
		i < localEndpointTotalCount && targetLocalEndpointCount < BFX_ARRAYSIZE(localTargetEpArray);
		i++)
	{
		const BdtEndpoint* curEndpoint = localEndpointList + i;
		if ((curEndpoint->flag & BDT_ENDPOINT_PROTOCOL_TCP) == (remote->flag & BDT_ENDPOINT_PROTOCOL_TCP) &&
			(curEndpoint->flag & BDT_ENDPOINT_PROTOCOL_UDP) == (remote->flag & BDT_ENDPOINT_PROTOCOL_UDP))
		{
			if (local->port != 0 && CallSessionIsSameEndpoint(curEndpoint, local))
			{
				localPtrPtr = &local;
				targetLocalEndpointCount = 1;
				break;
			}
			localTargetEpArray[targetLocalEndpointCount] = curEndpoint;
			targetLocalEndpointCount++;
		}
	}

	// 现在tcp都是用0地址
	const BdtEndpoint endpointAny = {
		.flag = BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 0,
		.address = {0, 0, 0, 0}
	};
	const BdtEndpoint* endpointAnyPtr = &endpointAny;
	if (targetLocalEndpointCount == 0)
	{
		localPtrPtr = &endpointAnyPtr;
		targetLocalEndpointCount = 1;
	}

	CallClientTryConnectTcp(callClient, localPtrPtr, targetLocalEndpointCount, &remote, 1);

	BdtReleasePeerInfo(localPeerInfo);
	return (int)targetLocalEndpointCount;
}

static int CallClientTryCallPriorityEndpointUdp(CallClient* callClient,
	const BdtEndpoint* local,
	const BdtEndpoint* remote)
{
	size_t interfaceCount = 0;
	const Bdt_UdpInterface* const* interfaceList = NULL;

    CallSession* callSession = callClient->callSession;
	BdtStack* stack = callSession->owner->stack;
	const Bdt_NetListener* netListener = Bdt_NetManagerGetListener(BdtStackGetNetManager(stack));
	interfaceList = Bdt_NetListenerListUdpInterface(netListener, &interfaceCount);
	if (interfaceCount == 0)
	{
		BLOG_ERROR("session:0x%p, seq:%d client:0x%p, no endpoint found: listenCount:%d",
			callSession, callSession->seq, callClient, interfaceCount);
	}

	if (interfaceCount > 0)
	{
		const Bdt_UdpInterface* targetInterface = NULL;
		for (size_t i = 0; i < interfaceCount; i++)
		{
			targetInterface = interfaceList[i];
			if (CallSessionIsSameEndpoint(Bdt_UdpInterfaceGetLocal(targetInterface), local))
			{
				interfaceList = &targetInterface;
				interfaceCount = 1;
				break;
			}
		}

		CallClientSendCallUdp(callClient,
			callSession->pkgs,
			interfaceList,
			interfaceCount,
			remote,
			1);

	}

	if (netListener)
	{
		Bdt_NetListenerRelease(netListener);
	}

	return (int)interfaceCount;
}

static int CallClientCallPriorityEndpoints(CallClient* callClient)
{
	BdtEndpoint local;
	BdtEndpoint remote;

	if (!CallClientFindPriorityEndpointByHistory(callClient, &local, &remote))
	{
		return 0;
	}


	if ((remote.flag & BDT_ENDPOINT_PROTOCOL_TCP) != 0)
	{
		return CallClientTryCallPriorityEndpointTcp(callClient, &local, &remote);
	}
	else
	{
		return CallClientTryCallPriorityEndpointUdp(callClient, &local, &remote);
	}
}

static void CallClientOnTimeout(CallClient* callClient)
{
	CallClientClearTcpInterfaces(callClient);
}