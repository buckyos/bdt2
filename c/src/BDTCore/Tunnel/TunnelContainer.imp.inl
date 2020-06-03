#ifndef __BDT_TUNNEL_CONTAINER_IMP_INL__
#define __BDT_TUNNEL_CONTAINER_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include <assert.h>
#include <SGLib/SGLib.h>
#include "../Stack.h"
#include "./TunnelContainer.inl"
#include "./UdpTunnel.inl"
#include "./TcpTunnel.inl"

typedef struct TunnelMap
{
	struct TunnelMap* left;
	struct TunnelMap* right;
	uint8_t color;
	const BdtEndpoint* local;
	const BdtEndpoint* remote;
	SocketTunnel* tunnel;
} TunnelMap, tunnel_map;

static int TunnelMapCompare(const TunnelMap* left, const TunnelMap* right)
{
	int result = BdtEndpointCompare(left->local, right->local, false);
	if (result)
	{
		return result;
	}
	return BdtEndpointCompare(left->remote, right->remote, false);
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(tunnel_map, left, right, color, TunnelMapCompare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(tunnel_map, left, right, color, TunnelMapCompare)

typedef struct WaitingBuilderConnection
{
	BfxListItem base;
	BdtConnection* connection;
} WaitingBuilderConnection;


typedef struct RespMap
{
	struct RespMap* left;
	struct RespMap* right;
	uint8_t color;
	uint32_t seq;
	const Bdt_PackageWithRef* resp;
} RespMap, resp_map;

static int resp_map_compare(const resp_map* left, const resp_map* right)
{
	if (left->seq == right->seq)
	{
		return 0;
	}
	return left->seq > right->seq ? 1 : -1;
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(resp_map, left, right, color, resp_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(resp_map, left, right, color, resp_map_compare)

typedef struct RespListItem
{
	BfxListItem base;
	RespMap node;
} RespListItem;

#define GET_RESP_LIST_ITEM_BY_MAP_NODE(nodeAddress) (RespListItem*)(((uint8_t*)(nodeAddress)) - (uint8_t*)&((RespListItem*)NULL)->node)

struct Bdt_TunnelContainer
{
	int32_t refCount;
	BdtSystemFramework* framework;
	Bdt_StackTls* tls;
	
	const BdtPeerid* localPeerid;
	const BdtPeerSecret* localSecret;
	Bdt_TunnelHistory* history;
	Bdt_NetManager* netManager;
	Bdt_EventCenter* eventCenter;
	BdtPeerFinder* peerFinder;
	BdtHistory_KeyManager* keyManager;
	BdtSuperNodeClient* snClient;
	Bdt_ConnectionManager* connectionManager;
	BdtPeerid remotePeerid;
	BdtPeerConstInfo* remoteConst;
	char* name;
	const BdtTunnelConfig* config;
	Bdt_SequenceCreator seqCreator;

	BfxThreadLock builderLock;
	// builderLock protected members begin
	TunnelBuilder* builder;
	BfxList waitingConnections;
	// builderLock protected members begin

	BfxRwLock tunnelsLock;
	// tunnelsLock protected members begin
	TunnelMap* tunnels;
	// tunnelsLock protected members end
	
	BfxRwLock defaultLock;
	// defaultLock protected members begin
	SocketTunnel* defaultTunnel;
	// defaultLock protected members end

	// pirority map
	BfxThreadLock respCacheLock;
	// respCacheLock protected members begin
	BfxList respList; // FIFO
	RespMap* respMap;
	// respCacheLock protected members finish
};

static Bdt_ConnectionManager* TunnelContainerGetConnectionManager(
	TunnelContainer* container
)
{
	return container->connectionManager;
}

static BdtPeerFinder* TunnelContainerGetPeerFinder(TunnelContainer* container)
{
	return container->peerFinder;
}

static BdtSuperNodeClient* TunnelContainerGetSnClient(TunnelContainer* container)
{
	return container->snClient;
}




static uint32_t TunnelContainerSetDefaultTunnel(
	TunnelContainer* container, 
	SocketTunnel* tunnel)
{
	bool changed = false;
	SocketTunnel* old = NULL;

	BfxRwLockWLock(&container->defaultLock);
	if (container->defaultTunnel != tunnel)
	{
		old = container->defaultTunnel;
		container->defaultTunnel = tunnel;
		if (tunnel)
		{
			BDT_SOCKET_TUNNEL_ADDREF(tunnel);
		}
		changed = true;
	}
	BfxRwLockWUnlock(&container->defaultLock);

	if (old)
	{
		// may stop before start, use a active count in impl
		SOCKET_TUNNEL_STOP_ACTIVITOR(old);
		BLOG_DEBUG("%s change default tunnel old %s", container->name, Bdt_SocketTunnelGetName(old));
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
	}
	if (changed)
	{
		if (tunnel)
		{
			BLOG_INFO("%s default tunnel changed to %s", container->name, Bdt_SocketTunnelGetName(tunnel));
			SOCKET_TUNNEL_START_ACTIVITOR(tunnel);
		}
		else
		{
			BLOG_INFO("%s default tunnel changed to null", container->name);
		}
		
		Bdt_PushTunnelContainerEvent(container->eventCenter, container, BDT_EVENT_DEFAULT_TUNNEL_CHANGED, NULL);
	}
	return BFX_RESULT_SUCCESS;
}

static const BdtPeerConstInfo* TunnelContainerGetRemoteConst(TunnelContainer* container)
{
	if (container->remoteConst)
	{
		return container->remoteConst;
	}
	const BdtPeerInfo* remotePeer = BdtPeerFinderGetCachedOrStatic(container->peerFinder, &container->remotePeerid);
	assert(remotePeer);
	BdtPeerConstInfo* remoteConst = BFX_MALLOC_OBJ(BdtPeerConstInfo);
	*remoteConst = *BdtPeerInfoGetConstInfo(remotePeer);
	BdtReleasePeerInfo(remotePeer);
	if (BfxAtomicCompareAndSwapPointer(&container->remoteConst, NULL, remoteConst))
	{
		BfxFree(remoteConst);
	}
	return container->remoteConst;
}


static void TunnelContainerOnHistoryLoad(
	Bdt_TunnelHistory* history,
	void* userData
);

static TunnelContainer* TunnelContainerCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls, 
	const BdtPeerid* localPeerid, 
	const BdtPeerSecret* localSecret,
	Bdt_NetManager* netManager,
	Bdt_EventCenter* eventCenter, 
	BdtPeerFinder* peerFinder, 
	BdtHistory_KeyManager* keyManager,
	BdtSuperNodeClient* snClient,
	Bdt_ConnectionManager* connectionManager, 
    BdtHistory_PeerUpdater* updater,
	const BdtPeerid* remote,
	const BdtTunnelConfig* config
)
{
	TunnelContainer* container = BFX_MALLOC_OBJ(TunnelContainer);
	memset(container, 0, sizeof(TunnelContainer));
	container->refCount = 1;
	container->framework = framework;
	container->localPeerid = localPeerid;
	container->netManager = netManager;
	container->eventCenter = eventCenter;
	container->localSecret = localSecret;
	container->keyManager = keyManager;
	container->tls = tls;
	// container->history = TunnelHistoryCreate(&config->history);

	BfxUserData udHistoryLoad = {
		.lpfnAddRefUserData = NULL, 
		.lpfnReleaseUserData = NULL,
		.userData = container
	};
	// make sure history life cycle longer than history
	container->history = Bdt_TunnelHistoryCreate(
		remote, 
		updater, 
		&config->history, 
		TunnelContainerOnHistoryLoad, 
		&udHistoryLoad);

	container->peerFinder = peerFinder;
	container->snClient = snClient;
	container->connectionManager = connectionManager;
	container->remotePeerid = *remote;
	container->config = config;
	Bdt_SequenceInit(&container->seqCreator);

	BfxRwLockInit(&container->tunnelsLock);
	
	BfxThreadLockInit(&container->builderLock);
	BfxListInit(&container->waitingConnections);

	BfxRwLockInit(&container->defaultLock);

	BfxThreadLockInit(&container->respCacheLock);
	BfxListInit(&container->respList);

	const char* prename = "TunnelContainer:";
	size_t prenameLen = strlen(prename);
	size_t nameLen = prenameLen + BDT_PEERID_STRING_LENGTH + 1;
	container->name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(container->name, 0, nameLen);
	memcpy(container->name, prename, prenameLen);
	BdtPeeridToString(remote, container->name + prenameLen);

	return container;
}

// not thread safe
// should not reenter
static void TunnelContainerUninit(TunnelContainer * container)
{
	assert(!sglib_tunnel_map_len(container->tunnels));
	BfxRwLockDestroy(&container->tunnelsLock);
	BfxRwLockDestroy(&container->defaultLock);
	BfxThreadLockDestroy(&container->builderLock);
	if (container->remoteConst)
	{
		BfxFree(container->remoteConst);
	}
	if (container->defaultTunnel)
	{
		BDT_SOCKET_TUNNEL_RELEASE(container->defaultTunnel);
	}

	{
		BfxThreadLockDestroy(&container->respCacheLock);
		RespListItem* item = NULL;

		while ((item = (RespListItem*)BfxListPopFront(&container->respList)) != NULL)
		{
			if (item->node.resp != NULL)
			{
				Bdt_PackageRelease(item->node.resp);
			}
			BfxFree(item);
		}

		// mapû�ж�������ڴ�
		container->respMap = NULL;
	}

	BFX_FREE(container->name);
	Bdt_TunnelHistoryRelease(container->history);
	memset(container, 0, sizeof(TunnelContainer));
}

const char* Bdt_TunnelContainerGetName(Bdt_TunnelContainer* container)
{
	return container->name;
}

int32_t Bdt_TunnelContainerAddRef(Bdt_TunnelContainer* container)
{
	return BfxAtomicInc32(&container->refCount);
}

int Bdt_TunnelContainerRelease(Bdt_TunnelContainer* container)
{
	int32_t ref = BfxAtomicDec32(&container->refCount);
	if (ref <= 0)
	{
		TunnelContainerUninit(container);
		BFX_FREE(container);
		return ref;
	}
	return ref;
}

const BdtPeerid* BdtTunnel_GetRemotePeerid(Bdt_TunnelContainer* container)
{
	return &(container->remotePeerid);
}

const BdtPeerid* BdtTunnel_GetLocalPeerid(Bdt_TunnelContainer* container)
{
	return container->localPeerid;
}

Bdt_TunnelHistory* BdtTunnel_GetHistory(Bdt_TunnelContainer* container)
{
	return container->history;
}

uint32_t BdtTunnel_GetNextSeq(Bdt_TunnelContainer* container)
{
	return Bdt_SequenceNext(&container->seqCreator);
}

const BdtTunnelConfig* Bdt_TunnelContainerGetConfig(Bdt_TunnelContainer* container)
{
	return container->config;
}

static uint32_t TunnelContainerSendFromTunnel(
	TunnelContainer* container, 
	SocketTunnel* tunnel, 
	const Bdt_Package** packages, 
	size_t* inoutCount
	)
{
	if (!*inoutCount)
	{
		return BFX_RESULT_SUCCESS;
	}
	Bdt_TunnelEncryptOptions opt;
	uint32_t result = BdtTunnel_GetEnryptOptions(container, &opt);
	if (result)
	{
		return result;
	}
	const Bdt_Package* toSend[BDT_MAX_PACKAGE_MERGE_LENGTH];
	size_t sent = 0;
	Bdt_ExchangeKeyPackage exchange;
	Bdt_ExchangeKeyPackageInit(&exchange);
	if (opt.exchange)
	{
		Bdt_ExchangeKeyPackageFillWithNext(&exchange, packages[0]);
		exchange.sendTime = 0;
		toSend[sent++] = (const Bdt_Package*)&exchange;
	}
	for (size_t ix = 0; ix < *inoutCount; ++ix)
	{
		toSend[sent++] = packages[ix];
	}

	BLOG_DEBUG("%s send package from %s", container->name, Bdt_SocketTunnelGetName(tunnel));
	if (Bdt_SocketTunnelIsUdp(tunnel))
	{
		result = UdpTunnelSend((UdpTunnel*)tunnel, toSend, &sent, &opt);
		if (sent)
		{
			*inoutCount = sent - (opt.exchange ? 1 : 0);
		}
	}
	else if (Bdt_SocketTunnelIsTcp(tunnel))
	{
		result = TcpTunnelSend((TcpTunnel*)tunnel, toSend, sent);
	}
	else
	{
		assert(false);
		result = BFX_RESULT_FAILED;
	}
	
	Bdt_ExchangeKeyPackageFinalize(&exchange);
	return result;
}

uint32_t BdtTunnel_GetEnryptOptions(
	TunnelContainer* container,
	Bdt_TunnelEncryptOptions* outOptions
)
{
	bool newKey = true;
	bool confirmed = true;
	// tofix: may cache key in container
	uint32_t result = BdtHistory_KeyManagerGetKeyByRemote(
		container->keyManager,
		&container->remotePeerid,
		outOptions->key,
		&newKey, 
		&confirmed,
		false
	);
	assert(!result);
	if (result)
	{
		BLOG_DEBUG("%s get key failed for %u", container->name, result);
		return result;
	}
	outOptions->exchange = !confirmed;
	outOptions->remoteConst = outOptions->exchange ? TunnelContainerGetRemoteConst(container) : NULL;
	outOptions->localSecret = outOptions->exchange ? container->localSecret : NULL;
	return BFX_RESULT_SUCCESS;
}


static uint32_t TunnelContainerUpdateTcpTunnel(
	TunnelContainer* container,
	Bdt_TcpInterface* tcpInterface,
	TcpTunnel* tunnel)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtEndpoint remoteEndpoint = *Bdt_TcpInterfaceGetRemote(tcpInterface);
	BdtEndpoint localEndpoint = *Bdt_TcpInterfaceGetLocal(tcpInterface);
	if (!tunnel)
	{
		tunnel = (TcpTunnel*)BdtTunnel_GetTunnelByEndpoint(
			container,
			&localEndpoint,
			&remoteEndpoint);
		if (!tunnel)
		{
			result = TunnelContainerCreateTunnel(
				container,
				Bdt_TcpInterfaceGetLocal(tcpInterface),
				&remoteEndpoint, 
				0, 
				(Bdt_SocketTunnel**)&tunnel);
		}

		BLOG_DEBUG("%s update tcp tunnel %s", container->name, Bdt_SocketTunnelGetName((SocketTunnel*)tunnel));
	}
	else
	{
		result = BFX_RESULT_ALREADY_EXISTS;
	}

	if (result == BFX_RESULT_SUCCESS)
	{
		SocketTunnel* defaultTunnel = TunnelContainerUpdateDefaultTunnel(container);
		BDT_SOCKET_TUNNEL_RELEASE(defaultTunnel);
	}

	return result;
}

static uint32_t TunnelContainerCreateTunnel(
	TunnelContainer* container,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	uint64_t lastRecv, 
	SocketTunnel** outTunnel
)
{
	SocketTunnel* tunnel = NULL;
	if (localEndpoint->flag & BDT_ENDPOINT_PROTOCOL_UDP)
	{
		tunnel = (SocketTunnel*)UdpTunnelCreate(
			container->framework, 
			container->tls, 
			container, 
			localEndpoint,
			&container->remotePeerid,
			remoteEndpoint,
			&container->config->udpTunnel, 
			lastRecv);
	}
	else if (localEndpoint->flag & BDT_ENDPOINT_PROTOCOL_TCP)
	{
		tunnel = (SocketTunnel*)TcpTunnelCreate(
			container->framework,
			container->netManager, 
			container->tls,
			container->peerFinder,
			container, 
			localEndpoint,
			remoteEndpoint,
			&container->config->tcpTunnel);
	}
	else
	{
		assert(false);
		return BFX_RESULT_INVALID_PARAM;
	}
	TunnelMap* add = BFX_MALLOC_OBJ(TunnelMap);
	add->tunnel = tunnel;
	add->local = Bdt_SocketTunnelGetLocalEndpoint(tunnel);
	add->remote = Bdt_SocketTunnelGetRemoteEndpoint(tunnel);
	BDT_SOCKET_TUNNEL_ADDREF(tunnel);
	
	TunnelMap* exists = NULL;
	BfxRwLockWLock(&container->tunnelsLock);
	{
		sglib_tunnel_map_add_if_not_member(&container->tunnels, add, &exists);
	}
	if (exists)
	{
		BDT_SOCKET_TUNNEL_ADDREF(exists->tunnel);
	}
	BfxRwLockWUnlock(&container->tunnelsLock);


	if (exists)
	{
		BFX_FREE(add);
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
		*outTunnel = exists->tunnel;
		return BFX_RESULT_ALREADY_EXISTS;
	}
	
	BLOG_DEBUG("%s add socket tunnel %s", container->name, Bdt_SocketTunnelGetName(tunnel));
	*outTunnel = tunnel;
	return BFX_RESULT_SUCCESS;
}


static uint32_t TunnelContainerRemoveTunnel(
	TunnelContainer* container,
	SocketTunnel* tunnel, 
	int64_t lastRecv
)
{
	TunnelMap toDel = {
		.local = Bdt_SocketTunnelGetLocalEndpoint(tunnel), 
		.remote = Bdt_SocketTunnelGetRemoteEndpoint(tunnel)
	};
	TunnelMap* del = NULL;
	BfxRwLockWLock(&container->tunnelsLock);
	{
		del = sglib_tunnel_map_find_member(container->tunnels, &toDel);
		if (del->tunnel == tunnel && SocketTunnelGetLastRecv(del->tunnel) == lastRecv)
		{
			sglib_tunnel_map_delete(&container->tunnels, del);
		}
		else
		{
			del = NULL;
		}
	}
	BfxRwLockWUnlock(&container->tunnelsLock);

	if (del)
	{
		BLOG_DEBUG("%s removed from %s", tunnel->name, container->name);
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
		BfxFree(del);
		return BFX_RESULT_SUCCESS;
	}
	else
	{
		return BFX_RESULT_NOT_FOUND;
	}
}

uint32_t BdtTunnel_OnTcpFirstResp(
	Bdt_TunnelContainer* container,
	Bdt_TcpTunnel* tunnel, 
	Bdt_TcpInterface* tcpInterface,
	const BdtPeerInfo* remoteInfo
)
{
	TunnelContainerUpdateTcpTunnel(container, tcpInterface, tunnel);
	return Bdt_TunnelHistoryAddTcpLog(
		container->history,
		Bdt_TcpInterfaceGetLocal(tcpInterface),
		Bdt_TcpInterfaceGetRemote(tcpInterface),
		BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_OK
	);
}

uint32_t BdtTunnel_OnTcpFirstPackage(
	Bdt_TunnelContainer* container,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package* firstPackage
)
{
	assert(firstPackage);
	uint32_t result = BFX_RESULT_SUCCESS;
	TcpTunnel* tunnel = NULL;

	switch (firstPackage->cmdtype)
	{
	case BDT_SYN_TUNNEL_PACKAGE:
	{
		BLOG_DEBUG("%s got passive tcp interface with first syn tunnel on tcp interface %s", container->name, Bdt_TcpInterfaceGetName(tcpInterface));

		tunnel = (TcpTunnel*)BdtTunnel_GetTunnelByEndpoint(
			container,
			Bdt_TcpInterfaceGetLocal(tcpInterface),
			Bdt_TcpInterfaceGetRemote(tcpInterface));
		if (tunnel == NULL)
		{
			TunnelContainerCreateTunnel(
				container,
				Bdt_TcpInterfaceGetLocal(tcpInterface),
				Bdt_TcpInterfaceGetRemote(tcpInterface),
				0, 
				(SocketTunnel**)&tunnel);
		}
		result = TcpTunnelOnTcpFirstPackage(tunnel, tcpInterface, firstPackage);
		break;
	}
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
	{
		BLOG_DEBUG("%s got passive tcp interface with first tcp syn connection on tcp interface %s", container->name, Bdt_TcpInterfaceGetName(tcpInterface));

		const Bdt_TcpSynConnectionPackage* syn = (Bdt_TcpSynConnectionPackage*)firstPackage;
		BdtConnectionListener* listener = NULL;
		BdtConnection* connection = Bdt_GetConnectionByRemoteSeq(
			container->connectionManager,
			&syn->fromPeerid,
			syn->seq);
		if (!connection)
		{
			size_t fqLen = 0;
			const uint8_t* fq = NULL;
			if (syn->payload)
			{
				fq = BfxBufferGetData(syn->payload, &fqLen);
			}
			uint32_t addResult = Bdt_AddPassiveConnection(
				container->connectionManager,
				syn->toVPort,
				&syn->fromPeerid,
				syn->fromSessionId,
				syn->seq, 
				fq, 
				fqLen, 
				&connection,
				&listener
			);
		}
		uint32_t result = Bdt_ConnectionOnTcpFirstPackage(connection, tcpInterface, firstPackage);
		if (listener)
		{
			result = Bdt_FirePreAcceptConnection(container->connectionManager, listener, connection);
			BdtConnectionListenerRelease(listener);
		}
		break;
	}
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
	{
		BLOG_DEBUG("%s got passive tcp interface with first tcp ack connection on tcp interface %s", container->name, Bdt_TcpInterfaceGetName(tcpInterface));

		const Bdt_TcpAckConnectionPackage* ack = (Bdt_TcpAckConnectionPackage*)firstPackage;
		BdtConnection* connection = Bdt_GetConnectionById(container->connectionManager, ack->toSessionId);
		result = Bdt_ConnectionOnTcpFirstPackage(connection, tcpInterface, firstPackage);
		break;
	}
	default:
		BLOG_ERROR("%s recv unknown tcp first package from tcp interface %s", container->name, Bdt_TcpInterfaceGetName(tcpInterface));
		result = BFX_RESULT_INVALID_PARAM;
	}


	if (result == BFX_RESULT_SUCCESS)
	{
		result = TunnelContainerUpdateTcpTunnel(container, tcpInterface, tunnel);

		Bdt_TunnelHistoryAddTcpLog(
			container->history,
			Bdt_TcpInterfaceGetLocal(tcpInterface),
			Bdt_TcpInterfaceGetRemote(tcpInterface),
			BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_OK
		);
	}
	else
	{
		BLOG_DEBUG("%s reset tcp interface %s for invalid first package", container->name, Bdt_TcpInterfaceGetName(tcpInterface));

		Bdt_TcpInterfaceReset(tcpInterface);
	}

	return result;
}

uint32_t BdtTunnel_PushUdpPackage(
	Bdt_TunnelContainer* container,
	Bdt_Package* package,
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote,
	bool* outHandled
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	//todo: verify package with seq or sendtime
	Bdt_UdpTunnel* tunnel = (Bdt_UdpTunnel*)BdtTunnel_GetTunnelByEndpoint(container, Bdt_UdpInterfaceGetLocal(udpInterface), remote);
	if (tunnel == NULL)
	{
		result = TunnelContainerCreateTunnel(
			container, 
			Bdt_UdpInterfaceGetLocal(udpInterface), 
			remote, 
			0, 
			(Bdt_SocketTunnel**)&tunnel);
	
		if (result == BFX_RESULT_SUCCESS)
		{
			UdpTunnelSetInterface(tunnel, udpInterface);
		}
	}
	else
	{
		// to ignore update default tunnel
		result = BFX_RESULT_ALREADY_EXISTS;
	}

	{
		const char* cmdName = Bdt_PackageGetNameByCmdType(package->cmdtype);
		BLOG_DEBUG("%s got package %s on %s", container->name, cmdName, Bdt_SocketTunnelGetName((SocketTunnel*)tunnel));
	}

	SocketTunnelUpdateLastRecv((SocketTunnel*)tunnel);
	if (result == BFX_RESULT_SUCCESS)
	{
		SocketTunnel* defaultTunnel = TunnelContainerUpdateDefaultTunnel(container);
		BDT_SOCKET_TUNNEL_RELEASE(defaultTunnel);
	}

	const BdtPeerInfo* remoteInfo = NULL;
	if (package->cmdtype == BDT_SYN_TUNNEL_PACKAGE)
	{
		// if syn merged with session object's first package, and session object has confirmed, 
		//		response ack merged with session object's first resp
		remoteInfo = ((Bdt_SynTunnelPackage*)package)->peerInfo;
		BdtPeerFinderAddCached(container->peerFinder, remoteInfo, 0);
		const Bdt_PackageWithRef* resp = TunnelContainerGetCachedResponse(
			container, 
			((Bdt_SynTunnelPackage*)package)->seq);
		if (resp)
		{
			Bdt_AckTunnelPackage ack;
			Bdt_AckTunnelPackageInit(&ack);
			ack.seq = ((Bdt_SynTunnelPackage*)package)->seq;
			ack.peerInfo = BdtPeerFinderGetLocalPeer(container->peerFinder);
			size_t toSendLen = 1;
			const Bdt_Package* toSend[2];
			toSend[0] = (Bdt_Package*)&ack;
			++toSendLen;
			toSend[1] = Bdt_PackageWithRefGet(resp);
			TunnelContainerSendFromTunnel(container, (Bdt_SocketTunnel*)tunnel, toSend, &toSendLen);
			Bdt_PackageRelease(resp);
			Bdt_AckTunnelPackageFinalize(&ack);
			*outHandled = true;

			BLOG_DEBUG("%s reply syn tunnel package %u with cached response", container->name, ack.seq);
		}
	}
	else if (package->cmdtype == BDT_ACK_TUNNEL_PACKAGE)
	{
		remoteInfo = ((Bdt_AckTunnelPackage*)package)->peerInfo;
		BdtPeerFinderAddCached(container->peerFinder, remoteInfo, 0);
		// todo: send ackack?
	}
	else if (package->cmdtype == BDT_PING_TUNNEL_PACKAGE)
	{
		Bdt_PingTunnelRespPackage resp;
		Bdt_PingTunnelRespPackageInit(&resp);
		//tofix: fill resp's package id
		size_t toSendLen = 1;
		const Bdt_Package* toSend[1];
		toSend[0] = (Bdt_Package*)&resp;
		TunnelContainerSendFromTunnel(container, (SocketTunnel*)tunnel, toSend, &toSendLen);
		Bdt_PingTunnelRespPackageFinalize(&resp);

		BLOG_DEBUG("%s reply ping from %s", container->name, Bdt_SocketTunnelGetName((SocketTunnel*)tunnel));
	}
	else if (package->cmdtype == BDT_PING_TUNNEL_RESP_PACKAGE)
	{
		// do nothing

		BLOG_DEBUG("%s got ping resp from %s", container->name, Bdt_SocketTunnelGetName((SocketTunnel*)tunnel));
	}
	else if (package->cmdtype == BDT_SESSION_DATA_PACKAGE)
	{
		// response cached session data with ack
		const Bdt_SessionDataPackage* sessionData = (Bdt_SessionDataPackage*)package;
		if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN)
		{
			if (!(sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK))
			{
				const Bdt_PackageWithRef* resp = TunnelContainerGetCachedResponse(
					container,
					sessionData->synPart->synSeq);
				if (resp)
				{
					size_t toSendLen = 1;
					const Bdt_Package* toSend[1];
					toSend[0] = Bdt_PackageWithRefGet(resp);
					TunnelContainerSendFromTunnel(container, (Bdt_SocketTunnel*)tunnel, toSend, &toSendLen);
					Bdt_PackageRelease(resp);
					*outHandled = true;

					BLOG_DEBUG("%s reply session data with syn flag %u with cached response", container->name, sessionData->synPart->synSeq);
				}
			}
		}
	}

	Bdt_TunnelHistoryOnPackageFromRemotePeer(
		container->history, 
		remoteInfo, 
		remote, 
		Bdt_UdpInterfaceGetLocal(udpInterface), 
		0
	);

	return BFX_RESULT_SUCCESS;
}

uint32_t BdtTunnel_PushSessionPackage(
	Bdt_TunnelContainer* container,
	const Bdt_Package* package,
	bool* handled
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	if (package->cmdtype == BDT_SESSION_DATA_PACKAGE
		|| package->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
	{
		result = Bdt_ConnectionManagerPushPackage(
			container->connectionManager,
			package,
			container,
			handled);
	}

	return result;
}

void BdtTunnel_OnFoundPeer(Bdt_TunnelContainer* container, const BdtPeerInfo* remotePeer)
{
	// todo: mobile compability
}

struct Bdt_TunnelIterator
{
	Bdt_TunnelContainer* container;
	struct sglib_tunnel_map_iterator iter;
};

Bdt_SocketTunnel* BdtTunnel_EnumTunnels(
	Bdt_TunnelContainer* container,
	Bdt_TunnelIterator** outIter)
{
	Bdt_TunnelIterator* iter = BFX_MALLOC_OBJ(Bdt_TunnelIterator);
	iter->container = container;
	Bdt_TunnelContainerAddRef(container);
	*outIter = iter;
	BfxRwLockRLock(&container->tunnelsLock);
	TunnelMap* node = sglib_tunnel_map_it_init(&iter->iter, container->tunnels);
	return node ? node->tunnel : NULL;
}

Bdt_SocketTunnel* BdtTunnel_EnumTunnelsNext(
	Bdt_TunnelIterator* iter)
{
	TunnelMap* node = sglib_tunnel_map_it_next(&iter->iter);
	return node ? node->tunnel : NULL;
}

void BdtTunnel_EnumTunnelsFinish(
	Bdt_TunnelIterator* iter)
{
	BfxRwLockRUnlock(&iter->container->tunnelsLock);
	Bdt_TunnelContainerRelease(iter->container);
	BFX_FREE(iter);
}

Bdt_SocketTunnel* BdtTunnel_GetTunnelByEndpoint(
	Bdt_TunnelContainer* container,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint)
{
	TunnelMap toFind;
	toFind.local = localEndpoint;
	toFind.remote = remoteEndpoint;
	SocketTunnel* tunnel = NULL;
	BfxRwLockRLock(&container->tunnelsLock);
	TunnelMap* found = sglib_tunnel_map_find_member(container->tunnels, &toFind);
	if (found)
	{
		tunnel = found->tunnel;
		BDT_SOCKET_TUNNEL_ADDREF(tunnel);
	}
	BfxRwLockRUnlock(&container->tunnelsLock);
	return tunnel;
}

Bdt_SocketTunnel* BdtTunnel_GetDefaultTunnel(Bdt_TunnelContainer* container)
{
	Bdt_SocketTunnel* tunnel = NULL;
	BfxRwLockRLock(&container->defaultLock);
	tunnel = container->defaultTunnel;
	if (tunnel)
	{
		BDT_SOCKET_TUNNEL_ADDREF(tunnel);
	}
	BfxRwLockRUnlock(&container->defaultLock);
	return tunnel;
}


uint32_t BdtTunnel_Send(
	Bdt_TunnelContainer* container, 
	const Bdt_Package** packages, 
	size_t* inoutCount, 
	const BdtTunnel_SendParams* sendParams)
{
	static BdtTunnel_SendParams defaultParams = {
		.flags = BDT_TUNNEL_SEND_FLAGS_DEFAULT_ONLY, 
		.buildParams = NULL
	};
	if (!sendParams)
	{
		sendParams = &defaultParams;
	}

	Bdt_SocketTunnel* tunnel;
	BfxRwLockRLock(&container->defaultLock);
	tunnel = container->defaultTunnel;
	if (tunnel)
	{
		BDT_SOCKET_TUNNEL_ADDREF(tunnel);
	}
	BfxRwLockRUnlock(&container->defaultLock);

	uint32_t result = BFX_RESULT_SUCCESS;
	if (tunnel)
	{
		// default tunnel exists
		// udp only not set 
		// or udp only set and default is udp
		// use default tunnel to send
		if (!(sendParams->flags & BDT_TUNNEL_SEND_FLAGS_UDP_ONLY)
			|| Bdt_SocketTunnelIsUdp(tunnel))
		{
			result = TunnelContainerSendFromTunnel(container, tunnel, packages, inoutCount);
			BDT_SOCKET_TUNNEL_RELEASE(tunnel);
			return result;
		}
		else
		{
			BLOG_DEBUG("%s ignore BdtTunnel_Send for default tunnel is not udp", container->name);
			BDT_SOCKET_TUNNEL_RELEASE(tunnel);
			tunnel = NULL;
		}
	}
	else if (sendParams->flags == BDT_TUNNEL_SEND_FLAGS_DEFAULT_ONLY)
	{
		BLOG_DEBUG("%s ignore BdtTunnel_Send for no default tunnel exists", container->name);
		// if default tunnel not exists and default only set 
		// ignore send 
		return BFX_RESULT_NOT_FOUND;
	}
	
	// not default tunnel set, explore all existing tunnels 
	Bdt_TunnelIterator* iter = NULL;
	Bdt_SocketTunnel* curTunnel = BdtTunnel_EnumTunnels(container, &iter);
	while (curTunnel)
	{
		tunnel = curTunnel;
		if (Bdt_SocketTunnelIsUdp(tunnel))
		{
			// udp first, if any udp tunnel exists, use it to send
			// if not udp tunnel exists, use last existing tcp tunnel
			break;
		}
		curTunnel = BdtTunnel_EnumTunnelsNext(iter);
	} 
	if (tunnel)
	{
		BDT_SOCKET_TUNNEL_ADDREF(tunnel);
	}
	BdtTunnel_EnumTunnelsFinish(iter);

	if (sendParams->flags & BDT_TUNNEL_SEND_FLAGS_UDP_ONLY
		&& tunnel 
		&& !Bdt_SocketTunnelIsUdp(tunnel))
	{
		// when udp only set
		// if tunnel exists but no udp tunnel exists
		// ignore send
		BLOG_DEBUG("%s ignore BdtTunnel_Send for no udp tunnel exists", container->name);
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
		return BFX_RESULT_NOT_FOUND;
	}

	if (tunnel)
	{
		result = TunnelContainerSendFromTunnel(container, tunnel, packages, inoutCount);
		BDT_SOCKET_TUNNEL_RELEASE(tunnel);
		return result;
	}

	// if no tunnel exists and build flag not set
	// ignore send
	if (!(sendParams->flags & BDT_TUNNEL_SEND_FLAGS_BUILD))
	{
		BLOG_DEBUG("%s ignore BdtTunnel_Send for no tunnel exists", container->name);
		return BFX_RESULT_NOT_FOUND;
	}

	// not tunnel exists and build flag set
	// use build params to build tunnel
	TunnelBuilder* builder = NULL;
	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder)
		{
			// todo: need a send buffer?
		}
		else
		{
			builder = container->builder = TunnelBuilderCreate(
				container,
				&container->config->builder,
				container->framework,
				container->tls,
				container->peerFinder,
				container->keyManager,
				container->netManager,
				container->snClient);
			Bdt_TunnelBuilderAddRef(builder);
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (!builder)
	{
		BLOG_DEBUG("%s ignore BdtTunnel_Send for builder's building", container->name);
		result = BFX_RESULT_PENDING;
	}
		
	BLOG_DEBUG("%s create create builder in BdtTunnel_Send", container->name);
	result = TunnelBuilderBuildForConnectTunnel(
		builder, 
		packages, 
		inoutCount, 
		sendParams->buildParams);
	Bdt_TunnelBuilderRelease(builder);

	return result;
}

static uint32_t AddWaitingBuilderConnection(Bdt_TunnelContainer* container, BdtConnection* connection)
{
	BFX_THREADLOCK_CHECK_CURRENT_THREAD_OWNED(&container->builderLock);
	WaitingBuilderConnection* wbc = BFX_MALLOC_OBJ(WaitingBuilderConnection);
	memset(wbc, 0, sizeof(WaitingBuilderConnection));
	wbc->connection = connection;
	BdtAddRefConnection(connection);
	BfxListPushBack(&container->waitingConnections, (BfxListItem*)wbc);
	return BFX_RESULT_SUCCESS;
}

static uint8_t GetBestConnectorForConnection(Bdt_TunnelContainer* container, BdtEndpoint* local, BdtEndpoint* remote)
{
	bool hasTcp = false;
	bool hasUdp = false;
	Bdt_TunnelIterator* iter = NULL;

	// 1. if active tcp tunnel exists, use stream connector
	// 2. if passive tcp tunnel exists,  use stream connector
	// 3. if no tcp tunnel exists but udp tunnel exists, use package connector
	// 4. if no tunnel exists, create a tunnel builder and use builder connector
	Bdt_SocketTunnel* tunnel = BdtTunnel_EnumTunnels(container, &iter);
	do
	{
		if (!tunnel)
		{
			break;
		}
		if (Bdt_SocketTunnelIsTcp(tunnel))
		{
			hasTcp = true;
			*local = *Bdt_SocketTunnelGetLocalEndpoint(tunnel);
			*remote = *Bdt_SocketTunnelGetRemoteEndpoint(tunnel);
			if (!Bdt_IsTcpEndpointPassive(local, remote))
			{
				break;
			}
		}
		else if (Bdt_SocketTunnelIsUdp(tunnel))
		{
			hasUdp = true;
		}
		tunnel = BdtTunnel_EnumTunnelsNext(iter);
	} while (false);
	BdtTunnel_EnumTunnelsFinish(iter);


	if (hasTcp)
	{
		BLOG_DEBUG("%s GetBestConnectorForConnection return stream connector", container->name);
		return BDT_CONNECTION_CONNECTOR_TYPE_STREAM;
	}
	else if (hasUdp)
	{
		BLOG_DEBUG("%s GetBestConnectorForConnection return package connector", container->name);
		return BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE;
	}
	BLOG_DEBUG("%s GetBestConnectorForConnection return unknown connector", container->name);
	return BDT_CONNECTION_CONNECTOR_TYPE_NONE;
}

// thread safe
uint32_t BdtTunnel_ConnectConnection(
	Bdt_TunnelContainer* container,
	BdtConnection* connection
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder)
		{
			// wait for current builder finish
			result = AddWaitingBuilderConnection(container, connection);
		}
		else
		{
			result = BFX_RESULT_NOT_FOUND;
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (result == BFX_RESULT_SUCCESS || result != BFX_RESULT_NOT_FOUND)
	{
		BLOG_DEBUG("connection %s waiting current builder finish", Bdt_ConnectionGetName(connection));
		return result;
	}
	
	//should always has remote const info in params
	if (!Bdt_ConnectionGetBuildParams(connection)->remoteConstInfo)
	{
		BLOG_DEBUG("connection %s connect failed for no remote const info set in build params", Bdt_ConnectionGetName(connection));
		return BFX_RESULT_INVALID_PARAM;
	}
	// set const info in params
	BdtPeerConstInfo* remoteConst = BFX_MALLOC_OBJ(BdtPeerConstInfo);
	*remoteConst = *Bdt_ConnectionGetBuildParams(connection)->remoteConstInfo;
	if (BfxAtomicCompareAndSwapPointer(&container->remoteConst, NULL, remoteConst))
	{
		BfxFree(remoteConst);
	}

	BdtEndpoint tcpLocal;
	BdtEndpoint tcpRemote;
	uint8_t connectorType = GetBestConnectorForConnection(container, &tcpLocal, &tcpRemote);
	if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
	{
		return Bdt_ConnectionConnectWithStream(connection, &tcpLocal, &tcpRemote);
	}
	else if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE)
	{
		return Bdt_ConnectionConnectWithPackage(connection);
	}
	
	TunnelBuilder* builder = NULL;
	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder)
		{
			// wait for current builder finish
			result = AddWaitingBuilderConnection(container, connection);
		}
		else
		{
			builder = container->builder = TunnelBuilderCreate(
				container,
				&container->config->builder,
				container->framework,
				container->tls,
				container->peerFinder,
				container->keyManager,
				container->netManager,
				container->snClient);
			Bdt_TunnelBuilderAddRef(builder);
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (!builder)
	{
		BLOG_DEBUG("connection %s waiting current builder finish", Bdt_ConnectionGetName(connection));
		return result;
	}
	BLOG_DEBUG("%s create builder for connect connection %s", container->name, Bdt_ConnectionGetName(connection));
	result = Bdt_ConnectionConnectWithBuilder(connection, builder);
	Bdt_TunnelBuilderRelease(builder);
	return result;
}


static uint32_t TunnelContainerResetBuilder(
	Bdt_TunnelContainer* container,
	TunnelBuilder* oldBuilder)
{
	bool release = false;
	BfxList waitingConnections;
	BfxListInit(&waitingConnections);
	SocketTunnel* defaultTunnel = TunnelContainerUpdateDefaultTunnel(container);
	BDT_SOCKET_TUNNEL_RELEASE(defaultTunnel);

	BdtEndpoint tcpLocal;
	BdtEndpoint tcpRemote;
	uint8_t connectorType = GetBestConnectorForConnection(container, &tcpLocal, &tcpRemote);
	TunnelBuilder* builder = NULL;

	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder == oldBuilder)
		{
			release = true;
			container->builder = NULL;
			if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_NONE)
			{
				BfxListItem* first = BfxListPopFront(&container->waitingConnections);
				if (first)
				{
					connectorType = BDT_CONNECTION_CONNECTOR_TYPE_BUILDER;
					BfxListPushBack(&waitingConnections, first);
					builder = container->builder = TunnelBuilderCreate(
						container,
						&container->config->builder,
						container->framework,
						container->tls,
						container->peerFinder,
						container->keyManager,
						container->netManager,
						container->snClient);
					Bdt_TunnelBuilderAddRef(builder);
				}
			}
			else
			{
				BfxListSwap(&waitingConnections, &container->waitingConnections);
				BfxListClear(&container->waitingConnections);
			}
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);


	if (!release)
	{
		return BFX_RESULT_SUCCESS;
	}
	BLOG_DEBUG("%s finish builder %u", container->name, oldBuilder->seq);
	Bdt_TunnelBuilderRelease(oldBuilder);

	if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
	{
		WaitingBuilderConnection* wbc = (WaitingBuilderConnection*)BfxListPopFront(&waitingConnections);
		while (wbc != NULL)
		{
			Bdt_ConnectionConnectWithStream(wbc->connection, &tcpLocal, &tcpRemote);
			BdtReleaseConnection(wbc->connection);
			BfxFree(wbc);
			wbc = (WaitingBuilderConnection*)BfxListPopFront(&waitingConnections);
		}
	}
	else if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE)
	{
		WaitingBuilderConnection* wbc = (WaitingBuilderConnection*)BfxListPopFront(&waitingConnections);
		while (wbc != NULL)
		{
			Bdt_ConnectionConnectWithPackage(wbc->connection);
			BdtReleaseConnection(wbc->connection);
			BfxFree(wbc);
			wbc = (WaitingBuilderConnection*)BfxListPopFront(&waitingConnections);
		}
	}
	else if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
	{
		WaitingBuilderConnection* wbc = (WaitingBuilderConnection*)BfxListFirst(&waitingConnections);
		if (wbc)
		{
			BLOG_DEBUG("%s create builder for connect connection %s", container->name, Bdt_ConnectionGetName(wbc->connection));
			Bdt_ConnectionConnectWithBuilder(wbc->connection, builder);
			Bdt_TunnelBuilderRelease(builder);
			BdtReleaseConnection(wbc->connection);
			BfxFree(wbc);
		}
	}
	return BFX_RESULT_SUCCESS;
}


uint32_t Bdt_TunnelContainerCancelBuild(
	Bdt_TunnelContainer* container,
	Bdt_TunnelBuilder* builder
)
{
	TunnelBuilderFinishWithRuntime(builder, NULL);
	return TunnelContainerResetBuilder(container, builder);
}

uint32_t BdtTunnel_GetBestTcpReverseConnectEndpoint(
	Bdt_TunnelContainer* container,
	const BdtEndpoint* remote,
	BdtEndpoint* local
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_TunnelIterator* iter = NULL;
	Bdt_SocketTunnel* tunnel = BdtTunnel_EnumTunnels(container, &iter);
	do
	{
		if (!tunnel)
		{
			result = BFX_RESULT_NOT_FOUND;
			break;
		}
		if (Bdt_SocketTunnelIsTcp(tunnel))
		{
			if (!BdtEndpointCompare(Bdt_SocketTunnelGetRemoteEndpoint(tunnel), remote, false))
			{
				*local = *Bdt_SocketTunnelGetLocalEndpoint(tunnel);
			}
		}
		tunnel = BdtTunnel_EnumTunnelsNext(iter);
	} while (false);
	BdtTunnel_EnumTunnelsFinish(iter);

	if (result == BFX_RESULT_SUCCESS)
	{
		{
			char szLocal[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
			char szRemote[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
			BdtEndpointToString(remote, szRemote);
			BdtEndpointToString(local, szLocal);
			BLOG_DEBUG("%s GetBestTcpReverseConnectEndpoint return %s for %s because tcp tunnel exists", container->name, szLocal, szRemote);
		}
		
		return BFX_RESULT_SUCCESS;
	}

	// if no tcp tunnel to remote exists, simply try first bound local enpoint
	const Bdt_NetListener* listener = Bdt_NetManagerGetListener(container->netManager);
	size_t localEpListLen = 0;
	const BdtEndpoint* localEpList = Bdt_NetListenerGetBoundAddr(listener, &localEpListLen);
	assert(localEpListLen);
	*local = localEpList[0];
	Bdt_NetListenerRelease(listener);

	{
		char szLocal[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		char szRemote[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BdtEndpointToString(remote, szRemote);
		BdtEndpointToString(local, szLocal);
		BLOG_DEBUG("%s GetBestTcpReverseConnectEndpoint return default bound endpoint %s for %s ", container->name, szLocal, szRemote);
	}

	return BFX_RESULT_SUCCESS;
}


static uint32_t TunnelContainerBuildTunnelForAcceptConnection(
	TunnelContainer* container, 
	const uint8_t key[BDT_AES_KEY_LENGTH],
	const Bdt_SessionDataPackage* sessionData)
{
	BdtConnectionListener* listener = NULL;
	BdtConnection* connection = NULL;
	uint32_t result = Bdt_AddPassiveConnection(
		container->connectionManager,
		sessionData->synPart->toVPort,
		BdtTunnel_GetRemotePeerid(container),
		sessionData->synPart->fromSessionId,
		sessionData->synPart->synSeq,
		sessionData->payload,
		sessionData->payloadLength,
		&connection,
		&listener
	);

	TunnelBuilder* builder = TunnelBuilderCreate(
		container,
		&container->config->builder,
		container->framework,
		container->tls,
		container->peerFinder,
		container->keyManager,
		container->netManager,
		container->snClient
	);

	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder)
		{
			result = BFX_RESULT_ALREADY_EXISTS;
			break;
		}
		Bdt_TunnelBuilderAddRef(builder);
		container->builder = builder;
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);


	if (result == BFX_RESULT_SUCCESS)
	{
		// if add connection failed, connection is NULL
		// but builder should build without connection
		if (connection)
		{
			// if connection already exists, should also call Bdt_ConnectionAcceptWithBuilder to cover previous acceptor
			Bdt_ConnectionAcceptWithBuilder(connection, builder);
		}
	}
	else
	{
		// todo: how to ?
		// another builder exists
		// 1. if existing builder is passive, 
		//	means that 2 or more sn called package got from sn server before existing builder finish
		//		1.1	2nd sn called package has same sequence with 1st, usualy local peer send sn call package to more than one sn server, 
		//			or local peer retry building for same connection serveral times, 
		//			in this case, ignore this sn called package totally no problem
		//		1.2	2nd sn called package has different sequence with 1st, means 1st sn called package sent by connecting 1st connection, but failed,
		//			and then 2nd connection connect called caused another builder building for this connection; 
		//			or maybe local process exits before building finish and no history log written, 
		//			in this case, should cover builder with leatest sequence builder? 
		//			or wait existing builder finish? tcp stream can't got establish for local peer's connection instance doesn't exits, 
		//			that may cause a long lived mistake that no tcp tunnel can establish but acturely can; 
		//			we have to make sure reply all first ack tcp connection package even connecting connection doesn't exist to avoid this
		// 2. if existing builder is active,
		//		means that local and remote peer create active builder at just same time for connect connection or send package on tunnel, 
		//		in this case, we can ignore 2nd builder caused by getting sn called package, the reason is:
		//		2.1 if only udp tunnel exists between peers, both sides' builder will finish, 
		//			hole punched because both builders are sending syn tunnel package, and then session data reaches
		//		2.2 if tcp tunnel exits between peers, tcp interface can establish from at least one side, 
		//			connection can establish without builder, the other side's builder may fail, 
		//			but connection will use correct reverse stream connector in next retry
		BLOG_DEBUG("%s ignore builder %u to accept connection for builder exists", container->name, builder->seq);
	}

	Bdt_TunnelBuilderRelease(builder);

	if (listener)
	{
		result = Bdt_FirePreAcceptConnection(container->connectionManager, listener, connection);
		BdtConnectionListenerRelease(listener);
	}
	
	if (connection)
	{
		BdtReleaseConnection(connection);
	}
	
	return result;
}


static uint32_t TunnelContainerBuildTunnelForAcceptTunnel(
	TunnelContainer* container,
	const uint8_t key[BDT_AES_KEY_LENGTH],
	const Bdt_SynTunnelPackage* synPackage)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	TunnelBuilder* builder = TunnelBuilderCreate(
		container,
		&container->config->builder,
		container->framework,
		container->tls,
		container->peerFinder,
		container->keyManager,
		container->netManager,
		container->snClient
	);

	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder)
		{
			result = BFX_RESULT_ALREADY_EXISTS;
			break;
		}
		Bdt_TunnelBuilderAddRef(builder);
		container->builder = builder;
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		TunnelBuilderBuildForAcceptTunnel(builder, key, synPackage->seq);
	}
	else
	{
		// ignore builder for accept tunnel if builder exists, totally no problem
		BLOG_DEBUG("%s ignore builder %u to accept connection for builder exists", container->name, builder->seq);
	}
	
	Bdt_TunnelBuilderRelease(builder);

	return result;
}

static uint32_t TunnelContainerReverseConnectTcpTunnel(
	TunnelContainer* container, 
	const BdtEndpoint* remote, 
	uint32_t seq)
{
	BdtEndpoint local;
	uint32_t result = BdtTunnel_GetBestTcpReverseConnectEndpoint(container, remote, &local);
	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}
	TcpTunnel* tunnel = NULL;
	result = TunnelContainerCreateTunnel(container, &local, remote, 0, (SocketTunnel**)&tunnel);
	if (tunnel)
	{
		size_t packageCount = 0;
		TcpTunnelState state = BDT_TCP_TUNNEL_STATE_UNKNOWN;
		result = TcpTunnelConnect(tunnel, seq, NULL, &packageCount, &state);
	}
	return result;
}

void BdtTunnel_OnSnCalled(BdtStack* stack, const Bdt_SnCalledPackage* calledPackage)
{
	/*
	0.�ȸ���PeerInfo,�Լ�����ExchangeKey
	1.��Ҫ����Called�����Ƿ����ƶ�EndPointArray,�еĻ����Ǿ�ȷCalled
	2.��ȷCalled(Ŀǰֻ֧�ֶ�ȡ��һ��EndPoint)
		Called For Tunnel with XXX Package. ����UDP����Tunnel��TCP����Tunnel,����Tunnel��Ͷ�ݵ�һ��xxx package
			Resp AckTnnel with XXX Resp(�����Ҫ�Ļ�)
		Called For Connection. ����TCP����Connection (��ȷCalled For Connection ֻ����һ��Ŀ�ģ�)
			Resp AckConnection
	3.��ͨCalled
		Called with SynTunnel with SynConn: Build for Accept Connection
		Called with SynTunnel with Others: Build for Accept Tunnel,����Ͷ�ݵ�һ��Package
	 */
	BFX_BUFFER_HANDLE payload = calledPackage->payload;
	if (payload == NULL)
	{
		BLOG_DEBUG("ignore sn called for no payload");
		return;
	}

	Bdt_Package* resultPackages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	bool isEncrypto = true;
	bool hasExchange = false;
	uint8_t key[BDT_AES_KEY_LENGTH];
	BdtPeerid keyPeerid;
	size_t bytes;
	const uint8_t* data = BfxBufferGetData(payload, &bytes);
	uint32_t result = Bdt_UnboxUdpPackage(
		stack,
		data,
		bytes,
		resultPackages,
		BDT_MAX_PACKAGE_MERGE_LENGTH, 
		(Bdt_Package*)calledPackage, 
		key,
		&keyPeerid,
		&isEncrypto,
		&hasExchange);
	if (result)
	{
		BLOG_DEBUG("ignore sn called for unbox payload failed for %u", result);
		return;
	}

	// tofix: may not safe for ddos
	if (hasExchange)
	{
		BdtHistory_KeyManagerAddKey(
			BdtStackGetKeyManager(stack), 
			key, 
			&keyPeerid, 
			true);
	}
	
	const size_t firstIndex = hasExchange ? 1 : 0;
	const Bdt_Package* firstPackage = resultPackages[firstIndex];
	if (!firstPackage)
	{
		BLOG_DEBUG("ignore sn called for empty payload package");
		return;
	}

	result = BFX_RESULT_SUCCESS;
	Bdt_TunnelContainer* container = BdtTunnel_GetContainerByRemotePeerid(BdtStackGetTunnelManager(stack), &keyPeerid);
	if (!container)
	{
		container = BdtTunnel_CreateContainer(
			BdtStackGetTunnelManager(stack),
			&keyPeerid);
	}
	if (!container)
	{
		assert(false);
		return;
	}

	BdtPeerFinderAddCached(container->peerFinder, calledPackage->peerInfo, 0);
	BLOG_DEBUG("%s got sn called with 1st payload package %s", container->name, Bdt_PackageGetNameByCmdType(firstPackage->cmdtype));
	if (firstPackage->cmdtype == BDT_SYN_TUNNEL_PACKAGE)
	{
		const Bdt_Package* secondPackage = resultPackages[firstIndex + 1];
		if (secondPackage && secondPackage->cmdtype == BDT_SESSION_DATA_PACKAGE)
		{
			BLOG_DEBUG("%s got sn called with session data payload package", container->name);
			TunnelContainerBuildTunnelForAcceptConnection(
				container, 
				key, 
				(Bdt_SessionDataPackage*)secondPackage);
		}
		else
		{
			if (!calledPackage->reverseEndpointArray.size)
			{
				BLOG_DEBUG("%s got sn called with session payload package", container->name);
				TunnelContainerBuildTunnelForAcceptTunnel(
					container,
					key,
					(Bdt_SynTunnelPackage*)firstPackage);
			}
			else
			{
				BLOG_DEBUG("%s got sn called with tcp tunnel reverse connect request", container->name);
				TunnelContainerReverseConnectTcpTunnel(
					container,
					calledPackage->reverseEndpointArray.list,
					((Bdt_SynTunnelPackage*)firstPackage)->seq
				);
			}
			bool handled = false;
			BdtTunnel_PushSessionPackage(container, secondPackage, &handled);
		}
	}
	/*else if (firstPackage->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
	{
		BLOG_DEBUG("%s got sn called with tcp stream reverse connect request", container->name);
		Bdt_AddReverseConnectStreamConnection(
			BdtStackGetConnectionManager(stack), 
			(Bdt_TcpSynConnectionPackage*)firstPackage);
	}*/
	Bdt_TunnelContainerRelease(container);
}

static const Bdt_PackageWithRef* TunnelContainerGetCachedResponse(Bdt_TunnelContainer* container, uint32_t seq)
{
	const Bdt_PackageWithRef* resp = NULL;
	RespMap toFind;
	toFind.seq = seq;

	{
		BfxThreadLockLock(&container->respCacheLock);
		RespMap* foundNode = sglib_resp_map_find_member(container->respMap, &toFind);
		if (foundNode)
		{
			resp = foundNode->resp;
			if (resp)
			{
				Bdt_PackageAddRef(resp);
			}

			RespListItem* listItem = GET_RESP_LIST_ITEM_BY_MAP_NODE(foundNode);
			BfxListErase(&container->respList, &listItem->base);
			BfxListPushBack(&container->respList, &listItem->base);
		}
		BfxThreadLockUnlock(&container->respCacheLock);
	}
	return resp;
}


uint32_t BdtTunnel_SetCachedResponse(
	Bdt_TunnelContainer* container,
	uint32_t seq,
	const Bdt_PackageWithRef* resp
)
{
	BLOG_DEBUG("%s cached response to remote sequence %u", container->name, seq);
	const Bdt_PackageWithRef* toRelease = NULL;
	RespMap* foundNode = NULL;
	RespMap toFind;
	toFind.seq = seq;
	Bdt_PackageAddRef(resp);

	{
		BfxThreadLockLock(&container->respCacheLock);

		foundNode = sglib_resp_map_find_member(container->respMap, &toFind);
		if (foundNode != NULL)
		{
			toRelease = foundNode->resp;
			foundNode->resp = resp;

			RespListItem* listItem = GET_RESP_LIST_ITEM_BY_MAP_NODE(foundNode);
			BfxListErase(&container->respList, &listItem->base);
			BfxListPushBack(&container->respList, &listItem->base);
		}
		else
		{
			RespListItem* listItem = NULL;
			if (container->respList.count < container->config->history.respCacheSize)
			{
				listItem = BFX_MALLOC_OBJ(RespListItem);
				memset(listItem, 0, sizeof(RespListItem));
			}
			else
			{
				listItem = (RespListItem*)BfxListPopFront(&container->respList);
				sglib_resp_map_delete(&container->respMap, &listItem->node);
				toRelease = listItem->node.resp;
			}
			listItem->node.seq = seq;
			listItem->node.resp = resp;
			sglib_resp_map_add(&container->respMap, &listItem->node);
			BfxListPushBack(&container->respList, &listItem->base);
		}
		BfxThreadLockUnlock(&container->respCacheLock);
	}

	if (toRelease != NULL)
	{
		Bdt_PackageRelease(toRelease);
	}
	
	Bdt_TunnelBuilder* builder = NULL;
	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder 
			&& TunnelBuilderIsPassive(container->builder) 
			&& TunnelBuilderGetSeq(container->builder) == seq)
		{
			builder = container->builder;
			Bdt_TunnelBuilderAddRef(builder);
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (builder)
	{
		TunnelBuilderConfirm(builder, resp);
		Bdt_TunnelBuilderRelease(builder);
	}

	return BFX_RESULT_SUCCESS;
}


uint32_t BdtTunnel_SendFirstTcpPackage(
	Bdt_StackTls* tls,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package** packages,
	uint8_t count,
	const Bdt_TunnelEncryptOptions* encrypt)
{
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(tls);
	size_t encodeLen = 0;
	int result = BFX_RESULT_SUCCESS;

	if (encrypt->exchange)
	{
		result = Bdt_TcpInterfaceBoxFirstPackage(
			tcpInterface,
			packages,
			count,
			tlsData->tcpEncodeBuffer,
			sizeof(tlsData->tcpEncodeBuffer),
			&encodeLen,
			encrypt->remoteConst->publicKeyType,
			(uint8_t*)&encrypt->remoteConst->publicKey,
			encrypt->localSecret->secretLength,
			(uint8_t*)&encrypt->localSecret->secret
		);
	}
	else
	{
		result = Bdt_TcpInterfaceBoxFirstPackage(
			tcpInterface,
			packages,
			count,
			tlsData->tcpEncodeBuffer,
			sizeof(tlsData->tcpEncodeBuffer),
			&encodeLen, 
			0, 
			NULL,
			0, 
			NULL
		);
	}

	size_t sent = 0;

	BLOG_DEBUG("%s send first package length: %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen);

	uint32_t sendResult = Bdt_TcpInterfaceSend(tcpInterface, tlsData->tcpEncodeBuffer, encodeLen, &sent);
	
	if (sendResult || encodeLen != sent)
	{
		BLOG_DEBUG("%s send first package failed for initial send buffer smaller than %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen);
		// should not pending for this is first package and not large
		return BFX_RESULT_FAILED;
	}

	BLOG_DEBUG("%s send first package length: %zu sent: %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen, sent);
	return BFX_RESULT_SUCCESS;
}

uint32_t BdtTunnel_SendFirstTcpResp(
	Bdt_StackTls* tls,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package** packages,
	uint8_t count
)
{
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(tls);
	size_t encodeLen = 0;
	int result = BFX_RESULT_SUCCESS;

	result = Bdt_TcpInterfaceBoxPackage(
		tcpInterface,
		packages,
		count,
		tlsData->tcpEncodeBuffer,
		sizeof(tlsData->tcpEncodeBuffer),
		&encodeLen
	);
	
	size_t sent = 0;

	BLOG_DEBUG("%s send first resp length: %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen);

	uint32_t sendResult = Bdt_TcpInterfaceSend(tcpInterface, tlsData->tcpEncodeBuffer, encodeLen, &sent);

	if (sendResult || encodeLen != sent)
	{
		BLOG_DEBUG("%s send first resp failed for initial send buffer smaller than %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen);
		// should not pending for this is first package and not large
		return BFX_RESULT_FAILED;
	}

	BLOG_DEBUG("%s send first resp length: %zu sent: %zu", Bdt_TcpInterfaceGetName(tcpInterface), encodeLen, sent);

	return BFX_RESULT_SUCCESS;
}

static uint32_t TunnelContainerOnTcpTunnelReverseConnected(TunnelContainer* container, TcpTunnel* tunnel)
{
	Bdt_TunnelBuilder* builder = NULL;
	BfxThreadLockLock(&container->builderLock);
	do
	{
		if (container->builder
			&& !TunnelBuilderIsPassive(container->builder)
			&& !TunnelBuilderIsForConnection(container->builder))
		{
			builder = container->builder;
			Bdt_TunnelBuilderAddRef(builder);
		}
	} while (false);
	BfxThreadLockUnlock(&container->builderLock);

	if (builder)
	{
		TunnelBuilderOnTcpTunnelReverseConnected(builder, tunnel);
		Bdt_TunnelBuilderRelease(builder);
	}
	
	return BFX_RESULT_SUCCESS;
}

static SocketTunnel* TunnelContainerUpdateDefaultTunnel(TunnelContainer* container)
{
	BLOG_DEBUG("%s update default tunnel", container->name);
	
	// tofix: default tunnel strategy can be user defined

	// builder use strategy should fit default tunnel strategy 
	// for example, if default tunnel strategy prefers ipv6 udp tunnel, 
	//	builder should use a strategy that waits a ipv6 tunnel actived as long as possible
	

	// samplely prefers udp tunnel
	// if no udp tunnel exists, then use established tcp tunnel 
	Bdt_TunnelIterator* iter = NULL;
	Bdt_SocketTunnel* defaultTunnel = NULL;
	Bdt_SocketTunnel* firstTunnel = NULL;
	Bdt_SocketTunnel* secondTunnel = NULL;
	Bdt_SocketTunnel* lastTunnel = NULL;

	Bdt_SocketTunnel* tunnel = BdtTunnel_EnumTunnels(container, &iter);
	while (tunnel)
	{
		if (Bdt_SocketTunnelIsUdp(tunnel))
		{
			firstTunnel = tunnel;
			break;
		}
		else if (Bdt_TcpTunnelGetState((TcpTunnel*)tunnel) == BDT_TCP_TUNNEL_STATE_ALIVE)
		{
			secondTunnel = tunnel;
		}
		else if (!secondTunnel)
		{
			lastTunnel = tunnel;
		}
		tunnel = BdtTunnel_EnumTunnelsNext(iter);
	}
	
	defaultTunnel = firstTunnel ? firstTunnel : (secondTunnel ? secondTunnel : lastTunnel);
	if (defaultTunnel)
	{
		BDT_SOCKET_TUNNEL_ADDREF(defaultTunnel);
	}
	BdtTunnel_EnumTunnelsFinish(iter);


	TunnelContainerSetDefaultTunnel(container, defaultTunnel);
	return defaultTunnel;
}


static void TunnelContainerOnTunnelDead(
	TunnelContainer* container,
	Bdt_SocketTunnel* tunnel, 
	uint64_t lastRecv
)
{
	BLOG_DEBUG("%s dead", Bdt_SocketTunnelGetName(tunnel));
	if (BFX_RESULT_SUCCESS == TunnelContainerRemoveTunnel(container, tunnel, lastRecv))
	{
		SocketTunnel* defaultTunnel = TunnelContainerUpdateDefaultTunnel(container);
		BDT_SOCKET_TUNNEL_RELEASE(defaultTunnel);
	}
}


static void TunnelContainerOnHistoryLoad(
	Bdt_TunnelHistory* history,
	void* userData
)
{
	TunnelContainer* container = (TunnelContainer*)userData;
	const Bdt_NetListener* netListener = Bdt_NetManagerGetListener(container->netManager);

	// enum all local tcp endpoint: bound addresses and listening tcp ports
	size_t boundAddrLen = 0;
	const BdtEndpoint* boundAddr = Bdt_NetListenerGetBoundAddr(netListener, &boundAddrLen);
	
	size_t tcpListenerLen = 0;
	const Bdt_TcpListener* const* tcpListenerList = Bdt_NetListenerListTcpListener(netListener, &tcpListenerLen);

	size_t localTcpEpListLen = boundAddrLen + tcpListenerLen;
	BdtEndpoint* localTcpEpList = BFX_MALLOC_ARRAY(BdtEndpoint, localTcpEpListLen);
	for (size_t ix = 0; ix < boundAddrLen; ++ix)
	{
		BdtEndpoint* ep = localTcpEpList + ix;
		*ep = *(boundAddr + ix);
		ep->flag |= BDT_ENDPOINT_PROTOCOL_TCP;
		ep->port = 0;
	}
	for (size_t ix = 0; ix < tcpListenerLen; ++ix)
	{
		*(localTcpEpList + ix + boundAddrLen) = tcpListenerList[ix]->localEndpoint;
	}

	// restore tcp tunnel with logs
	Bdt_TunnelHistoryTcpLogForLocalEndpoint* tcpLogs = BFX_MALLOC_ARRAY(Bdt_TunnelHistoryTcpLogForLocalEndpoint, localTcpEpListLen);
	Bdt_TunnelHistoryGetTcpLog(container->history, localTcpEpList, tcpLogs, localTcpEpListLen);
	for (size_t ix = 0; ix < localTcpEpListLen; ++ix)
	{
		const BdtEndpoint* localEp = localTcpEpList + ix;
		Bdt_TunnelHistoryTcpLogForRemoteEndpointVector* localLogs = &(tcpLogs[ix].logForRemoteVector);
		for (size_t iy = 0; iy < localLogs->size; ++iy)
		{
			Bdt_TunnelHistoryTcpLogForRemoteEndpoint* remoteLogs = localLogs->logsForRemote + iy;
			// tofix: see earlier logs or use a log overtime?
			if (remoteLogs->logVector.size)
			{
				if (remoteLogs->logVector.logs[0].type == BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_HANDLE_OK)
				{
					SocketTunnel* tunnel = NULL;
					TunnelContainerCreateTunnel(
						container, 
						localEp, 
						&remoteLogs->remote, 
						remoteLogs->logVector.logs[0].timestamp, 
						&tunnel);
					BDT_SOCKET_TUNNEL_RELEASE(tunnel);
				}
			}
		}
	}
	BfxFree(localTcpEpList);
	Bdt_ReleaseTunnelHistoryTcpLogs(tcpLogs, localTcpEpListLen);
	BfxFree(tcpLogs);


	// enum all bound udp ports
	size_t udpInterfaceListLen = 0;
	const Bdt_UdpInterface* const* udpInterfaceList = Bdt_NetListenerListUdpInterface(netListener, &udpInterfaceListLen);
	BdtEndpoint* localUdpEpList = BFX_MALLOC_ARRAY(BdtEndpoint, udpInterfaceListLen);
	for (size_t ix = 0; ix < udpInterfaceListLen; ++ix)
	{
		*(localUdpEpList + ix) = *Bdt_UdpInterfaceGetLocal(udpInterfaceList[ix]);
	}
	// restore udp tunnel with logs
	uint64_t now = BfxTimeGetNow(false);
	Bdt_TunnelHistoryUdpLogForLocalEndpoint* udpLogs = BFX_MALLOC_ARRAY(Bdt_TunnelHistoryUdpLogForLocalEndpoint, udpInterfaceListLen);
	Bdt_TunnelHistoryGetUdpLog(container->history, localUdpEpList, udpLogs, udpInterfaceListLen);
	for (size_t ix = 0; ix < udpInterfaceListLen; ++ix)
	{
		const BdtEndpoint* localEp = localUdpEpList + ix;
		BdtHistory_PeerConnectVector* localLogs = &(udpLogs[ix].logForRemoteVector);
		for (size_t iy = 0; iy < localLogs->size; ++iy)
		{
			BdtHistory_PeerConnectInfo* remoteLog = *(localLogs->connections + iy);
			// if remote endpoint is static wan, ignore log time
			// if remote endpoint is not static wan, last recv time not earlier than ping over time
			if (remoteLog->endpoint.flag & BDT_ENDPOINT_FLAG_STATIC_WAN
				|| ((now - remoteLog->lastConnectTime) / 1000 < container->config->udpTunnel.pingOvertime))
			{
				SocketTunnel* tunnel = NULL;
				TunnelContainerCreateTunnel(
					container, 
					localEp, 
					&remoteLog->endpoint, 
					remoteLog->lastConnectTime, 
					&tunnel);
				BDT_SOCKET_TUNNEL_RELEASE(tunnel);
			}
		}
	}
	BfxFree(localUdpEpList);
	Bdt_ReleaseTunnelHistoryUdpLogs(udpLogs, udpInterfaceListLen);
	BfxFree(udpLogs);

	Bdt_NetListenerRelease(netListener);
	// at last update default tunnel
	SocketTunnel* defaultTunnel = TunnelContainerUpdateDefaultTunnel(container);
	BDT_SOCKET_TUNNEL_RELEASE(defaultTunnel);
}

#endif //__BDT_TUNNEL_CONTAINER_IMP_INL__
