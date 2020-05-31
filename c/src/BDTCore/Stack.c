#include "./Stack.h"
#include "./BdtSystemFramework.h"
#include "BDTCore/Connection/Connection.h"

struct BdtStack
{
	const BdtPeerInfo* localPeerConst;

	BdtSystemFramework* framework;
	BdtStorage* storage;
	StackEventCallback callback;
	BfxUserData userData;

	Bdt_StackTls tls;
	BdtStackConfig stackConfig;
	Bdt_EventCenter eventCenter;
	BdtHistory_PeerUpdater* peerHistoryUpdater;
	BdtHistory_KeyManager* keyManager;
	Bdt_NetManager* netManager;
	Bdt_Uint32IdCreator idCreator;
	BdtPeerFinder peerFinder;
	Bdt_TunnelContainerManager* tunnelManager;
	Bdt_ConnectionManager* connectionManager;
	BdtSuperNodeClient* snClient;
};

static void BdtStackLoadHistoryFromStorage(
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount,
	void* userData
);

static inline void FireCreate(BdtStack* stack)
{
	stack->callback(stack, BDT_STACK_EVENT_CREATE, NULL, stack->userData.userData);
}

static inline void FireError(BdtStack* stack, uint32_t error)
{
	stack->callback(stack, BDT_STACK_EVENT_ERROR, &error, stack->userData.userData);
}


static void BdtStackInitConfig(BdtStack* stack)
{

	static const BdtTunnelConfig tunnelConfig = {
		.history = {
			.respCacheSize = 100, 
			.tcpLogCacheSize = 100
		},
		.builder = {
			.holePunchCycle = 100,
			.holePunchTimeout = 3000,
			.findPeerTimeout = 2000
		},
		.udpTunnel = {
			.pingInterval = 60000
		},
		.tcpTunnel = {
			.connectTimeout = 3000,
			.sendBufferSize = 1024*1024*1024
		}
	};

	stack->stackConfig.net.tcp.firstPackageWaitTime = 60000;

	static const BdtSuperNodeClientConfig bdtSuperNodeClientDefaultConfig = {
		.onlineSnLimit = 3,
		.snRefreshInterval = 600000,
		.pingIntervalInit = 500,
		.pingInterval = 25000,
		.offlineTimeout = 300000,
		// call
		.callSnLimit = 3,
		.callInterval = 200000,
		.callTimeout = 3000
	};
	BdtStackConfig* config = &stack->stackConfig;
	memcpy(&config->snClient, &bdtSuperNodeClientDefaultConfig, sizeof(bdtSuperNodeClientDefaultConfig));

	config->acceptConnection.connection.connectTimeout 
		= config->connection.connectTimeout = 20000;
	static const BdtPackageConnectionConfig pkgConConfig = {
		.resendInterval = 200,
		.recvTimeout = 200,
		.msl = 200,
		.mtu = 1308,
		.mss = 1308,
		.maxRecvBuffer = 2 * 1024 * 1024,
		.maxSendBuffer = 1024 * 1024,
		.halfOpen = true,
	};

	static const BdtStreamConnectionConfig streamConConfig =
	{
        // maxNagleDelay和minRecordSize用于处理小包延迟和合并；
        // 应用场景中存在大量高频小包，并且特别在意流量时启用（加密可能带来一定的流量损失），否则可能带来一定的发送延迟
		.maxNagleDelay = 0,
		.minRecordSize = 1,

		.maxRecordSize = 2048,
        // recvTimeout的设定可能带来一定的接收延迟，不在意延迟的应用场景可以通过设定适当的值减少recv操作的回调次数
		.recvTimeout = 0,
	};

	static const BdtPeerHistoryConfig historyConfig = {
		.activeTime = 3600 * 24 * 10,
		.peerCapacity = 1000
	};

	static const BdtAesKeyConfig aesKeyConfig = {
		.activeTime = 3600 * 24 * 10
	};

	memcpy(&(stack->stackConfig.tunnel), &tunnelConfig, sizeof(BdtTunnelConfig));
	memcpy(&(stack->stackConfig.connection.package), &pkgConConfig, sizeof(BdtPackageConnectionConfig));
	memcpy(&(stack->stackConfig.connection.stream), &streamConConfig, sizeof(BdtStreamConnectionConfig));
	memcpy(&(stack->stackConfig.acceptConnection.connection.package), &pkgConConfig, sizeof(BdtPackageConnectionConfig));
	memcpy(&(stack->stackConfig.acceptConnection.connection.stream), &streamConConfig, sizeof(BdtStreamConnectionConfig));
	memcpy(&(stack->stackConfig.peerHistoryConfig), &historyConfig, sizeof(BdtPeerHistoryConfig));
	memcpy(&(stack->stackConfig.aesKeyConfig), &aesKeyConfig, sizeof(BdtAesKeyConfig));
}

BDT_API(uint32_t) BdtCreateStack(
	BdtSystemFramework* framework,
	const BdtPeerInfo* localPeer,
	const BdtPeerInfo** initialPeers,
	size_t initialPeerCount,
	BdtStorage* storage,
	StackEventCallback callback,
	const BfxUserData* userData,
	BdtStack** outStack
)
{
	BdtStack* newStack = BFX_MALLOC_OBJ(BdtStack);
	memset(newStack, 0, sizeof(BdtStack));

	BdtStackInitConfig(newStack);

	BdtAddRefPeerInfo(localPeer);
	newStack->localPeerConst = localPeer;

	newStack->framework = framework;
	newStack->storage = storage;
	framework->stack = newStack;
	newStack->callback = callback;
	newStack->userData = *userData;
	if (userData->lpfnAddRefUserData)
	{
		userData->lpfnAddRefUserData(userData->userData);
	}
	Bdt_StackTlsInit(&(newStack->tls));
	Bdt_EventCenterInit(&(newStack->eventCenter), newStack);

	newStack->peerHistoryUpdater = BdtHistory_PeerUpdaterCreate(
		localPeer,
		false,
		&(newStack->stackConfig.peerHistoryConfig));
	newStack->keyManager = BdtHistory_KeyManagerCreate(&(newStack->stackConfig.aesKeyConfig));

	{
		// 加载历史运行数据
		int64_t nowMS = BfxTimeGetNow(false) / 1000;
		BfxUserData userData = { .userData = newStack,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
		BdtHistory_StorageLoadAll(
			storage,
			nowMS - (int64_t)newStack->stackConfig.peerHistoryConfig.activeTime * 1000,
			BdtStackLoadHistoryFromStorage,
			&userData
		);
	}

	/*to fix*/
	Bdt_Uint32IdCreatorInit(&(newStack->idCreator));

	size_t endpointLength = 0;
	const BdtEndpoint* localEndpoint = BdtPeerInfoListEndpoint(localPeer, &endpointLength);
	uint32_t* listenResults = BFX_MALLOC_ARRAY(uint32_t, endpointLength);
	newStack->netManager = Bdt_NetManagerCreate(framework, &newStack->stackConfig.net);
	Bdt_NetManagerStart(newStack->netManager, localEndpoint, endpointLength, listenResults); 
	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST |
        BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
	BdtPeerInfoSetConstInfo(builder, BdtPeerInfoGetConstInfo(localPeer));
	for (size_t ix = 0; ix < endpointLength; ++ix)
	{
		if (!listenResults[ix])
		{
			BdtPeerInfoAddEndpoint(builder, localEndpoint + ix);
		}
	}

	//todo:sign it
	const BdtPeerInfo* updatePeer = BdtPeerInfoBuilderFinish(builder, BdtPeerInfoGetSecret(localPeer));

	BfxFree(listenResults);

	BdtPeerFinderInit(&(newStack->peerFinder), newStack->framework, &(newStack->eventCenter), updatePeer);
	BdtReleasePeerInfo(updatePeer);
	for (size_t ix = 0; ix < initialPeerCount; ++ix)
	{
		BdtPeerFinderAddStatic(BdtStackGetPeerFinder(newStack), initialPeers[ix]);
	}
	size_t snCount = 0;
	const BdtPeerInfo** snList = BdtPeerInfoListSnInfo(localPeer, &snCount);
	for (size_t ix = 0; ix < snCount; ++ix)
	{
		BdtPeerFinderAddStatic(BdtStackGetPeerFinder(newStack), snList[ix]);
	}

	//todo: wait sn online event
	BdtSnClient_Create(newStack, true, &newStack->snClient);
    size_t localSnCount = 0;
    const BdtPeerInfo** localSnInfo = BdtPeerInfoListSnInfo(localPeer, &localSnCount);
    if (localSnCount > 0)
    {
        BdtSnClient_UpdateSNList(newStack->snClient, localSnInfo, (int)localSnCount);
    }
    
    const BdtPeerid* localSnPeerid = BdtPeerInfoListSn(localPeer, &localSnCount);
    if (localSnCount > 0)
    {
        localSnInfo = BFX_MALLOC_ARRAY(const BdtPeerInfo*, localSnCount);
        size_t findCount = 0;
        for (size_t si = 0; si < localSnCount; si++)
        {
            localSnInfo[findCount] = BdtPeerFinderGetCachedOrStatic(BdtStackGetPeerFinder(newStack), localSnPeerid + si);
            if (localSnInfo[findCount] != NULL)
            {
                findCount++;
            }
        }
        BdtSnClient_UpdateSNList(newStack->snClient, localSnInfo, (int)findCount);
        for (size_t sfi = 0; sfi < findCount; sfi++)
        {
            BdtReleasePeerInfo(localSnInfo[sfi]);
        }
        BFX_FREE((void*)localSnInfo);
    }

	BdtSnClient_Start(newStack->snClient);

	newStack->tunnelManager = Bdt_TunnelContainerManagerCreate(newStack);
	newStack->connectionManager = Bdt_ConnectionManagerCreate(
		framework, 
		&newStack->tls, 
		newStack->netManager,
		&newStack->peerFinder,
		newStack->tunnelManager, 
		&newStack->idCreator, 
		&newStack->stackConfig.connection, 
		&newStack->stackConfig.acceptConnection);
	
	*outStack = newStack;
	return BFX_RESULT_SUCCESS;
}

BDT_API(uint32_t) BdtCloseStack(BdtStack* stack) 
{
	Bdt_TunnelContainerManagerDestroy(stack->tunnelManager);
	BdtSnClient_Stop(stack->snClient);
	BdtPeerFinderUninit(&(stack->peerFinder));
	Bdt_Uint32IdCreatorUninit(&(stack->idCreator));
	BdtHistory_PeerUpdaterDestroy(stack->peerHistoryUpdater);
	BdtHistory_KeyManagerDestroy(stack->keyManager);
	BdtHistory_StorageDestroy(stack->storage);
	Bdt_EventCenterUninit(&(stack->eventCenter));
	Bdt_StackTlsUninit(&(stack->tls));

	BdtSnClient_Destroy(stack->snClient);
	BdtReleasePeerInfo(stack->localPeerConst);

	if (stack->userData.lpfnReleaseUserData)
	{
		stack->userData.lpfnReleaseUserData(stack->userData.userData);
	}

	BFX_FREE(stack);
	return BFX_RESULT_SUCCESS;
}


BDT_API(uint32_t) BdtCreateConnection(
	BdtStack* stack,
	const BdtPeerid* remotePeerid,
	bool encrypted,
	BdtConnection** outConnection)
{
	BdtConnection* connection = Bdt_AddActiveConnection(
		stack->connectionManager,
		remotePeerid);
	*outConnection = connection;
	return BFX_RESULT_SUCCESS;
}

BDT_API(void) BdtAddStaticPeerInfo(
    BdtStack* stack,
    const BdtPeerInfo* peerInfo)
{
    BdtPeerFinderAddStatic(BdtStackGetPeerFinder(stack), peerInfo);
}

const BdtPeerid* BdtStackGetLocalPeerid(BdtStack* stack)
{
	return BdtPeerInfoGetPeerid(stack->localPeerConst);
}

const BdtPeerConstInfo* BdtStackGetLocalPeerConstInfo(BdtStack* stack)
{
	return BdtPeerInfoGetConstInfo(stack->localPeerConst);
}

const BdtPeerInfo* BdtStackGetConstLocalPeer(BdtStack* stack)
{
    return stack->localPeerConst;
}

const BdtPeerSecret* BdtStackGetSecret(BdtStack* stack)
{
	return BdtPeerInfoGetSecret(stack->localPeerConst);
}

BdtSystemFramework* BdtStackGetFramework(BdtStack* stack)
{
	return stack->framework;
}

const BdtStackConfig* BdtStackGetConfig(BdtStack* stack)
{
	return &stack->stackConfig;
}
Bdt_EventCenter* BdtStackGetEventCenter(BdtStack* stack)
{
	return &stack->eventCenter;
}

Bdt_Uint32IdCreator* BdtStackGetIdCreator(BdtStack* stack)
{
	return &stack->idCreator;
}

BdtPeerFinder* BdtStackGetPeerFinder(BdtStack* stack)
{
	return &stack->peerFinder;
}

Bdt_TunnelContainerManager* BdtStackGetTunnelManager(BdtStack* stack)
{
	return stack->tunnelManager;
}

Bdt_ConnectionManager* BdtStackGetConnectionManager(BdtStack* stack)
{
	return stack->connectionManager;
}

BdtSuperNodeClient* BdtStackGetSnClient(BdtStack* stack)
{
	return stack->snClient;
}

BdtHistory_KeyManager* BdtStackGetKeyManager(BdtStack* stack)
{
	return stack->keyManager;
}

Bdt_NetManager* BdtStackGetNetManager(BdtStack* stack)
{
	return stack->netManager;
}

Bdt_StackTls* BdtStackGetTls(BdtStack* stack)
{
	return &stack->tls;
}

BdtStorage* BdtStackGetStorage(BdtStack* stack)
{
	return stack->storage;
}

BdtHistory_PeerUpdater* BdtStackGetHistoryPeerUpdater(BdtStack* stack)
{
	return stack->peerHistoryUpdater;
}

static void BdtStackLoadHistoryFromStorage(
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount,
	void* userData
)
{
	// <TODO>需要防御加载完成前退出
	BdtStack* stack = (BdtStack*)userData;

	BdtHistory_PeerUpdaterOnLoadDone(stack->peerHistoryUpdater,
		stack->storage,
		errorCode,
		historys,
		historyCount);

	BdtHistory_KeyManagerOnLoadDone(stack->keyManager,
		stack->storage,
		errorCode,
		aesKeys,
		keyCount);
}

