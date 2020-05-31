#ifndef __BDT_SUPERNODE_CLIENT_MODULE_IMPL__
#error "should only include in Module.c of SuperNodeClient module"
#endif //__BDT_SUPERNODE_CLIENT_MODULE_IMPL__

#include <assert.h>
#include <SGLib/SGLib.h>
#include <BuckyBase/Coll/vector.h>
#include "../BdtCore.h"
#include "../Stack.h"
#include "../BdtSystemFramework.h"
#include "./Client.h"
#include "./Client.inl"
#include "./Ping.inl"
#include "../Protocol/Package.h"
#include "../Protocol/PackageEncoder.h"
#include "../Interface/NetManager.h"

typedef enum PingClientStatus
{
	PING_CLIENT_STATUS_INIT = 0,
	PING_CLIENT_STATUS_CONNECTING = 1,
	PING_CLIENT_STATUS_ONLINE = 2,
	PING_CLIENT_STATUS_OFFLINE = 3,
} PingClientStatus;

typedef struct PingSession
{
    const Bdt_UdpInterface* handle;
    BdtEndpoint outEndpoint; // 出口地址
    BdtEndpoint remoteEndpoint;
    int64_t lastPingTime;
    int64_t lastRespTime;
} PingSession;

typedef struct PingClient
{
	BdtStack* stack;

	const BdtPeerInfo* snPeerInfo;
	int64_t lastPingTime;
	int64_t lastRespTime;

    PingSession* sessions;
    size_t sessionCount;

	volatile PingClientStatus status;
	volatile int32_t refCount;

	bool encrypto;
	uint8_t aesKey[BDT_AES_KEY_LENGTH];
} PingClient;

static PingClient* PingClientCreate(BdtStack* stack,
	const BdtPeerInfo* snPeerInfo,
	bool encrypto);
static int32_t PingClientAddRef(PingClient* pingClient);
static int32_t PingClientRelease(PingClient* pingClient);
static void PingClientStart(PingClient* pingClient);
static void PingClientUpdateStatus(PingClient* pingClient);
static void PingClientStop(PingClient* pingClient);
static bool PingClientOnResponce(
    PingClient* pingClient,
	Bdt_SnPingRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
    const BdtEndpoint** newOutEndpoint
);
static void PingClientSendPing(PingClient* pingClient);
static void PingClientDestroy(PingClient* pingClient);

typedef struct PingManager
{
	BdtSuperNodeClient* owner;
	bool encrypto;

	BfxThreadLock clientsLock;
	int maxPingClientCount;
	PingClient* pingClients[16];
} PingManager;

static void PingManagerInit(
	BdtStack* stack,
	PingManager* manager,
	bool encrypto,
	BdtSuperNodeClient* owner)
{
	const BdtSuperNodeClientConfig* config = &(BdtStackGetConfig(stack)->snClient);

	BLOG_INFO("manager:0x%p, owner:0x%p", manager, owner);
	BLOG_CHECK(config->onlineSnLimit * 2 <= (int)BFX_ARRAYSIZE(manager->pingClients));

	memset(manager, 0, sizeof(PingManager));
	manager->owner = owner;
	manager->encrypto = encrypto;
	BfxThreadLockInit(&manager->clientsLock);
	manager->maxPingClientCount = min((int)BFX_ARRAYSIZE(manager->pingClients), config->onlineSnLimit * 2);
	memset(manager->pingClients, 0, sizeof(manager->pingClients));
}

static void PingManagerUninit(PingManager* mgr)
{
	BLOG_INFO("manager:0x%p.", mgr);
	BLOG_CHECK(mgr != NULL);

	for (int i = 0; i < mgr->maxPingClientCount; i++)
	{
		PingClient* pingClient = mgr->pingClients[i];
		if (pingClient != NULL)
		{
			mgr->pingClients[i] = NULL;
			PingClientStop(pingClient);
			PingClientRelease(pingClient);
		}
	}

	BfxThreadLockDestroy(&mgr->clientsLock);
}

static PingManager* PingManagerCreate(
	BdtStack* stack,
	bool encrypto,
	BdtSuperNodeClient* owner)
{
	PingManager* mgr = BFX_MALLOC_OBJ(PingManager);
	PingManagerInit(stack, mgr, encrypto, owner);
	return mgr;
}

static void PingManagerDestroy(PingManager* manager)
{
	PingManagerUninit(manager);
	BfxFree(manager);
}

static void PingManagerUpdateLocalPeerInfo(PingManager* mgr, PingClient* pingClients[], int snCount)
{
    BdtStack* stack = mgr->owner->stack;
    const BdtPeerInfo* constLocalPeerInfo = BdtStackGetConstLocalPeer(stack);

    BdtPeerInfoBuilder* peerInfoBuilder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST |
        BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
    BdtPeerInfoClone(peerInfoBuilder, constLocalPeerInfo);

    for (int i = 0; i < snCount; i++)
    {
        if (pingClients[i])
        {
            for (size_t si = 0; si < pingClients[i]->sessionCount; si++)
            {
                PingSession* session = pingClients[i]->sessions + si;
                if (session->lastRespTime > 0)
                {
                    BdtEndpoint* outEndpoint = &session->outEndpoint;
                    BdtPeerInfoAddEndpoint(peerInfoBuilder, outEndpoint);
                }
            }
        }
    }
    const BdtPeerInfo* newLocalPeer = BdtPeerInfoBuilderFinish(
        peerInfoBuilder,
        BdtStackGetSecret(stack)
    );

    if (newLocalPeer != NULL)
    {
        BdtPeerFinderUpdateLocalPeer(BdtStackGetPeerFinder(stack), newLocalPeer);
        BdtReleasePeerInfo(newLocalPeer);
    }
}

static uint32_t PingManagerPingResponsed(PingManager* mgr,
	Bdt_SnPingRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
	bool* outHandled)
{
	PingClient* targetPingClient = NULL;
	const BdtPeerid* snPeerid = BdtPeerInfoGetPeerid(package->peerInfo);

#ifndef BLOG_DISABLE
	char fromEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BLOG_HEX(snPeerid->pid, 15, "manager:0x%p, cmdtype:%d, seq:%d, fromEp:%s",
		mgr, package->cmdtype, package->seq, (BdtEndpointToString(fromEndpoint, fromEpStr), fromEpStr));
#endif

    PingClient* pingClients[16] = {NULL};
    int snCount = 0;

	BfxThreadLockLock(&mgr->clientsLock);

    snCount = mgr->maxPingClientCount;
	for (int i = 0; i < mgr->maxPingClientCount; i++)
	{
        pingClients[i] = mgr->pingClients[i];
        if (pingClients[i] != NULL)
        {
            PingClientAddRef(pingClients[i]);
        }
	}

	BfxThreadLockUnlock(&mgr->clientsLock);

    for (int i = 0; i < snCount; i++)
    {
        if (pingClients[i] &&
            memcmp(BdtPeerInfoGetPeerid(pingClients[i]->snPeerInfo), snPeerid, sizeof(BdtPeerid)) == 0 &&
            (pingClients[i]->status == PING_CLIENT_STATUS_ONLINE || pingClients[i]->status == PING_CLIENT_STATUS_CONNECTING))
        {
            targetPingClient = pingClients[i];
            break;
        }
    }

	if (targetPingClient != NULL)
	{
        BdtEndpoint* newOutEndpoint;
		bool isNewEndpoint = PingClientOnResponce(targetPingClient, package, fromEndpoint, fromInterface, &newOutEndpoint);
        if (isNewEndpoint) {
            // 更新localPeerInfo，并签名
            BdtStack* stack = targetPingClient->stack;
            const BdtPeerInfo* localPeer = BdtPeerFinderGetLocalPeer(BdtStackGetPeerFinder(stack));

            const BdtEndpoint* foundEp = BdtPeerInfoFindEndpoint(localPeer, newOutEndpoint);
            BdtReleasePeerInfo(localPeer);
            if (foundEp == NULL)
            {
                PingManagerUpdateLocalPeerInfo(mgr, pingClients, snCount);
            }

            for (int i = 0; i < snCount; i++)
            {
                if (pingClients[i] &&
                    (pingClients[i]->status == PING_CLIENT_STATUS_ONLINE || pingClients[i]->status == PING_CLIENT_STATUS_CONNECTING))
                {
                    PingClientSendPing(pingClients[i]);
                }
            }
        }

		BdtHistory_PeerUpdater* historyUpdater = BdtStackGetHistoryPeerUpdater(targetPingClient->stack);
        BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
			historyUpdater,
			BdtPeerInfoGetPeerid(targetPingClient->snPeerInfo),
			targetPingClient->snPeerInfo,
			fromEndpoint,
			Bdt_UdpInterfaceGetLocal(fromInterface),
			BDT_HISTORY_PEER_CONNECT_TYPE_UDP,
			0);
		BdtHistory_PeerUpdaterOnSuperNodePingResponse(historyUpdater,
			targetPingClient->snPeerInfo);
	}

    for (int i = 0; i < snCount; i++)
    {
        if (pingClients[i])
        {
            PingClientRelease(pingClients[i]);
        }
    }

	BLOG_HEX_IF(targetPingClient == NULL, snPeerid->pid, 15, "manager:0x%p, cmdtype:%d, seq:%d, fromEp:%s ping client has destroyed.",
		mgr, package->cmdtype, package->seq, fromEpStr);

	return BFX_RESULT_SUCCESS;
}

static void PingManagerOnTimer(PingManager* mgr)
{
	// 1.检查所有在线SN是否掉线
	// 2.搜索更新SN列表

	PingClient* offlineClients[16];
	int offlineCount = 0;
	PingClient* activeClients[16];
	int activeCount = 0;

	BfxThreadLockLock(&mgr->clientsLock);

	for (int i = 0; i < mgr->maxPingClientCount; i++)
	{
		PingClient* pingClient = mgr->pingClients[i];
		PingClientStatus status = PING_CLIENT_STATUS_INIT;
		if (pingClient != NULL)
		{
			status = pingClient->status;

			if (status == PING_CLIENT_STATUS_OFFLINE)
			{
				mgr->pingClients[i] = NULL;
			}
		}

		if (status == PING_CLIENT_STATUS_OFFLINE)
		{
			offlineClients[offlineCount++] = pingClient;
			if (offlineCount >= BFX_ARRAYSIZE(offlineClients))
			{
				break;
			}
		}
		else if (status != PING_CLIENT_STATUS_INIT)
		{
			PingClientAddRef(pingClient);
			activeClients[activeCount++] = pingClient;
			if (activeCount >= BFX_ARRAYSIZE(activeClients))
			{
				break;
			}
		}
	}

	BfxThreadLockUnlock(&mgr->clientsLock);

	for (int i = 0; i < offlineCount; i++)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(offlineClients[i]->snPeerInfo)->pid, 15,
			"ping client(0x%p) release for offline.", offlineClients[i]);

		PingClientRelease(offlineClients[i]);
	}

	for (int i = 0; i < activeCount; i++)
	{
		PingClientUpdateStatus(activeClients[i]);
		PingClientRelease(activeClients[i]);
	}
}

static PingClient* PingManagerFindPingClientByPeeridWithoutLock(PingManager* mgr, const BdtPeerInfo* peerInfo)
{
	for (int j = 0; j < mgr->maxPingClientCount; j++)
	{
		PingClient* pingClient = mgr->pingClients[j];
		if (pingClient != NULL)
		{
			if (memcmp(BdtPeerInfoGetPeerid(pingClient->snPeerInfo), BdtPeerInfoGetPeerid(peerInfo), sizeof(BdtPeerid)) == 0)
			{
				return pingClient;
			}
		}
	}
	return NULL;
}

static void PingManagerRemoveClientsExclude(PingManager* mgr, const BdtPeerInfo** peerInfos, int count)
{
	PingClient* removeClients[16];
	int removeCount = 0;

	BfxThreadLockLock(&mgr->clientsLock);
	{
		for (int i = 0; i < mgr->maxPingClientCount; i++)
		{
			PingClient* pingClient = mgr->pingClients[i];
			BOOL found = FALSE;
			if (pingClient != NULL)
			{
				for (int j = 0; j < count; j++)
				{
					const BdtPeerInfo* peerInfo = peerInfos[j];
					if (memcmp(BdtPeerInfoGetPeerid(pingClient->snPeerInfo), BdtPeerInfoGetPeerid(peerInfo), sizeof(BdtPeerid)) == 0)
					{
						found = TRUE;
						break;
					}
				}

				if (!found)
				{
					mgr->pingClients[i] = NULL;
					removeClients[removeCount++] = pingClient;
					if (removeCount == BFX_ARRAYSIZE(removeClients))
					{
						break;
					}
				}
			}
		}
	}

	BfxThreadLockUnlock(&mgr->clientsLock);

	for (int i = 0; i < removeCount; i++)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(removeClients[i]->snPeerInfo)->pid, 15,
			"sn(0x%p) removed.", removeClients[i]);
		PingClientStop(removeClients[i]);
		PingClientRelease(removeClients[i]);
	}

	if (removeCount == BFX_ARRAYSIZE(removeClients))
	{
		PingManagerRemoveClientsExclude(mgr, peerInfos, count);
	}
}

uint32_t BdtSnClient_UpdateSNList(BdtSuperNodeClient* snClient, const BdtPeerInfo* snPeerInfos[], int count)
{
	PingManager* mgr = snClient->pingMgr;

	count = min(count, 8);
	PingClient* newClients[8];
	memset(newClients, 0, sizeof(newClients));

	PingClient* dupClients[8];
	memset(dupClients, 0, sizeof(dupClients));

	for (int i = 0; i < count; i++)
	{
		newClients[i] = PingClientCreate(snClient->stack,
			snPeerInfos[i],
			mgr->encrypto);
	}

	BfxThreadLockLock(&mgr->clientsLock);

	int nextEmptyPos = 0;
	for (int i = 0; i < count; i++)
	{
		if (PingManagerFindPingClientByPeeridWithoutLock(mgr, snPeerInfos[i]) != NULL)
		{
			dupClients[i] = newClients[i];
			newClients[i] = NULL;
			continue;
		}

		for (; nextEmptyPos < mgr->maxPingClientCount; nextEmptyPos++)
		{
			PingClient* pingClient = mgr->pingClients[nextEmptyPos];
			if (pingClient == NULL)
			{
				break;
			}
		}

		mgr->pingClients[nextEmptyPos] = newClients[i];
		PingClientAddRef(newClients[i]);
		nextEmptyPos++;
	}

	BfxThreadLockUnlock(&mgr->clientsLock);

	for (int i = 0; i < count; i++)
	{
		if (newClients[i] != NULL)
		{
			BLOG_HEX(BdtPeerInfoGetPeerid(newClients[i]->snPeerInfo)->pid, 15,
				"new sn(0x%p) added to ping, and start.", newClients[i]);

			PingClientStart(newClients[i]);
			PingClientRelease(newClients[i]);
		}
	}

	for (int i = 0; i < count; i++)
	{
		if (dupClients[i] != NULL)
		{
			BLOG_HEX(BdtPeerInfoGetPeerid(dupClients[i]->snPeerInfo)->pid, 15,
				"dup sn(0x%p) added to ping and destroy.", dupClients[i]);

			PingClientRelease(dupClients[i]);
		}
	}

	PingManagerRemoveClientsExclude(mgr, snPeerInfos, count);

	return BFX_RESULT_SUCCESS;
}

static PingClient* PingClientCreate(BdtStack* stack,
	const BdtPeerInfo* snPeerInfo,
	bool encrypto)
{
	PingClient* pingClient = (PingClient*)BFX_MALLOC_OBJ(PingClient);
	memset(pingClient, 0, sizeof(*pingClient));
	pingClient->stack = stack;
	pingClient->snPeerInfo = snPeerInfo;
	pingClient->status = PING_CLIENT_STATUS_INIT;
	pingClient->refCount = 1;
	BdtAddRefPeerInfo(snPeerInfo);

    const Bdt_NetListener* netListener = Bdt_NetManagerGetListener(BdtStackGetNetManager(stack));
    size_t interfaceCount = 0;
    const Bdt_UdpInterface* const* interfaceList = Bdt_NetListenerListUdpInterface(netListener, &interfaceCount);
    if (!interfaceCount)
    {
        BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
            "sn(0x%p) no interface found, interfaceCount: %d.",
            pingClient, interfaceCount);
    }
    pingClient->sessions = BFX_MALLOC_ARRAY(PingSession, interfaceCount);
    pingClient->sessionCount = interfaceCount;
    for (size_t i = 0; i < interfaceCount; i++)
    {
        BLOG_CHECK(interfaceList[i] != NULL);
        PingSession* session = pingClient->sessions + i;
        session->outEndpoint = *Bdt_UdpInterfaceGetLocal(interfaceList[i]);
        session->handle = interfaceList[i];
        session->lastPingTime = session->lastRespTime = 0;
        Bdt_UdpInterfaceAddRef(session->handle);
    }

    Bdt_NetListenerRelease(netListener);

	if (encrypto)
	{
		pingClient->encrypto = true;

		bool isNew = true;
		bool isConfirmed = false;
		BdtHistory_KeyManagerGetKeyByRemote(BdtStackGetKeyManager(stack),
			BdtPeerInfoGetPeerid(snPeerInfo),
			pingClient->aesKey, &isNew, &isConfirmed, true);
	}

	BLOG_HEX(BdtPeerInfoGetPeerid(snPeerInfo)->pid, 15,
		"sn(0x%p) created, encrypto:%d.", pingClient, (int)encrypto);

	return pingClient;
}

static int32_t PingClientAddRef(PingClient* pingClient)
{
	return BfxAtomicInc32(&pingClient->refCount);
}

static int32_t PingClientRelease(PingClient* pingClient)
{
	int32_t ref = BfxAtomicDec32(&pingClient->refCount);
	if (ref == 0)
	{
		PingClientDestroy(pingClient);
	}
	return ref;
}

static void PingClientDestroy(PingClient* pingClient)
{
	BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
		"sn(0x%p) destroyed.", pingClient);

	BdtReleasePeerInfo(pingClient->snPeerInfo);
	for (size_t i = 0; i < pingClient->sessionCount; i++)
	{
		Bdt_UdpInterfaceRelease(pingClient->sessions[i].handle);
	}
	BFX_FREE(pingClient);
}

static void PingClientStart(PingClient* pingClient)
{
	BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
		"sn(0x%p) started.", pingClient);

	PingClientStatus oldStatus = BfxAtomicCompareAndSwap32((int32_t*)& pingClient->status,
		PING_CLIENT_STATUS_INIT,
		PING_CLIENT_STATUS_CONNECTING);

	if (oldStatus == PING_CLIENT_STATUS_INIT)
	{
		PingClientUpdateStatus(pingClient);
	}
}

static void PingClientUpdateStatus(PingClient* pingClient)
{
	if (pingClient->status == PING_CLIENT_STATUS_OFFLINE)
	{
		return;
	}

	BdtStack* stack = pingClient->stack;
	int64_t now = BDT_MS_NOW;
	const BdtSuperNodeClientConfig* config = &BdtStackGetConfig(stack)->snClient;
	int pingInterval = config->pingInterval;

	switch (pingClient->status)
	{
	case PING_CLIENT_STATUS_INIT: // fall through
	case PING_CLIENT_STATUS_CONNECTING:
	{
		if (pingClient->lastPingTime > 0 &&
			now - pingClient->lastPingTime > config->offlineTimeout)
		{
			PingClientStatus oldStatus = BfxAtomicExchange32((int32_t*)& pingClient->status, PING_CLIENT_STATUS_OFFLINE);
			if (oldStatus != PING_CLIENT_STATUS_OFFLINE)
			{
				BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
					"sn(0x%p) status changed to offline from %d.", pingClient, oldStatus);

				Bdt_PushSuperNodeClientEvent(BdtStackGetEventCenter(stack),
					(void*)pingClient->snPeerInfo,
					BDT_SUPER_NODE_CLIENT_EVENT_OFFLINE,
					NULL);
			}
		}
		else
		{
			pingInterval = config->pingIntervalInit;
		}
	}
	break;
	case PING_CLIENT_STATUS_ONLINE:
	{
		if (now - pingClient->lastRespTime > config->offlineTimeout)
		{
			PingClientStatus oldStatus = BfxAtomicExchange32((int32_t*)& pingClient->status, PING_CLIENT_STATUS_OFFLINE);
			if (oldStatus != PING_CLIENT_STATUS_OFFLINE)
			{
				BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
					"sn(0x%p) status changed to offline from %d.", pingClient, oldStatus);

				Bdt_PushSuperNodeClientEvent(BdtStackGetEventCenter(stack),
					(void*)pingClient->snPeerInfo,
					BDT_SUPER_NODE_CLIENT_EVENT_OFFLINE,
					NULL);
			}
		}
	}
	break;
	case PING_CLIENT_STATUS_OFFLINE: // fall through
		break;
	default:
		return;
	}

	if (pingClient->status == PING_CLIENT_STATUS_OFFLINE)
	{
		return;
	}

	if (now - pingClient->lastPingTime > pingInterval)
	{
		PingClientSendPing(pingClient);
	}
}

static void PingClientStop(PingClient* pingClient)
{
	BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
		"sn(0x%p) will stop.", pingClient);

	if (pingClient->status == PING_CLIENT_STATUS_OFFLINE)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
			"sn(0x%p) has offline.", pingClient);

		return;
	}

	BdtStack* stack = pingClient->stack;
	PingClientStatus oldStatus = BfxAtomicExchange32((int32_t*)& pingClient->status, PING_CLIENT_STATUS_OFFLINE);
	if (oldStatus != PING_CLIENT_STATUS_OFFLINE)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
			"sn(0x%p) status changed to offline from %d, for stop.", pingClient, oldStatus);

		Bdt_PushSuperNodeClientEvent(BdtStackGetEventCenter(stack),
			(void*)pingClient->snPeerInfo,
			BDT_SUPER_NODE_CLIENT_EVENT_OFFLINE,
			NULL);
	}
}

static bool PingClientOnResponce(
    PingClient* pingClient,
	Bdt_SnPingRespPackage* package,
	const BdtEndpoint* fromEndpoint,
	const Bdt_UdpInterface* fromInterface,
    const BdtEndpoint** newOutEndpoint
)
{
#ifndef BLOG_DISABLE
	char fromEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
		"sn(0x%p) responce from (%s).",
		pingClient, (BdtEndpointToString(fromEndpoint, fromEpStr), fromEpStr));
#endif

    int64_t now = BDT_MS_NOW;
	pingClient->lastRespTime = now;

    bool isEndpointChange = false;
	BdtStack* stack = pingClient->stack;

    assert(package->endpointArray.size == 1);

    // 找到interface对应的session更新其中信息
    const BdtEndpoint* outEndpoint = package->endpointArray.list;
    for (size_t i = 0; i < pingClient->sessionCount; i++)
    {
        PingSession* session = pingClient->sessions + i;
        if (session->handle == fromInterface)
        {
            if (outEndpoint->flag & BDT_ENDPOINT_IP_VERSION_6)
            {
                if (memcmp(outEndpoint->addressV6, session->outEndpoint.addressV6, 8) != 0
                    || outEndpoint->port != session->outEndpoint.port)
                {
                    isEndpointChange = true;
                }
            }
            else if (BdtEndpointCompare(&session->outEndpoint, outEndpoint, false) != 0)
            {
                isEndpointChange = true;
            }

            if (isEndpointChange)
            {
                session->outEndpoint = *outEndpoint;
                session->outEndpoint.flag &= ~BDT_ENDPOINT_FLAG_SIGNED;
                session->outEndpoint.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
                *newOutEndpoint = &session->outEndpoint;
                // 前半段网络地址，后半段主机地址
                if (outEndpoint->flag & BDT_ENDPOINT_IP_VERSION_6)
                {
                    memcpy(session->outEndpoint.addressV6 + 8,
                        Bdt_UdpInterfaceGetLocal(fromInterface)->addressV6 + 8,
                        8);
                }

#ifndef BLOG_DISABLE
                char foundEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
                BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
                    "sn(0x%p) found new endpoint from (%s).",
                    pingClient, (BdtEndpointToString(&session->outEndpoint, foundEpStr), foundEpStr), fromEpStr);
#endif
            }
            if (session->lastRespTime == 0)
            {
                session->remoteEndpoint = *fromEndpoint;
            }
            session->lastRespTime = now;
        }
    }

	PingClientStatus oldStatus = BfxAtomicCompareAndSwap32((int32_t*)& pingClient->status,
		PING_CLIENT_STATUS_CONNECTING,
		PING_CLIENT_STATUS_ONLINE);

	if (oldStatus == PING_CLIENT_STATUS_CONNECTING)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
			"sn(0x%p) status changed to online from %d.", pingClient, oldStatus);

		Bdt_PushSuperNodeClientEvent(BdtStackGetEventCenter(stack), (void*)pingClient->snPeerInfo, BDT_SUPER_NODE_CLIENT_EVENT_ONLINE, NULL);
	}

    return isEndpointChange;
}

static void PingClientSendPing(PingClient* pingClient)
{
	BdtStack* stack = pingClient->stack;
	BdtSystemFramework* framework = BdtStackGetFramework(stack);
	uint32_t result = BFX_RESULT_SUCCESS;

	const BdtPeerInfo* localPeer = BdtPeerFinderGetLocalPeer(BdtStackGetPeerFinder(stack));

	// fromPeerid被peerinfo字段包含，不专门填充
	Bdt_Package* pkgs[2] = { NULL, NULL };
	Bdt_ExchangeKeyPackage* exchgKey = NULL;
	int pkgCount = 0;
	uint32_t seq = SuperNodeClientNextSeq(BdtStackGetSnClient(pingClient->stack));
	if (/*pingClient->lastRespTime == 0 && */pingClient->encrypto)
	{
		exchgKey = Bdt_ExchangeKeyPackageCreate();
		Bdt_ExchangeKeyPackageInit(exchgKey);

		exchgKey->cmdflags = BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQ |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQANDKEYSIGN |
			BDT_EXCHANGEKEY_PACKAGE_FLAG_SENDTIME;

		exchgKey->peerInfo = localPeer;
		exchgKey->seq = seq;
		exchgKey->sendTime = BfxTimeGetNow(false);
		pkgs[0] = (Bdt_Package*)exchgKey;
		pkgCount++;
	}

	Bdt_SnPingPackage* pingPkg = Bdt_SnPingPackageCreate();
	Bdt_SnPingPackageInit(pingPkg);
	pingPkg->cmdflags = BDT_SN_PING_PACKAGE_FLAG_SEQ |
		BDT_SN_PING_PACKAGE_FLAG_PEERINFO;

	pingPkg->seq = seq;
	pingPkg->peerInfo = localPeer;
	pkgs[pkgCount] = (Bdt_Package*)pingPkg;
	pkgCount++;

	// Bdt_StackTlsData* tls = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	Bdt_StackTlsData tlsObj;
	Bdt_StackTlsData* tls = &tlsObj;
	size_t encodedSize = sizeof(tls->udpEncodeBuffer);

	if (exchgKey == NULL)
	{
		if (pingClient->encrypto)
		{
			result = Bdt_BoxEncryptedUdpPackage(
				pkgs, 
				pkgCount, 
				NULL, 
				pingClient->aesKey, 
				tls->udpEncodeBuffer, 
				&encodedSize);
		}
		else
		{
			result = Bdt_BoxUdpPackage(
				(const Bdt_Package**)pkgs, 
				pkgCount, 
				NULL, 
				tls->udpEncodeBuffer, 
				&encodedSize);
		}
	}
	else
	{
		const BdtPeerSecret* secret = BdtStackGetSecret(stack);
		const BdtPeerConstInfo* snPeerConstInfo = BdtPeerInfoGetConstInfo(pingClient->snPeerInfo);
		result = Bdt_BoxEncryptedUdpPackageStartWithExchange(
			(const Bdt_Package**)pkgs,
			pkgCount, 
			NULL, 
			pingClient->aesKey,
			snPeerConstInfo->publicKeyType,
			(const uint8_t*)& snPeerConstInfo->publicKey,
			secret->secretLength,
			(const uint8_t*)secret,
			tls->udpEncodeBuffer, &encodedSize);
	}

	if (exchgKey != NULL)
	{
		exchgKey->peerInfo = NULL;
		Bdt_ExchangeKeyPackageFree(exchgKey);
	}

	if (pingPkg != NULL)
	{
		pingPkg->peerInfo = NULL;
		Bdt_SnPingPackageFree(pingPkg);
	}
	BdtReleasePeerInfo(localPeer);

	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
			"sn(0x%p) box failed, result: %d, pkgCount:%d.",
			pingClient, result, pkgCount);

		return;
	}

    int64_t now = BDT_MS_NOW;

    pingClient->lastPingTime = now;

	for (size_t i = 0; i < pingClient->sessionCount; i++)
	{
        PingSession* session = pingClient->sessions + i;
		const Bdt_UdpInterface* udpInterface = session->handle;
        const BdtEndpoint* localEp = Bdt_UdpInterfaceGetLocal(udpInterface);

        const BdtEndpoint* toEndpointArray = &session->remoteEndpoint;
        size_t toEndpointCount = 1;
        if (session->lastRespTime == 0) {
            toEndpointArray = BdtPeerInfoListEndpoint(pingClient->snPeerInfo, &toEndpointCount);
        }
        session->lastPingTime = now;
		for (size_t j = 0; j < toEndpointCount; j++)
		{
			const BdtEndpoint* toEp = toEndpointArray + j;
			if ((toEp->flag & BDT_ENDPOINT_PROTOCOL_UDP) == 0 ||
                !BdtEndpointIsSameIpVersion(toEp, localEp))
			{
				continue;
			}

			{
#ifndef BLOG_DISABLE
				char localPeerStr[1024];
				size_t localPeerStrSize = 1024;
				char fromEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
				char toEpStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
				BLOG_HEX(BdtPeerInfoGetPeerid(pingClient->snPeerInfo)->pid, 15,
					"sn(0x%p) send to %s, from %s, localPeer:%s.",
					pingClient,
					(BdtEndpointToString(toEp, toEpStr), toEpStr),
					(BdtEndpointToString(Bdt_UdpInterfaceGetLocal(udpInterface), fromEpStr), fromEpStr),
					BdtPeerInfoFormat(localPeer, localPeerStr, &localPeerStrSize)
				);
#endif
			}
           

			BdtUdpSocketSendTo(framework, Bdt_UdpInterfaceGetSocket(udpInterface), toEp, tls->udpEncodeBuffer, encodedSize);
		}
	}
}