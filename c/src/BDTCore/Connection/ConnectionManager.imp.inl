
#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include <SGLib/SGLib.h>
#include "./ConnectionManager.inl"
#include "./Connection.inl"
#include "../Stack.h"

typedef struct BdtRemoteSeqConnectionMap
{
	struct BdtRemoteSeqConnectionMap* left;
	struct BdtRemoteSeqConnectionMap* right;
	uint8_t color;
	const BdtPeerid* peerid;
	uint32_t seq;
	BdtConnection* connection;
} BdtRemoteSeqConnectionMap, remote_seq_map;
static int remote_seq_map_compare(const remote_seq_map* left, const remote_seq_map* right)
{
	int result = memcmp(&left->peerid, &right->peerid, BDT_PEERID_LENGTH);
	if (!result)
	{
		return result;
	}
	if (left->seq == right->seq)
	{
		return 0;
	}
	return left->seq > right->seq ? 1 : -1;
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(remote_seq_map, left, right, color, remote_seq_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(remote_seq_map, left, right, color, remote_seq_map_compare)

typedef struct BdtIdConnectionMap
{
	struct BdtIdConnectionMap* left;
	struct BdtIdConnectionMap* right;
	uint8_t color;
	uint32_t id;
	BdtConnection* connection;
} BdtIdConnectionMap, id_map;
static int id_map_compare(const id_map* left, const id_map* right)
{
	if (left->id == right->id)
	{
		return 0;
	}
	return left->id > right->id ? 1 : -1;
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(id_map, left, right, color, id_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(id_map, left, right, color, id_map_compare)

typedef struct AcceptRequestListItem
{
	BfxListItem base;
	BdtAcceptConnectionCallback callback;
	BfxUserData userData;
} AcceptRequestListItem;

typedef struct ConnectionListItem
{
	BfxListItem base;
	BdtConnection* connection;
} ConnectionListItem;

struct BdtConnectionListener
{
	int32_t refCount;
	BdtAcceptConnectionConfig config;
	uint16_t port;
	BfxThreadLock acceptLock;
	// acceptRequestsLock protected members begin
	// see we use a time out to determine if a connection in backlog is dropped, 
	// earlier connection is more easily removed from backlog
	BfxList backlog;
	BfxList acceptRequests;
	// acceptRequestsLock protected members finish
};
static BdtConnectionListener* BdtCreateConnectionListener(uint16_t port, const BdtAcceptConnectionConfig* config)
{
	BdtConnectionListener* listener = BFX_MALLOC_OBJ(BdtConnectionListener);
	memset(listener, 0, sizeof(BdtConnectionListener));
	listener->refCount = 1;
	listener->port = port;
	listener->config = *config;
	BfxThreadLockInit(&listener->acceptLock);
	BfxListInit(&listener->acceptRequests);
	BfxListInit(&listener->backlog);
	return listener;
}

int32_t BdtConnectionListenerAddRef(BdtConnectionListener* listener)
{
	return BfxAtomicInc32(&listener->refCount);
}

int32_t BdtConnectionListenerRelease(BdtConnectionListener* listener)
{
	int32_t refCount = BfxAtomicDec32(&listener->refCount);
	if (refCount <= 0)
	{
		//todo:
	}
	return refCount;
}

typedef struct BdtConnectionListenerMap
{
	struct BdtConnectionListenerMap* left;
	struct BdtConnectionListenerMap* right;
	uint8_t color;
	uint16_t port;
	BdtConnectionListener* listener;
}BdtConnectionListenerMap, connection_listener_map;
static int connection_listener_map_compare(const connection_listener_map* left, const connection_listener_map* right)
{
    if (left->port > right->port)
    {
        return 1;
    }

    if (left->port < right->port)
    {
        return -1;
    }

    return 0;
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(connection_listener_map, left, right, color, connection_listener_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(connection_listener_map, left, right, color, connection_listener_map_compare)

struct Bdt_ConnectionManager 
{
	BdtSystemFramework* framework;
	Bdt_StackTls* tls;
	Bdt_NetManager* netManager;
	BdtPeerFinder* peerFinder;
	BdtConnectionConfig connectionConfig;
	BdtAcceptConnectionConfig acceptConfig;
	Bdt_TunnelContainerManager* tunnelManager;
	Bdt_Uint32IdCreator* idCreator;

	BfxRwLock idLock;
	// idLock protected members begin
	BdtIdConnectionMap* idEntry;
	BdtRemoteSeqConnectionMap* passiveEntry;
	// idLock protected members finish

	BfxRwLock listenerMapLock;
	// listenerMapLock protected members begin
	BdtConnectionListenerMap* listenerMap;
	// listenerMapLock protected members finish
};

static uint32_t CallbackAccept(
	BdtConnection* connection,
	AcceptRequestListItem* item);

BdtConnection* Bdt_GetConnectionById(Bdt_ConnectionManager* manager, uint32_t id)
{
	BdtIdConnectionMap toFind;
	BdtConnection* connection = NULL;
	toFind.id = id;

	BfxRwLockRLock(&manager->idLock);
	BdtIdConnectionMap* found = sglib_id_map_find_member(manager->idEntry, &toFind);
	if (found)
	{
		connection = found->connection;
		BdtAddRefConnection(connection);
	}
	BfxRwLockRUnlock(&manager->idLock);

	return connection;
}

Bdt_ConnectionManager* Bdt_ConnectionManagerCreate(
	BdtSystemFramework* framework, 
	Bdt_StackTls* tls, 
	Bdt_NetManager* netManager,
	BdtPeerFinder* peerFinder,
	Bdt_TunnelContainerManager* tunnelManager,
	Bdt_Uint32IdCreator* idCreator,
	const BdtConnectionConfig* connectionConfig,
	const BdtAcceptConnectionConfig* acceptConfig)
{
	Bdt_ConnectionManager* manager = BFX_MALLOC_OBJ(Bdt_ConnectionManager);
	memset(manager, 0, sizeof(Bdt_ConnectionManager));
	manager->framework = framework;
	manager->tls = tls;
	manager->netManager = netManager;
	manager->peerFinder = peerFinder;
	manager->tunnelManager = tunnelManager;
	manager->framework = framework;
	manager->idCreator = idCreator;
	manager->connectionConfig = *connectionConfig;
	manager->acceptConfig = *acceptConfig;
	BfxRwLockInit(&manager->idLock);
	BfxRwLockInit(&manager->listenerMapLock);
	
	return manager;
}

static void ConnectionManagerUninit(Bdt_ConnectionManager* manager)
{

}

BdtConnection* Bdt_GetConnectionByRemoteSeq(
	Bdt_ConnectionManager* manager,
	const BdtPeerid* peerid,
	uint32_t seq)
{
	BdtRemoteSeqConnectionMap toFind;
	toFind.peerid = peerid;
	toFind.seq = seq;
	BdtConnection* connection = NULL;

	BfxRwLockRLock(&manager->idLock);
	BdtRemoteSeqConnectionMap* found = sglib_remote_seq_map_find_member(manager->passiveEntry, &toFind);
	if (found)
	{
		connection = found->connection;
		BdtAddRefConnection(connection);
	}
	BfxRwLockRUnlock(&manager->idLock);

	return connection;
}

BdtConnection* Bdt_AddActiveConnection(
	Bdt_ConnectionManager* manager,
	const BdtPeerid* remote
)
{
	Bdt_TunnelContainer* tunnelContainer = BdtTunnel_GetContainerByRemotePeerid(manager->tunnelManager, remote);
	if (!tunnelContainer)
	{
		tunnelContainer = BdtTunnel_CreateContainer(manager->tunnelManager, remote);
	}

	BdtConnection* connection = ConnectionCreate(
		manager->framework, 
		manager->tls, 
		manager->netManager,
		manager->peerFinder,
		remote,
		tunnelContainer,
		manager, 
		&manager->connectionConfig,
		Bdt_Uint32IdCreatorNext(manager->idCreator),
		BdtTunnel_GetNextSeq(tunnelContainer), 
		BDT_UINT32_ID_INVALID_VALUE,
		0);
	Bdt_TunnelContainerRelease(tunnelContainer);

	BdtIdConnectionMap* toAddId = BFX_MALLOC_OBJ(BdtIdConnectionMap);
	toAddId->connection = connection;
	toAddId->id = Bdt_ConnectionGetId(connection);
	ConnectionListItem* toAddList = BFX_MALLOC_OBJ(ConnectionListItem);

	BfxRwLockWLock(&manager->idLock);
	do
	{
		sglib_id_map_add(&manager->idEntry, toAddId);
		ConnectionListItem* connItem = BFX_MALLOC_OBJ(ConnectionListItem);
		connItem->connection = connection;
		BdtAddRefConnection(connection);
	} while (false);
	BfxRwLockWUnlock(&manager->idLock);
	return connection;
}

 uint32_t Bdt_AddPassiveConnection(
	Bdt_ConnectionManager* manager,
	uint16_t port,
	const BdtPeerid* remote,
	uint32_t remoteId,
	uint32_t seq, 
	const uint8_t* firstQuestion, 
	size_t firstQuestionLen, 
	BdtConnection** outConnection, 
	BdtConnectionListener** outListener
)
{
	BdtConnectionListener* listener = NULL;

	do
	{
		BdtConnectionListenerMap toFind;

		toFind.port = port;
		BfxRwLockRLock(&manager->listenerMapLock);
		BdtConnectionListenerMap* found = sglib_connection_listener_map_find_member(manager->listenerMap, &toFind);
		if (found)
		{
			listener = found->listener;
			BdtConnectionListenerAddRef(listener);
		}
		BfxRwLockRUnlock(&manager->listenerMapLock);
	} while (false);
	
	if (!listener)
	{
		{
			char szRemote[BDT_PEERID_STRING_LENGTH + 1];
			BdtPeeridToString(remote, szRemote);
			BLOG_DEBUG("ignore passive connection from %s seq %u for port %u not listening", szRemote, seq, port);
		}
		
		return BFX_RESULT_NOT_FOUND;
	}
	// no lock compare, for backlog limit is not absolute
	if (listener->backlog.count > manager->acceptConfig.backlog)
	{
		{
			char szRemote[BDT_PEERID_STRING_LENGTH + 1];
			BdtPeeridToString(remote, szRemote);
			BLOG_DEBUG("ignore passive connection from %s seq %u for port %u out of backlog", szRemote, seq, port);
		}

		BdtConnectionListenerRelease(listener);
		return BFX_RESULT_OUTOFMEMORY;
	}
	
	Bdt_TunnelContainer* tunnelContainer = BdtTunnel_GetContainerByRemotePeerid(manager->tunnelManager, remote);
	if (!tunnelContainer)
	{
		tunnelContainer = BdtTunnel_CreateContainer(manager->tunnelManager, remote);
	}
	assert(tunnelContainer);
	BdtIdConnectionMap* toAddId = BFX_MALLOC_OBJ(BdtIdConnectionMap);
	BdtRemoteSeqConnectionMap* toAddPassive = BFX_MALLOC_OBJ(BdtRemoteSeqConnectionMap);
	toAddPassive->seq = seq;
	toAddPassive->peerid = remote;

	BdtRemoteSeqConnectionMap* found = NULL;
	BdtConnection* connection = NULL;
	BfxRwLockWLock(&manager->idLock);
	{
		sglib_remote_seq_map_add_if_not_member(&manager->passiveEntry, toAddPassive, &found);
		if (!found)
		{
			connection = ConnectionCreate(
				manager->framework,
				manager->tls,
				manager->netManager,
				manager->peerFinder,
				remote,
				tunnelContainer,
				manager, 
				&listener->config.connection,
				Bdt_Uint32IdCreatorNext(manager->idCreator),
				seq,
				remoteId,
				CONNECTION_CONNECT_FLAG_PASSIVE);
			toAddPassive->peerid = Bdt_ConnectionGetRemotePeerid(connection);
			toAddPassive->connection = connection;
			toAddId->id = Bdt_ConnectionGetId(connection);
			toAddId->connection = connection;
			sglib_id_map_add(&manager->idEntry, toAddId);
		}
		else
		{
			connection = found->connection;
		}
		BdtAddRefConnection(connection);
	}
	BfxRwLockWUnlock(&manager->idLock);

	Bdt_TunnelContainerRelease(tunnelContainer);
	*outConnection = connection;
	
	if (found)
	{
		BdtConnectionListenerRelease(listener);
		BFX_FREE(toAddPassive);
		BFX_FREE(toAddId);
		return BFX_RESULT_ALREADY_EXISTS;
	}
	*outListener = listener;
	
	ConnectionSetFirstQuestion(connection, firstQuestion, firstQuestionLen);
	return BFX_RESULT_SUCCESS;
}


uint32_t Bdt_RemoveConnection(
	Bdt_ConnectionManager* manager,
	BdtConnection* connection
)
{
	BLOG_DEBUG("remove %s from connection manager", connection->name);
	BdtIdConnectionMap toFindId;
	toFindId.id = Bdt_ConnectionGetId(connection);
	BdtIdConnectionMap* foundId = NULL;

	BdtRemoteSeqConnectionMap toFindSeq;
	toFindSeq.peerid = Bdt_ConnectionGetRemotePeerid(connection);
	toFindSeq.seq = Bdt_ConnectionGetConnectSeq(connection);
	BdtRemoteSeqConnectionMap* foundSeq = NULL;

	BfxRwLockWLock(&manager->idLock);
	do
	{
		sglib_id_map_delete_if_member(&manager->idEntry, &toFindId, &foundId);
		if (Bdt_ConnectionIsPassive(connection))
		{
			sglib_remote_seq_map_delete_if_member(&manager->passiveEntry, &toFindSeq, &foundSeq);
		}
	} while (false);
	BfxRwLockWUnlock(&manager->idLock);

	if (foundSeq)
	{
		BfxFree(foundSeq);
	}

	if (foundId)
	{
		BdtReleaseConnection(foundId->connection);
		BfxFree(foundId);
	}

	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_FirePreAcceptConnection(
	Bdt_ConnectionManager* manager,
	BdtConnectionListener* listener, 
	BdtConnection* connection)
{
	AcceptRequestListItem* request = NULL;
	BfxThreadLockLock(&listener->acceptLock);
	if (BfxListSize(&listener->acceptRequests))
	{
		request = (AcceptRequestListItem*)BfxListPopFront(&listener->acceptRequests);
	}
	else
	{
		ConnectionListItem* b = BFX_MALLOC_OBJ(ConnectionListItem);
		b->connection = connection;
		BdtAddRefConnection(connection);
		BfxListPushBack(&listener->backlog, (BfxListItem*)b);
	}
	BfxThreadLockUnlock(&listener->acceptLock);

	if (request)
	{
		CallbackAccept(connection, request);
	}
	else
	{
		BLOG_DEBUG("%s adding to backlog, wait accept call", Bdt_ConnectionGetName(connection));
	}
	return BFX_RESULT_SUCCESS;
}

static uint32_t CallbackAccept(
	BdtConnection* connection,
	AcceptRequestListItem* request)
{
	BLOG_DEBUG("%s fire pre accept", Bdt_ConnectionGetName(connection));
	size_t fqLen = 0;
	BFX_BUFFER_HANDLE fq = ConnectionGetFirstQuestion(connection);
	uint8_t* buffer = BfxBufferGetData(fq, &fqLen);
	request->callback(BFX_RESULT_SUCCESS, buffer, fqLen, connection, request->userData.userData);
	if (request->userData.lpfnReleaseUserData)
	{
		request->userData.lpfnReleaseUserData(request->userData.userData);
	}
	BfxBufferRelease(fq);
	BfxFree(request);
	return BFX_RESULT_SUCCESS;
}


BDT_API(uint32_t) BdtListenConnection(
	BdtStack* stack,
	uint16_t port,
	const BdtAcceptConnectionConfig* config
)
{
	BLOG_INFO("begin listen connection on port %u", port);
	Bdt_ConnectionManager* manager = BdtStackGetConnectionManager(stack);
	BdtConnectionListenerMap toFind;
	toFind.port = port;

	BfxRwLockWLock(&manager->listenerMapLock);
	BdtConnectionListenerMap* found = sglib_connection_listener_map_find_member(manager->listenerMap, &toFind);
	if (!found)
	{
		found = BFX_MALLOC_OBJ(BdtConnectionListenerMap);
		found->port = port;
		found->listener = BdtCreateConnectionListener(port, config ? config : &manager->acceptConfig);
		sglib_connection_listener_map_add(&manager->listenerMap, found);
	}
	BfxRwLockWUnlock(&manager->listenerMapLock);

	return BFX_RESULT_SUCCESS;
}

BDT_API(uint32_t) BdtAcceptConnection(
	BdtStack* stack,
	uint16_t port, 
	BdtAcceptConnectionCallback callback,
	const BfxUserData* userData
)
{
	if (!stack || !callback || !userData)
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	Bdt_ConnectionManager* manager = BdtStackGetConnectionManager(stack);

	BdtConnectionListenerMap toFind;
	BdtConnectionListener* listener = NULL;
	toFind.port = port;
	BfxRwLockRLock(&manager->listenerMapLock);
	BdtConnectionListenerMap* found = sglib_connection_listener_map_find_member(manager->listenerMap, &toFind);
	if (found)
	{
		listener = found->listener;
		BdtConnectionListenerAddRef(listener);
	}
	BfxRwLockRUnlock(&manager->listenerMapLock);
	if (!listener)
	{
		BLOG_DEBUG("ignore accept connection call for port %u is not listening", port);
		return BFX_RESULT_NOT_FOUND;
	}

	ConnectionListItem* item = NULL;
	AcceptRequestListItem* request = BFX_MALLOC_OBJ(AcceptRequestListItem);
	memset(request, 0, sizeof(AcceptRequestListItem));
	request->callback = callback;
	request->userData = *userData;

    if (userData->lpfnAddRefUserData != NULL && userData->userData != NULL)
    {
        userData->lpfnAddRefUserData(userData->userData);
    }

	BfxThreadLockLock(&listener->acceptLock);
	if (!BfxListSize(&listener->acceptRequests) && BfxListSize(&listener->backlog))
	{
		item = (ConnectionListItem*)BfxListPopFront(&listener->backlog);
	}
	else
	{
		BfxListPushBack(&listener->acceptRequests, (BfxListItem*)request);
	}
	BfxThreadLockUnlock(&listener->acceptLock);

	if (item)
	{
		CallbackAccept(item->connection, request);
		BdtReleaseConnection(item->connection);
		BFX_FREE(item);
	}
	else
	{
		BLOG_DEBUG("accept connection call on port %u pending", port);
	}
	BdtConnectionListenerRelease(listener);
	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_AddReverseConnectStreamConnection(
	Bdt_ConnectionManager* manager,
	const Bdt_TcpSynConnectionPackage* synPackage)
{
	BdtConnectionListener* listener = NULL;
	BdtConnection* connection = NULL;
	size_t fqLen;
	const uint8_t* fq = BfxBufferGetData(synPackage->payload, &fqLen);

	uint32_t addResult = Bdt_AddPassiveConnection(
		manager,
		synPackage->toVPort,
		&synPackage->fromPeerid,
		synPackage->fromSessionId,
		synPackage->seq, 
		fq, 
		fqLen, 
		&connection,
		&listener
	);
	if (listener)
	{
		BLOG_DEBUG("%s got reverse connect request, begin connecting", Bdt_ConnectionGetName(connection));
		Bdt_ConnectionAcceptWithReverseStream(connection, synPackage->reverseEndpointArray.list);
		uint32_t result = Bdt_FirePreAcceptConnection(manager, listener, connection);
		BdtConnectionListenerRelease(listener);
	}
	else
	{
		char szRemote[BDT_PEERID_STRING_LENGTH + 1];
		BdtPeeridToString(&synPackage->fromPeerid, szRemote);
		BLOG_DEBUG("ignore reverse connect request from %s seq %u for %u", szRemote, synPackage->seq, addResult);
	}

	if (connection)
	{
		BdtReleaseConnection(connection);
	}

	return BFX_RESULT_SUCCESS;
}


uint32_t Bdt_ConnectionManagerPushPackage(
	Bdt_ConnectionManager* manager,
	const Bdt_Package* package,
	Bdt_TunnelContainer* tunnelContainer,
	bool* outHandled)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	if (package->cmdtype == BDT_SESSION_DATA_PACKAGE)
	{
		BdtConnection* connection = NULL;
		BdtConnectionListener* listener = NULL;
		Bdt_SessionDataPackage* sessionData = (Bdt_SessionDataPackage*)package;
		if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN
			&& !(sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK))
		{
			connection = Bdt_GetConnectionByRemoteSeq(
				manager,
				BdtTunnel_GetRemotePeerid(tunnelContainer),
				sessionData->synPart->synSeq);
			if (!connection)
			{
				result = Bdt_AddPassiveConnection(
					manager,
					sessionData->synPart->toVPort,
					BdtTunnel_GetRemotePeerid(tunnelContainer),
					sessionData->synPart->fromSessionId,
					sessionData->synPart->synSeq,
					sessionData->payload, 
					sessionData->payloadLength, 
					&connection,
					&listener
				);
			}
		}
		else
		{
			if ((sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN) && (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK))
			{
				connection = Bdt_GetConnectionById(manager, sessionData->toSessionId);
			}
			else
			{
				connection = Bdt_GetConnectionById(manager, sessionData->sessionId);
			}
		}
		if (connection)
		{
			result = Bdt_ConnectionPushPackage(connection, package, tunnelContainer, outHandled);
			if (listener)
			{
				result = Bdt_FirePreAcceptConnection(manager, listener, connection);
				BdtConnectionListenerRelease(listener);
			}
			BdtReleaseConnection(connection);
		}
		else
		{
			BLOG_DEBUG("connection manager ignore session data with syn flag for %u", result);
		}
	}
	else if (package->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
	{
		result = Bdt_AddReverseConnectStreamConnection(manager, (Bdt_TcpSynConnectionPackage*)package);
	}
	else
	{
		result = BFX_RESULT_INVALIDPARAM;
	}
	return result;
}