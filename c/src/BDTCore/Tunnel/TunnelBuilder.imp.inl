#ifndef __BDT_TUNNEL_BUILDER_IMP_INL__
#define __BDT_TUNNEL_BUILDER_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include <SGLib/SGLib.h>
#include <assert.h>
#include "./TunnelBuildAction.inl"
#include "./TunnelBuilder.inl"
#include "./TunnelContainer.inl"
#include "./TcpTunnel.inl"
#include "./UdpTunnel.inl"

void Bdt_BuildTunnelParamsClone(const BdtBuildTunnelParams* src, BdtBuildTunnelParams* dst)
{
	dst->flags = src->flags;
    if (src->remoteConstInfo != NULL)
    {
	    dst->remoteConstInfo = (BdtPeerConstInfo*)BfxMalloc(sizeof(BdtPeerConstInfo));
	    *(BdtPeerConstInfo*)(dst->remoteConstInfo) = *(src->remoteConstInfo);
    }
    else
    {
        dst->remoteConstInfo = NULL;
    }
	
	if (src->flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_KEY)
	{
		// TODO
	}
	if (src->flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_SN)
	{
		dst->snListLength = src->snListLength;
		dst->snList = (BdtPeerid*)BfxMalloc(sizeof(BdtPeerid) * src->snListLength);
		for (size_t ix = 0; ix < src->snListLength; ++ix)
		{
			*(BdtPeerid*)(dst->snList + ix) = *(src->snList + ix);
		}
	}
	if (src->flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_ENDPOINT)
	{
		dst->localEndpoint = (BdtEndpoint*)BfxMalloc(sizeof(BdtEndpoint));
		*(BdtEndpoint*)(dst->localEndpoint) = *(src->localEndpoint);
		dst->remoteEndpoint = (BdtEndpoint*)BfxMalloc(sizeof(BdtEndpoint));
		*(BdtEndpoint*)(dst->remoteEndpoint) = *(src->remoteEndpoint);
	}
}

void Bdt_BuildTunnelParamsUninit(BdtBuildTunnelParams* params)
{
	if (params->localEndpoint)
	{
		BfxFree((void*)params->localEndpoint);
	}
	if (params->remoteEndpoint)
	{
		BfxFree((void*)params->remoteEndpoint);
	}
	if (params->remoteConstInfo)
	{
		BfxFree((void*)params->remoteConstInfo);
	}
	if (params->snList)
	{
		BfxFree((void*)params->snList);
	}
	if (params->key)
	{
		BfxFree((void*)params->key);
	}
	memset(params, 0, sizeof(BdtBuildTunnelParams));
}


#define TUNNEL_BUILDER_FLAG_PASSIVE					(1<<6)
#define TUNNEL_BUILDER_FLAG_BUILD_CONNECTION		(1<<7)


static void BFX_STDCALL TunnelBuilderAsRefUserDataAddRef(void* userData)
{
	Bdt_TunnelBuilderAddRef((TunnelBuilder*)userData);
}
static void BFX_STDCALL TunnelBuilderAsRefUserDataRelease(void* userData)
{
	Bdt_TunnelBuilderRelease((TunnelBuilder*)userData);
}

static void TunnelBuilderAsRefUserData(TunnelBuilder* builder, BfxUserData* outUserData)
{
	outUserData->userData = builder;
	outUserData->lpfnAddRefUserData = TunnelBuilderAsRefUserDataAddRef;
	outUserData->lpfnReleaseUserData = TunnelBuilderAsRefUserDataRelease;
}

static TunnelBuilder* TunnelBuilderCreate(
	Bdt_TunnelContainer* container, 
	const BdtTunnelBuilderConfig* config,
	BdtSystemFramework* framework, 
	Bdt_StackTls* tls, 
	BdtPeerFinder* peerFinder,
	BdtHistory_KeyManager* keyManager,
	Bdt_NetManager* netManager,
	BdtSuperNodeClient* snClient
)
{
	TunnelBuilder* builder = (TunnelBuilder*)BfxMalloc(sizeof(TunnelBuilder));
	memset(builder, 0, sizeof(TunnelBuilder));
	builder->refCount = 1;
	*(BdtTunnelBuilderConfig*)& builder->config = *config;
	// no add ref
	// make sure container's life cycle longer than builder!
	builder->framework = framework;
	builder->tls = tls;
	builder->container = container;
	builder->peerFinder = peerFinder;
	builder->keyManager = keyManager;
	builder->netManager = netManager;
	builder->snClient = snClient;
	BfxRwLockInit(&builder->stateLock);
	return builder;
}

int32_t Bdt_TunnelBuilderAddRef(TunnelBuilder* builder)
{
	return BfxAtomicInc32(&builder->refCount);
}

int32_t Bdt_TunnelBuilderRelease(TunnelBuilder* builder)
{
	int32_t refCount = BfxAtomicDec32(&builder->refCount);
	if (refCount <= 0)
	{
		if (builder->connection)
		{
			BdtReleaseConnection(builder->connection);
			builder->connection = NULL;
		}
		if (builder->firstPackages)
		{
			for (size_t ix = 0; ix < builder->firstPackageLength; ++ix)
			{
				Bdt_PackageRelease(builder->firstPackages[ix]);
			}
		}
		Bdt_BuildTunnelParamsUninit(&builder->params);
	}
	return refCount;
}

static uint32_t TunnelBuilderGetSeq(TunnelBuilder* builder)
{
	return builder->seq;
}

static void TunnelBuilderGenSynPackages(
	TunnelBuilder* builder, 
	const Bdt_PackageWithRef** firstPackage, 
	size_t firstPackageCount)
{
	// one for tunnel syn, one for first package in request
	const Bdt_PackageWithRef** packages = BFX_MALLOC_ARRAY(Bdt_PackageWithRef*, TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH);
	size_t packageLen = 0;
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_SYN_TUNNEL_PACKAGE);
	Bdt_SynTunnelPackage* synTunnel = (Bdt_SynTunnelPackage*)Bdt_PackageWithRefGet(pkg);
	Bdt_SynTunnelPackageInit(synTunnel);
	synTunnel->fromPeerid = *BdtTunnel_GetLocalPeerid(builder->container);
	synTunnel->toPeerid = *BdtTunnel_GetRemotePeerid(builder->container);
	memset(&synTunnel->proxyPeerid, 0, sizeof(BdtPeerid));
	synTunnel->seq = builder->seq;
	synTunnel->fromContainerId = BDT_UINT32_ID_INVALID_VALUE;
	synTunnel->proxyPeerInfo = NULL;
	synTunnel->peerInfo = BdtPeerFinderGetLocalPeer(builder->peerFinder);
	packages[packageLen++] = pkg;
	for (size_t ix = 0; ix < firstPackageCount; ++ix)
	{
		packages[packageLen++] = firstPackage[ix];
		Bdt_PackageAddRef(firstPackage[ix]);
	}
	builder->firstPackageLength = packageLen;
	builder->firstPackages = packages;
}

static void TunnelBuilderGenAckPackages(
	TunnelBuilder* builder
)
{
	const Bdt_PackageWithRef** packages = BFX_MALLOC_ARRAY(Bdt_PackageWithRef*, TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH);
	size_t packageLen = 0;
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_ACK_TUNNEL_PACKAGE);
	Bdt_AckTunnelPackage* ackTunnel = (Bdt_AckTunnelPackage*)Bdt_PackageWithRefGet(pkg);
	Bdt_AckTunnelPackageInit(ackTunnel);
	ackTunnel->seq = builder->seq;
	ackTunnel->peerInfo = BdtPeerFinderGetLocalPeer(builder->peerFinder);
	packages[packageLen++] = pkg;

	builder->firstPackageLength = packageLen;
	builder->firstPackages = packages;
}

static bool TunnelBuilderIsPassive(TunnelBuilder* builder)
{
	return builder->buildingFlags & TUNNEL_BUILDER_FLAG_PASSIVE;
}

static bool TunnelBuilderIsForConnection(TunnelBuilder* builder)
{
	return builder->buildingFlags & TUNNEL_BUILDER_FLAG_BUILD_CONNECTION;
}

static const Bdt_PackageWithRef** TunnelBuilderGetFirstPackages(TunnelBuilder* builder, size_t* outPackageLength)
{
	*outPackageLength = builder->firstPackageLength;
	return builder->firstPackages;
}

typedef struct SimpleConnectConnectionStrategy
{
	void* reserved;
} SimpleConnectConnectionStrategy;

static void SimpleConnectConnectionStrategyOnTunnelActive(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	void* userData)
{
	SimpleConnectConnectionStrategy* strategy = (SimpleConnectConnectionStrategy*)userData;
	// once tunnel active, use this tunnel to continue connect
	ConnectConnectionActionContinueConnect((ConnectConnectionAction*)action);
}


static void SimpleConnectConnectionStrategyOnConnectFinish(BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result,
	void* userData)
{
	SimpleConnectConnectionStrategy* strategy = (SimpleConnectConnectionStrategy*)userData;
	// once connection connected, finish runtime
	if (result == BFX_RESULT_SUCCESS)
	{
		// ignore syn udp tunnel actions
		if (remote->flag & BDT_ENDPOINT_PROTOCOL_TCP || remote->port == 0)
		{
			BuildActionRuntimeFinishBuilder(runtime);
		}
	}
}


static void SimpleConnectConnectionStrategyAsUserData(SimpleConnectConnectionStrategy* strategy, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = NULL;
	userData->lpfnReleaseUserData = NULL;
	userData->userData = NULL;
}

static void SimpleConnectConnectionStrategyTryConnectAllTunnel(
	SimpleConnectConnectionStrategy* strategy,
	BuildActionRuntime* runtime,
	const BdtPeerid* snList,
	size_t snListLength
);


static void SimpleConnectConnectionStrategyOnTimeout(BuildAction* baseAction, void* userData)
{
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}


static void SimpleConnectConnectionStrategyOnFindPeerResolved(BuildAction* baseAction, void* userData)
{
	SimpleConnectConnectionStrategy* strategy = (SimpleConnectConnectionStrategy*)userData;
	const BdtPeerInfo* remoteInfo = BuildActionRuntimeGetRemoteInfo(baseAction->runtime);
	if (!remoteInfo)
	{
		BLOG_DEBUG("%s find remote peer info failed", baseAction->name);
		SimpleConnectConnectionStrategyOnTimeout(baseAction, userData);
		return;
	}
	BdtReleasePeerInfo(remoteInfo);
	SimpleConnectConnectionStrategyTryConnectAllTunnel(strategy, baseAction->runtime, NULL, 0);
}

static void SimpleConnectConnectionStrategyOnFindPeerRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}

static void SimpleConnectConnectionStrategyTryConnectAllTunnel(
	SimpleConnectConnectionStrategy* strategy,
	BuildActionRuntime* runtime,
	const BdtPeerid* snList,
	size_t snListLength
)
{
	//TODO:可读性 ListBuildActionAllCreate和ListBuildActionRaceCreate真的看不出语义的区别，取个好名字吧..
	ListBuildAction* waitTimeout = ListBuildActionAllCreate(runtime, "AllActions");
	// tofix: timeout and tick cycle

	BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(
		runtime, 
		Bdt_ConnectionGetConfig(runtime->builder->connection)->connectTimeout,
		200, 
		"ConnectTimeoutAction");
	ListBuildActionAdd(waitTimeout, (BuildAction*)timeoutAction);
	BuildActionRelease((BuildAction*)timeoutAction);

	bool checkSnCall = true;
	const Bdt_NetListener* listener = Bdt_NetManagerGetListener(runtime->builder->netManager);
	size_t localEpLen = 0;
	const BdtEndpoint* localEpList = Bdt_NetListenerGetBoundAddr(listener, &localEpLen);
	size_t remoteEpLen = 0;
	const BdtPeerInfo* remoteInfo = BuildActionRuntimeGetRemoteInfo(runtime);
	const BdtEndpoint* remoteEpList = BdtPeerInfoListEndpoint(remoteInfo, &remoteEpLen);
	size_t localUdpLen = 0;
	const Bdt_UdpInterface* const* localUdpList = Bdt_NetListenerListUdpInterface(listener, &localUdpLen);
	
	//TODO:这里的策略需要优化，应该根据双方的endpoint情况进行一些分支判断，M*N遍历后应该是把Action放在不同的list中
	//     然后选择执行什么再执行什么
	for (size_t ix = 0; ix < remoteEpLen; ++ix)
	{
		const BdtEndpoint* re = remoteEpList + ix;
		if (re->flag & BDT_ENDPOINT_IP_VERSION_6
			&& !(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
		{
			// ignore all ipv6 endpoint not wan static
			continue;
		}
		if (re->flag & BDT_ENDPOINT_PROTOCOL_UDP)
		{
			for (size_t iy = 0; iy < localUdpLen; ++iy)
			{
				const Bdt_UdpInterface* udpInterface = *(localUdpList + iy);
				if (!BdtEndpointIsSameIpVersion(re, Bdt_UdpInterfaceGetLocal(udpInterface)))
				{
					continue;
				}
				if (re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN)
				{
					//TODO:一定能放弃么？会存在更新不及时的情况？
					BLOG_DEBUG("%s remote has static wan address, ignore sn call", Bdt_TunnelContainerGetName(runtime->builder->container));
					checkSnCall = false;
				}
				BuildAction* synAction = (BuildAction*)CreateSynUdpTunnelAction(runtime, timeoutAction, udpInterface, re);
				BuildActionRelease(synAction);
			}
		}
		else if (re->flag & BDT_ENDPOINT_PROTOCOL_TCP)
		{
			for (size_t iy = 0; iy < localEpLen; ++iy)
			{
				const BdtEndpoint* le = localEpList + iy;
				if (!BdtEndpointIsSameIpVersion(re, le))
				{
					continue;
				}
				if (re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN)
				{
					//TODO:一定能放弃么？会存在更新不及时的情况？
					BLOG_DEBUG("%s remote has static wan address, ignore sn call", Bdt_TunnelContainerGetName(runtime->builder->container));
					checkSnCall = false;
				}
				BdtEndpoint localTcp = *le;
				localTcp.flag |= BDT_ENDPOINT_PROTOCOL_TCP;
				BuildAction* connectAction = (BuildAction*)CreateConnectStreamAction(runtime, &localTcp, re);
				BuildActionRelease(connectAction);
			}
		}
	}
	{
		BuildAction* connectAction = (BuildAction*)CreateConnectPackageConnectionAction(runtime, timeoutAction);
		BuildActionRelease(connectAction);
	}

	if (checkSnCall)
	{
		ListBuildAction* callActions = ListBuildActionRaceCreate(runtime, "SnCallActions");
		for (size_t ix = 0; ix < snListLength; ++ix)
		{
			SnCallAction* callAction = CreateSnCallAction(runtime, snList + ix, false);
			ListBuildActionAdd(callActions, (BuildAction*)callAction);
			BuildActionRelease((BuildAction*)callAction);
		}
		ListBuildActionAdd(waitTimeout, (BuildAction*)callActions);
		BuildActionRelease((BuildAction*)callActions);
	}

	Bdt_NetListenerRelease(listener);
	BdtReleasePeerInfo(remoteInfo);
	
	BuildActionCallbacks waitTimeoutCallbacks;
	waitTimeoutCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION;
	waitTimeoutCallbacks.resolve.function.function = SimpleConnectConnectionStrategyOnTimeout;
	SimpleConnectConnectionStrategyAsUserData(strategy, &waitTimeoutCallbacks.resolve.function.userData);

	BuildActionExecute((BuildAction*)waitTimeout, &waitTimeoutCallbacks);

	BuildActionRelease((BuildAction*)waitTimeout);
}

static void SimpleConnectConnectionStrategyBegin(SimpleConnectConnectionStrategy* strategy, BuildActionRuntime* runtime)
{
	BuildActionRuntimeOnBegin(runtime);
	//TODO：重要，应该有一段注释，用来描述一下这个策略的基本思路。否则通过一堆代码和回调想看出来目的，还是比较难的。
	//      如果能用一段js伪代码来表达，就更好了
	
	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetCachedOrStatic(
		runtime->builder->peerFinder, 
		BdtTunnel_GetRemotePeerid(runtime->builder->container));


	if (remoteInfo)
	{
		BuildActionRuntimeUpdateRemoteInfo(runtime, remoteInfo);
		size_t snListLength = 0;
		const BdtPeerid* snList = BdtPeerInfoListSn(remoteInfo, &snListLength);
		SimpleConnectConnectionStrategyTryConnectAllTunnel(strategy, runtime, snList, snListLength);
	}
	else
	{
		if (runtime->builder->params.flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_SN)
		{
			//List是串行执行么?
			ListBuildAction* findPeerAction = ListBuildActionRaceCreate(runtime, "FindPeerActions");
			// tofix: timeout and tick cycle
			BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(runtime, 1000, 0, "FindPeerTimeout");
			//TODO：重要，找到remote peer可能注册的SNList也是有策略的，这个再说
			for (size_t ix = 0; ix < runtime->builder->params.snListLength; ++ix)
			{
				SnCallAction* callAction = CreateSnCallAction(runtime, runtime->builder->params.snList + ix, true);
				ListBuildActionAdd(findPeerAction, (BuildAction*)callAction);
				BuildActionRelease((BuildAction*)callAction);
			}
			ListBuildActionAdd(findPeerAction, (BuildAction*)timeoutAction);
			BuildActionRelease((BuildAction*)timeoutAction);

			BuildActionCallbacks findPeerCallbacks;
			findPeerCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
			//TODO 可读性 这个.function.function的写法实在不友好
			findPeerCallbacks.resolve.function.function = SimpleConnectConnectionStrategyOnFindPeerResolved;
			SimpleConnectConnectionStrategyAsUserData(strategy, &findPeerCallbacks.resolve.function.userData);
			findPeerCallbacks.reject.function.function = SimpleConnectConnectionStrategyOnFindPeerRejected;
			SimpleConnectConnectionStrategyAsUserData(strategy, &findPeerCallbacks.reject.function.userData);

			BuildActionExecute((BuildAction*)findPeerAction, &findPeerCallbacks);

			BuildActionRelease((BuildAction*)findPeerAction);
		}
		else
		{
			assert(false);
			// todo: use dht action to find peer
		}
	}
}


typedef struct SimpleAcceptConnectionStrategy
{
	void* reserved;
} SimpleAcceptConnectionStrategy;


static void SimpleAcceptConnectionStrategyOnTunnelActive(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	void* userData)
{
	SimpleAcceptConnectionStrategy* strategy = (SimpleAcceptConnectionStrategy*)userData;
	// do nothing
}


static void SimpleAcceptConnectionStrategyOnConnectFinish(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result,
	void* userData)
{
	SimpleAcceptConnectionStrategy* strategy = (SimpleAcceptConnectionStrategy*)userData;
	// once connection connected, finish runtime
	if (result == BFX_RESULT_SUCCESS)
	{
		// ignore syn udp tunnel actions
		if (remote->flag & BDT_ENDPOINT_PROTOCOL_TCP || remote->port == 0)
		{
			BuildActionRuntimeFinishBuilder(runtime);
		}
	}
}

static void SimpleAcceptConnectionStrategyOnTimeout(BuildAction* baseAction, void* userData)
{
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}

static void SimpleAcceptConnectionStrategyAsUserData(SimpleConnectConnectionStrategy* strategy, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = NULL;
	userData->lpfnReleaseUserData = NULL;
	userData->userData = NULL;
}

static void SimpleAcceptConnectionStrategyBegin(SimpleConnectConnectionStrategy* strategy, BuildActionRuntime* runtime)
{
	BuildActionRuntimeOnBegin(runtime);

	// cache must exists for remote peer info in SnCalled should has update cache
	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetCached(
		runtime->builder->peerFinder,
		BdtTunnel_GetRemotePeerid(runtime->builder->container));
	if (!remoteInfo)
	{
		BLOG_DEBUG("%s %u ignore build tunnel for remote info missed", Bdt_TunnelContainerGetName(runtime->builder->container), runtime->builder->seq);
		assert(false);
		return;
	}

	ListBuildAction* waitTimeout = ListBuildActionAllCreate(runtime, "AllActions");
	// tofix: timeout and tick cycle
	BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(
		runtime, 
		Bdt_ConnectionGetConfig(runtime->builder->connection)->connectTimeout, 
		200, 
		"ConnectTimeoutAction");
	ListBuildActionAdd(waitTimeout, (BuildAction*)timeoutAction);
	BuildActionRelease((BuildAction*)timeoutAction);

	const Bdt_NetListener* listener = Bdt_NetManagerGetListener(runtime->builder->netManager);
	size_t localEpLen = 0;
	const BdtEndpoint* localEpList = Bdt_NetListenerGetBoundAddr(listener, &localEpLen);
	size_t remoteEpLen = 0;
	const BdtEndpoint* remoteEpList = BdtPeerInfoListEndpoint(remoteInfo, &remoteEpLen);
	size_t localUdpLen = 0;
	const Bdt_UdpInterface* const* localUdpList = Bdt_NetListenerListUdpInterface(listener, &localUdpLen);
	for (size_t ix = 0; ix < remoteEpLen; ++ix)
	{
		const BdtEndpoint* re = remoteEpList + ix;
		{
			// ignore all ipv6 endpoint not wan static
			char szep[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
			BdtEndpointToString(re, szep);
			BLOG_DEBUG("%s %u try use remote ep %s",
				Bdt_TunnelContainerGetName(runtime->builder->container),
				runtime->builder->seq,
				szep);
		}
		if (re->flag & BDT_ENDPOINT_IP_VERSION_6
			&& !(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
		{
			{
				// ignore all ipv6 endpoint not wan static
				char szep[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
				BdtEndpointToString(re, szep);
				BLOG_DEBUG("%s %u ignore remote ep %s for ipv6 but not static wan",
					Bdt_TunnelContainerGetName(runtime->builder->container),
					runtime->builder->seq,
					szep);
			}
			continue;
		}
		if (re->flag & BDT_ENDPOINT_PROTOCOL_UDP)
		{
			for (size_t iy = 0; iy < localUdpLen; ++iy)
			{
				const Bdt_UdpInterface* udpInterface = *(localUdpList + iy);
				if (!BdtEndpointIsSameIpVersion(re, Bdt_UdpInterfaceGetLocal(udpInterface)))
				{
					{
						char szepr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
						BdtEndpointToString(re, szepr);
						char szepl[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
						BdtEndpointToString(Bdt_UdpInterfaceGetLocal(udpInterface), szepr);
						BLOG_DEBUG("%s %u ignore remote ep %s local ep %s for ipversion mismatched",
							Bdt_TunnelContainerGetName(runtime->builder->container),
							runtime->builder->seq,
							szepr, 
							szepl);
					}

					continue;
				}
				// simplely do not begin send ack before confirmed
				BuildAction* synAction = (BuildAction*)CreateAckUdpTunnelAction(runtime, timeoutAction, udpInterface, re, 0xffffffff);
				BuildActionRelease(synAction);
			}
		}
		else if (re->flag & BDT_ENDPOINT_PROTOCOL_TCP)
		{
			if (!(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
			{
				{
					// ignore all ipv6 endpoint not wan static
					char szep[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
					BdtEndpointToString(re, szep);
					BLOG_DEBUG("%s %u ignore remote ep %s for tcp but not static wan",
						Bdt_TunnelContainerGetName(runtime->builder->container),
						runtime->builder->seq,
						szep);
				}
				continue;
			}
			for (size_t iy = 0; iy < localEpLen; ++iy)
			{
				const BdtEndpoint* le = localEpList + iy;
				if (!BdtEndpointIsSameIpVersion(re, le))
				{
					{
						char szepr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
						BdtEndpointToString(re, szepr);
						char szepl[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
						BdtEndpointToString(le, szepr);
						BLOG_DEBUG("%s %u ignore remote ep %s local ep %s for ipversion mismatched",
							Bdt_TunnelContainerGetName(runtime->builder->container),
							runtime->builder->seq,
							szepr,
							szepl);
					}
					continue;
				}
				BdtEndpoint localTcp = *le;
				localTcp.flag |= BDT_ENDPOINT_PROTOCOL_TCP;
				BuildAction* connectAction = (BuildAction*)CreateConnectReverseStreamAction(runtime, &localTcp, re);
				BuildActionRelease(connectAction);
			}
		}
	}
	{
		BuildAction* connectAction = (BuildAction*)CreateAcceptPackageConnectionAction(runtime, timeoutAction);
		BuildActionRelease(connectAction);
	}

	Bdt_NetListenerRelease(listener);
	BdtReleasePeerInfo(remoteInfo);

	BuildActionCallbacks waitTimeoutCallbacks;
	waitTimeoutCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION;
	waitTimeoutCallbacks.resolve.function.function = SimpleAcceptConnectionStrategyOnTimeout;
	SimpleAcceptConnectionStrategyAsUserData(strategy, &waitTimeoutCallbacks.resolve.function.userData);

	BuildActionExecute((BuildAction*)waitTimeout, &waitTimeoutCallbacks);

	BuildActionRelease((BuildAction*)waitTimeout);
}


uint32_t Bdt_TunnelBuilderBuildForConnectConnection(
	TunnelBuilder* builder,
	BdtConnection* connection
)
{
	// if builder created, Bdt_TunnelBuilderBuildForConnectConnection will be called even if connection has canceled connecting
	builder->connection = connection;
	BdtAddRefConnection(builder->connection);
	builder->seq = Bdt_ConnectionGetConnectSeq(connection);
	Bdt_BuildTunnelParamsClone(Bdt_ConnectionGetBuildParams(connection), &builder->params);
	builder->buildingFlags |= TUNNEL_BUILDER_FLAG_BUILD_CONNECTION;
	const Bdt_PackageWithRef* firstPackage = Bdt_ConnectionGenSynPackage(connection);
	TunnelBuilderGenSynPackages(builder, &firstPackage, 1);
	Bdt_PackageRelease(firstPackage);
	
	if (builder->params.flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_KEY)
	{
		uint32_t result = BdtHistory_KeyManagerAddKey(
			builder->keyManager,
			builder->params.key,
			BdtTunnel_GetRemotePeerid(builder->container),
			false);
	}
	
	BfxUserData udStrategy;
	
	SimpleConnectConnectionStrategy strategy;
	SimpleConnectConnectionStrategyAsUserData(&strategy, &udStrategy);
	BuildActionRuntime* runtime = BuildActionRuntimeCreate(
		builder,
		NULL,
		SimpleConnectConnectionStrategyOnTunnelActive,
		SimpleConnectConnectionStrategyOnConnectFinish,
		&udStrategy);
	BuildActionRuntimeAddRef(runtime);
	
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&builder->stateLock);
	do
	{
		if (builder->state != TUNNEL_BUILDER_STATE_NONE)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		builder->state = TUNNEL_BUILDER_STATE_BUILDING;
		builder->buildingRuntime = runtime;
	} while (false);
	BfxRwLockWUnlock(&builder->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		BLOG_INFO("%s %u begin build tunnel", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		SimpleConnectConnectionStrategyBegin(&strategy, runtime);
	}
	else
	{
		BLOG_INFO("%s %u ignore build for caceled", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		BuildActionRuntimeRelease(runtime);
	}

	BuildActionRuntimeRelease(runtime);
	return result;
}

static BuildActionRuntime* TunnelBuilderGetBuildingRuntime(TunnelBuilder* builder)
{
	BuildActionRuntime* runtime = NULL;

	BfxRwLockRLock(&builder->stateLock);
	do
	{
		if (builder->buildingRuntime)
		{
			runtime = builder->buildingRuntime;
			BuildActionRuntimeAddRef(runtime);
		}
	} while (false);
	BfxRwLockRUnlock(&builder->stateLock);
	
	return runtime;
}

uint32_t Bdt_TunnelBuilderBuildForAcceptConnection(
	TunnelBuilder* builder,
	BdtConnection* connection
)
{
	builder->connection = connection;
	BdtAddRefConnection(builder->connection);

	builder->seq = Bdt_ConnectionGetConnectSeq(connection);
	builder->buildingFlags |= TUNNEL_BUILDER_FLAG_BUILD_CONNECTION;
	builder->buildingFlags |= TUNNEL_BUILDER_FLAG_PASSIVE;
	TunnelBuilderGenAckPackages(builder);

	BfxUserData udStrategy;
	SimpleConnectConnectionStrategy strategy;
	SimpleAcceptConnectionStrategyAsUserData(&strategy, &udStrategy);
	BuildActionRuntime* runtime = BuildActionRuntimeCreate(
		builder,
		NULL,
		SimpleAcceptConnectionStrategyOnTunnelActive,
		SimpleAcceptConnectionStrategyOnConnectFinish,
		&udStrategy);
	BuildActionRuntimeAddRef(runtime);

	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&builder->stateLock);
	do
	{
		if (builder->state != TUNNEL_BUILDER_STATE_NONE)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		builder->state = TUNNEL_BUILDER_STATE_BUILDING;
		builder->buildingRuntime = runtime;
	} while (false);
	BfxRwLockWUnlock(&builder->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		BLOG_INFO("%s %u begin build tunnel", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		SimpleAcceptConnectionStrategyBegin(&strategy, runtime);
	}
	else
	{
		BLOG_INFO("%s %u ingnore build tunnel for canceled", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		BuildActionRuntimeRelease(runtime);
	}

	BuildActionRuntimeRelease(runtime);

	return result;
}


static uint32_t TunnelBuilderOnTcpFirstSynConnectionPackage(
	Bdt_TunnelBuilder* builder,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_TcpSynConnectionPackage* package
)
{
	if (!TunnelBuilderIsForConnection(builder))
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	if (!TunnelBuilderIsPassive(builder))
	{
		return BFX_RESULT_INVALID_STATE;
	}

	uint32_t result = BFX_RESULT_SUCCESS;

	BuildActionRuntime* runtime = TunnelBuilderGetBuildingRuntime(builder);
	if (runtime)
	{
		AcceptStreamAction* action = CreateAcceptStreamAction(runtime, tcpInterface);
		BuildActionRelease((BuildAction*)action);
		BuildActionRuntimeRelease(runtime);
	}
	else
	{
		result = BFX_RESULT_INVALID_STATE;
	}

	return result;
}


static uint32_t TunnelBuilderOnTcpFirstAckConnectionPackage(
	Bdt_TunnelBuilder* builder,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_TcpAckConnectionPackage* package
)
{
	if (!TunnelBuilderIsForConnection(builder))
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	if (TunnelBuilderIsPassive(builder))
	{
		return BFX_RESULT_INVALID_STATE;
	}

	uint32_t result = BFX_RESULT_SUCCESS;

	BuildActionRuntime* runtime = TunnelBuilderGetBuildingRuntime(builder);
	if (runtime)
	{
		size_t faLen = 0;
		const uint8_t* fa = NULL;
		if (package->payload)
		{
			BfxBufferGetData(package->payload, &faLen);
		}
		Bdt_ConnectionFireFirstAnswer(builder->connection, package->toSessionId, fa, faLen);
		AcceptReverseStreamAction* action = CreateAcceptReverseStreamAction(runtime, tcpInterface);
		BuildActionRelease((BuildAction*)action);
		BuildActionRuntimeRelease(runtime);
	}
	else
	{
		result = BFX_RESULT_INVALID_STATE;
	}

	return result;
}


uint32_t Bdt_TunnelBuilderOnTcpFistPackage(
	Bdt_TunnelBuilder* builder, 
	Bdt_TcpInterface* tcpInterface, 
	const Bdt_Package* package)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	if (package->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
	{
		result = TunnelBuilderOnTcpFirstSynConnectionPackage(builder, tcpInterface, (Bdt_TcpSynConnectionPackage*)package);
	}
	else if (package->cmdtype == BDT_TCP_ACK_CONNECTION_PACKAGE)
	{
		result = TunnelBuilderOnTcpFirstAckConnectionPackage(builder, tcpInterface, (Bdt_TcpAckConnectionPackage*)package);
	}
	else if (package->cmdtype == BDT_SYN_TUNNEL_PACKAGE)
	{

	}
	else if (package->cmdtype == BDT_ACK_TUNNEL_PACKAGE)
	{

	}
	return result;
}

uint32_t Bdt_TunnelBuilderOnSessionData(
	Bdt_TunnelBuilder* builder, 
	const Bdt_SessionDataPackage* sessionData)
{
	BuildActionRuntime* runtime = TunnelBuilderGetBuildingRuntime(builder);
	if (!runtime)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	BuildAction* action = (BuildAction*)BuildActionRuntimeGetBuildingTunnelAction(
		runtime, 
		BuildActionGetPackageConnectionEndpoint(), 
		BuildActionGetPackageConnectionEndpoint());
	if (action)
	{
		if (TunnelBuilderIsPassive(builder))
		{
			AcceptPackageConnectionActionOnSessionData((AcceptPackageConnectionAction*)action, sessionData);
		}
		else
		{
			ConnectPackageConnectionActionOnSessionData((ConnectPackageConnectionAction*)action, sessionData);
		}
		BuildActionRelease(action);
	}

	BuildActionRuntimeRelease(runtime);

	return BFX_RESULT_SUCCESS;
}


static uint32_t TunnelBuilderConfirm(
	Bdt_TunnelBuilder* builder, 
	const Bdt_PackageWithRef* firstResp)
{
	BuildActionRuntime* runtime = TunnelBuilderGetBuildingRuntime(builder);
	if (!runtime)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	BuildingTunnelActionIterator* iter = NULL;
	PassiveBuildAction* action = (PassiveBuildAction*)BuildActionRuntimeEnumBuildingTunnelAction(runtime, &iter);
	while (action)
	{
		PassiveBuildActionConfirm(action, firstResp);
		action = (PassiveBuildAction*)BuildActionRuntimeEnumBuildingTunnelActionNext(iter);
	}
	BuildActionRuntimeEnumBuildingTunnelActionFinish(iter);

	return BFX_RESULT_SUCCESS;
}




typedef struct SimpleConnectTunnelStrategy
{
	void* reserved;
} SimpleConnectTunnelStrategy;

static void SimpleConnectTunnelStrategyOnTunnelActive(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	void* userData)
{
	SimpleConnectTunnelStrategy* strategy = (SimpleConnectTunnelStrategy*)userData;
	// not triggered when build for tunnel
}


static void SimpleConnectTunnelStrategyOnConnectFinish(BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result,
	void* userData)
{
	SimpleConnectTunnelStrategy* strategy = (SimpleConnectTunnelStrategy*)userData;
	// once connection connected, finish runtime
	if (result == BFX_RESULT_SUCCESS)
	{
		// ignore syn udp tunnel actions
		BuildActionRuntimeFinishBuilder(runtime);
	}
}


static void SimpleConnectTunnelStrategyAsUserData(SimpleConnectTunnelStrategy* strategy, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = NULL;
	userData->lpfnReleaseUserData = NULL;
	userData->userData = NULL;
}

static void SimpleConnectTunnelStrategyTryConnectAllTunnel(
	SimpleConnectTunnelStrategy* strategy,
	BuildActionRuntime* runtime,
	const BdtPeerid* snList,
	size_t snListLength
);

static void SimpleConnectTunnelStrategyOnFindPeerResolved(BuildAction* baseAction, void* userData)
{
	SimpleConnectTunnelStrategy* strategy = (SimpleConnectTunnelStrategy*)userData;
	SimpleConnectTunnelStrategyTryConnectAllTunnel(strategy, baseAction->runtime, NULL, 0);
}

static void SimpleConnectTunnelStrategyOnFindPeerRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	// simply finish builder, another strategy may retry with a new strategy and runtime instance
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}

static void SimpleConnectTunnelStrategyOnTimeout(BuildAction* baseAction, void* userData)
{
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}


static void SimpleConnectTunnelStrategyTryConnectAllTunnel(
	SimpleConnectTunnelStrategy* strategy,
	BuildActionRuntime* runtime,
	const BdtPeerid* snList,
	size_t snListLength
)
{
	ListBuildAction* waitTimeout = ListBuildActionAllCreate(runtime, "AllActions");
	// tofix: timeout and tick cycle
	BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(runtime, 3000, 200, "ConnectTimeoutAction");
	ListBuildActionAdd(waitTimeout, (BuildAction*)timeoutAction);
	BuildActionRelease((BuildAction*)timeoutAction);

	bool checkSnCall = true;
	const Bdt_NetListener* listener = Bdt_NetManagerGetListener(runtime->builder->netManager);
	size_t localEpLen = 0;
	const BdtEndpoint* localEpList = Bdt_NetListenerGetBoundAddr(listener, &localEpLen);
	size_t remoteEpLen = 0;
	const BdtPeerInfo* remoteInfo = BuildActionRuntimeGetRemoteInfo(runtime);
	const BdtEndpoint* remoteEpList = BdtPeerInfoListEndpoint(remoteInfo, &remoteEpLen);
	size_t localUdpLen = 0;
	const Bdt_UdpInterface* const* localUdpList = Bdt_NetListenerListUdpInterface(listener, &localUdpLen);

	for (size_t ix = 0; ix < remoteEpLen; ++ix)
	{
		const BdtEndpoint* re = remoteEpList + ix;
		if (re->flag & BDT_ENDPOINT_IP_VERSION_6
			&& !(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
		{
			// ignore all ipv6 endpoint not wan static
			continue;
		}

		if (re->flag & BDT_ENDPOINT_PROTOCOL_UDP)
		{
			for (size_t iy = 0; iy < localUdpLen; ++iy)
			{
				const Bdt_UdpInterface* udpInterface = *(localUdpList + iy);
				if (!BdtEndpointIsSameIpVersion(re, Bdt_UdpInterfaceGetLocal(udpInterface)))
				{
					continue;
				}
				if (re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN)
				{
					BLOG_DEBUG("%s remote has static wan address, ignore sn call", Bdt_TunnelContainerGetName(runtime->builder->container));
					checkSnCall = false;
				}
				BuildAction* synAction = (BuildAction*)CreateSynUdpTunnelAction(runtime, timeoutAction, udpInterface, re);
				BuildActionRelease(synAction);
			}
		}
		else if (re->flag & BDT_ENDPOINT_PROTOCOL_TCP)
		{
			for (size_t iy = 0; iy < localEpLen; ++iy)
			{
				const BdtEndpoint* le = localEpList + iy;
				if (!BdtEndpointIsSameIpVersion(re, le))
				{
					continue;
				}
				if (re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN)
				{
					BLOG_DEBUG("%s remote has static wan address, ignore sn call", Bdt_TunnelContainerGetName(runtime->builder->container));
					checkSnCall = false;
				}
				BdtEndpoint localTcp = *le;
				localTcp.flag |= BDT_ENDPOINT_PROTOCOL_TCP;
				BuildAction* connectAction = (BuildAction*)CreateSynTcpTunnelAction(runtime, timeoutAction, &localTcp, re);
				BuildActionRelease(connectAction);
			}
		}
	}
	
	if (checkSnCall)
	{
		ListBuildAction* callActions = ListBuildActionRaceCreate(runtime, "SnCallActions");
		for (size_t ix = 0; ix < snListLength; ++ix)
		{
			SnCallAction* callAction = CreateSnCallAction(runtime, snList + ix, false);
			ListBuildActionAdd(callActions, (BuildAction*)callAction);
			BuildActionRelease((BuildAction*)callAction);
		}
		ListBuildActionAdd(waitTimeout, (BuildAction*)callActions);
		BuildActionRelease((BuildAction*)callActions);
	}

	Bdt_NetListenerRelease(listener);
	BdtReleasePeerInfo(remoteInfo);

	BuildActionCallbacks waitTimeoutCallbacks;
	waitTimeoutCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION;
	waitTimeoutCallbacks.resolve.function.function = SimpleConnectTunnelStrategyOnTimeout;
	SimpleConnectTunnelStrategyAsUserData(strategy, &waitTimeoutCallbacks.resolve.function.userData);

	BuildActionExecute((BuildAction*)waitTimeout, &waitTimeoutCallbacks);

	BuildActionRelease((BuildAction*)waitTimeout);
}

static void SimpleConnectTunnelStrategyBegin(SimpleConnectTunnelStrategy* strategy, BuildActionRuntime* runtime)
{
	BuildActionRuntimeOnBegin(runtime);

	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetCachedOrStatic(
		runtime->builder->peerFinder,
		BdtTunnel_GetRemotePeerid(runtime->builder->container));


	if (remoteInfo)
	{
		BuildActionRuntimeUpdateRemoteInfo(runtime, remoteInfo);
		size_t snListLength = 0;
		const BdtPeerid* snList = BdtPeerInfoListSn(remoteInfo, &snListLength);
		SimpleConnectTunnelStrategyTryConnectAllTunnel(strategy, runtime, snList, snListLength);
	}
	else
	{
		if (runtime->builder->params.flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_SN)
		{
			ListBuildAction* findPeerAction = ListBuildActionRaceCreate(runtime, "FindPeerActions");
			// tofix: timeout and tick cycle
			BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(runtime, 1000, 0, "FindPeerTimeoutAction");
			for (size_t ix = 0; ix < runtime->builder->params.snListLength; ++ix)
			{
				SnCallAction* callAction = CreateSnCallAction(runtime, runtime->builder->params.snList + ix, true);
				ListBuildActionAdd(findPeerAction, (BuildAction*)callAction);
				BuildActionRelease((BuildAction*)callAction);
			}
			ListBuildActionAdd(findPeerAction, (BuildAction*)timeoutAction);
			BuildActionRelease((BuildAction*)timeoutAction);

			BuildActionCallbacks findPeerCallbacks;
			findPeerCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
			//TODO 可读性 这个.function.function的写法实在不友好
			findPeerCallbacks.resolve.function.function = SimpleConnectTunnelStrategyOnFindPeerResolved;
			SimpleConnectTunnelStrategyAsUserData(strategy, &findPeerCallbacks.resolve.function.userData);
			findPeerCallbacks.reject.function.function = SimpleConnectTunnelStrategyOnFindPeerRejected;
			SimpleConnectTunnelStrategyAsUserData(strategy, &findPeerCallbacks.reject.function.userData);

			BuildActionExecute((BuildAction*)findPeerAction, &findPeerCallbacks);

			BuildActionRelease((BuildAction*)findPeerAction);
		}
		else
		{
			assert(false);
			// todo: use dht action to find peer
		}
	}
}

static uint32_t TunnelBuilderOnTcpTunnelReverseConnected(
	Bdt_TunnelBuilder* builder, 
	TcpTunnel* tunnel)
{
	BuildActionRuntime* runtime = TunnelBuilderGetBuildingRuntime(builder);
	if (runtime)
	{
		BuildActionRuntimeFinishBuildingTunnel(
			runtime,
			Bdt_SocketTunnelGetLocalEndpoint((SocketTunnel*)tunnel),
			Bdt_SocketTunnelGetRemoteEndpoint((SocketTunnel*)tunnel),
			NULL,
			BFX_RESULT_SUCCESS);
		BuildActionRuntimeRelease(runtime);
	}
	return BFX_RESULT_SUCCESS;
}

static uint32_t TunnelBuilderBuildForConnectTunnel(
	TunnelBuilder* builder,
	const Bdt_Package** packages,
	size_t* inoutPackageCount,
	const BdtBuildTunnelParams* params
)
{
	BdtAddRefConnection(builder->connection);
	builder->seq = BdtTunnel_GetNextSeq(builder->container);
	Bdt_BuildTunnelParamsClone(params, &builder->params);
	// todo: if package clone method implemented, may merge packages into syn packages
	TunnelBuilderGenSynPackages(builder, NULL, 0);

	if (builder->params.flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_KEY)
	{
		uint32_t result = BdtHistory_KeyManagerAddKey(
			builder->keyManager,
			builder->params.key,
			BdtTunnel_GetRemotePeerid(builder->container),
			false);
	}
	BLOG_INFO("%s %u begin build tunnel", Bdt_TunnelContainerGetName(builder->container), builder->seq);

	BfxUserData udStrategy;

	SimpleConnectTunnelStrategy strategy;
	SimpleConnectTunnelStrategyAsUserData(&strategy, &udStrategy);
	BuildActionRuntime* runtime = BuildActionRuntimeCreate(
		builder,
		NULL,
		SimpleConnectTunnelStrategyOnTunnelActive,
		SimpleConnectTunnelStrategyOnConnectFinish,
		&udStrategy);

	BuildActionRuntimeAddRef(runtime);

	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&builder->stateLock);
	do
	{
		if (builder->state != TUNNEL_BUILDER_STATE_NONE)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		builder->state = TUNNEL_BUILDER_STATE_BUILDING;
		builder->buildingRuntime = runtime;
	} while (false);
	BfxRwLockWUnlock(&builder->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		BLOG_INFO("%s %u begin build tunnel", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		SimpleConnectTunnelStrategyBegin(&strategy, runtime);
	}
	else
	{
		BLOG_INFO("%s %u ingnore build tunnel for canceled", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		BuildActionRuntimeRelease(runtime);
	}

	BuildActionRuntimeRelease(runtime);

	return result;
}




typedef struct SimpleAcceptTunnelStrategy
{
	void* reserved;
} SimpleAcceptTunnelStrategy;


static void SimpleAcceptTunnelStrategyOnTunnelActive(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	void* userData)
{
	SimpleAcceptTunnelStrategy* strategy = (SimpleAcceptTunnelStrategy*)userData;
	// not triggered when accept tunnel
}


static void SimpleAcceptTunnelStrategyOnConnectFinish(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result,
	void* userData)
{
	SimpleAcceptTunnelStrategy* strategy = (SimpleAcceptTunnelStrategy*)userData;
	// once connection connected, finish runtime
	if (result == BFX_RESULT_SUCCESS)
	{
		BuildActionRuntimeFinishBuilder(runtime);
	}
}

static void SimpleAcceptTunnelStrategyOnTimeout(BuildAction* baseAction, void* userData)
{
	BuildActionRuntimeFinishBuilder(baseAction->runtime);
}

static void SimpleAcceptTunnelStrategyAsUserData(SimpleAcceptTunnelStrategy* strategy, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = NULL;
	userData->lpfnReleaseUserData = NULL;
	userData->userData = NULL;
}

static void SimpleAcceptTunnelStrategyBegin(SimpleAcceptTunnelStrategy* strategy, BuildActionRuntime* runtime)
{
	// cache must exists for remote peer info in SnCalled should has update cache
	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetStatic(
		runtime->builder->peerFinder,
		BdtTunnel_GetRemotePeerid(runtime->builder->container));
	if (!remoteInfo)
	{
		assert(false);
		return;
	}

	BuildActionRuntimeOnBegin(runtime);

	ListBuildAction* waitTimeout = ListBuildActionAllCreate(runtime, "AllActions");
	// tofix: timeout and tick cycle
	BuildTimeoutAction* timeoutAction = CreateBuildTimeoutAction(runtime, 3000, 200, "ConnectTimeoutAction");
	ListBuildActionAdd(waitTimeout, (BuildAction*)timeoutAction);
	BuildActionRelease((BuildAction*)timeoutAction);

	const Bdt_NetListener* listener = Bdt_NetManagerGetListener(runtime->builder->netManager);
	size_t localEpLen = 0;
	const BdtEndpoint* localEpList = Bdt_NetListenerGetBoundAddr(listener, &localEpLen);
	size_t remoteEpLen = 0;
	const BdtEndpoint* remoteEpList = BdtPeerInfoListEndpoint(remoteInfo, &remoteEpLen);
	size_t localUdpLen = 0;
	const Bdt_UdpInterface* const* localUdpList = Bdt_NetListenerListUdpInterface(listener, &localUdpLen);
	for (size_t ix = 0; ix < remoteEpLen; ++ix)
	{
		const BdtEndpoint* re = remoteEpList + ix;
		if (re->flag & BDT_ENDPOINT_IP_VERSION_6
			&& !(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
		{
			// ignore all ipv6 endpoint not wan static
			continue;
		}

		if (re->flag & BDT_ENDPOINT_PROTOCOL_UDP)
		{
			for (size_t iy = 0; iy < localUdpLen; ++iy)
			{
				const Bdt_UdpInterface* udpInterface = *(localUdpList + iy);
				if (!BdtEndpointIsSameIpVersion(re, Bdt_UdpInterfaceGetLocal(udpInterface)))
				{
					continue;
				}
				// should not wait confirm
				BuildAction* synAction = (BuildAction*)CreateAckUdpTunnelAction(runtime, timeoutAction, udpInterface, re, 0);
				BuildActionRelease(synAction);
			}
		}
		else if (re->flag & BDT_ENDPOINT_PROTOCOL_TCP)
		{
			if (!(re->flag & BDT_ENDPOINT_FLAG_STATIC_WAN))
			{
				continue;
			}
			for (size_t iy = 0; iy < localEpLen; ++iy)
			{	
				const BdtEndpoint* le = localEpList + iy;
				if (!BdtEndpointIsSameIpVersion(re, le))
				{
					continue;
				}
				// reverse connect same as connect 
				BuildAction* connectAction = (BuildAction*)CreateSynTcpTunnelAction(runtime, timeoutAction, le, re);
				BuildActionRelease(connectAction);
			}
		}
	}

	Bdt_NetListenerRelease(listener);
	BdtReleasePeerInfo(remoteInfo);

	BuildActionCallbacks waitTimeoutCallbacks;
	waitTimeoutCallbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION;
	waitTimeoutCallbacks.resolve.function.function = SimpleAcceptTunnelStrategyOnTimeout;
	SimpleAcceptTunnelStrategyAsUserData(strategy, &waitTimeoutCallbacks.resolve.function.userData);

	BuildActionExecute((BuildAction*)waitTimeout, &waitTimeoutCallbacks);

	BuildActionRelease((BuildAction*)waitTimeout);
}



static uint32_t TunnelBuilderBuildForAcceptTunnel(
	TunnelBuilder* builder,
	const uint8_t key[BDT_AES_KEY_LENGTH],
	uint32_t seq
)
{
	builder->seq = seq;
	builder->buildingFlags |= TUNNEL_BUILDER_FLAG_PASSIVE;
	TunnelBuilderGenAckPackages(builder);

	BfxUserData udStrategy;
	SimpleAcceptTunnelStrategy strategy;
	SimpleAcceptTunnelStrategyAsUserData(&strategy, &udStrategy);
	BuildActionRuntime* runtime = BuildActionRuntimeCreate(
		builder,
		NULL,
		SimpleAcceptTunnelStrategyOnTunnelActive,
		SimpleAcceptTunnelStrategyOnConnectFinish,
		&udStrategy);
	BuildActionRuntimeAddRef(runtime);

	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&builder->stateLock);
	do
	{
		if (builder->state != TUNNEL_BUILDER_STATE_NONE)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		builder->state = TUNNEL_BUILDER_STATE_BUILDING;
		builder->buildingRuntime = runtime;
	} while (false);
	BfxRwLockWUnlock(&builder->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		BLOG_INFO("%s %u begin build tunnel", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		SimpleAcceptTunnelStrategyBegin(&strategy, runtime);
	}
	else
	{
		BLOG_INFO("%s %u ingnore build tunnel for canceled", Bdt_TunnelContainerGetName(builder->container), builder->seq);
		BuildActionRuntimeRelease(runtime);
	}

	BuildActionRuntimeRelease(runtime);

	return result;
}

static uint32_t TunnelBuilderFinishWithRuntime(
	TunnelBuilder* builder, 
	BuildActionRuntime* fromRuntime
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BuildActionRuntime* runtime = NULL;
	BfxRwLockWLock(&builder->stateLock);
	do
	{
		if (builder->state == TUNNEL_BUILDER_STATE_FINISHED)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		if (fromRuntime != NULL 
			&& builder->buildingRuntime != fromRuntime)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		builder->state = TUNNEL_BUILDER_STATE_FINISHED;
		runtime = builder->buildingRuntime;
		builder->buildingRuntime = NULL;
	} while (false);
	BfxRwLockWUnlock(&builder->stateLock);

	if (runtime)
	{
		BuildActionRuntimeOnFinish(runtime);
		BuildActionRuntimeRelease(runtime);
	}
	else
	{
		if (fromRuntime)
		{
			BuildActionRuntimeOnFinish(fromRuntime);
		}
	}

	return result;
}

#endif //__BDT_TUNNEL_BUILDER_IMP_INL__
