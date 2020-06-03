#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include <SGLib/SGLib.h>
#include "./Connection.inl"
#include "./ConnectionManager.h"
#include "./PackageConnection/PackageConnection.inl"
#include "./StreamConnection.inl"
#include "../Stack.h"
#include "../Global/Statistics.h"

struct BdtConnection
{
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	BdtSystemFramework* framework;
	Bdt_StackTls* tls;
	Bdt_NetManager* netManager;
	BdtPeerFinder* peerFinder;
	uint32_t id;
	uint32_t connectSeq;
	uint16_t connectPort;
	BdtBuildTunnelParams* buildParams;
	uint32_t remoteId;
	BdtPeerid remotePeerid;
	BdtConnectionConfig config;
	char* name;
	int32_t connectFlags;
	Bdt_TunnelContainer* tunnelContainer;
	Bdt_ConnectionManager* connectionManager;
	BFX_BUFFER_HANDLE firstQuestion;
	BFX_BUFFER_HANDLE firstAnswer;

	
	BfxRwLock stateLock;
	// stateLock protected members begin
	BdtConnectionState state;
	ConnectionConnector* connector;

	// assign acceptor from null to instance must before connection returned to application by Bdt_PreAcceptConnection call!!!
	ConnectionAcceptor* acceptor;

	BdtConnectionConnectCallback connectCallback;
	BfxUserData connectUserData;
	BdtConnectionFirstAnswerCallback faCallback;
	BfxUserData faUserData;

	// must not change after assigned 
	uint8_t providerType;
	union
	{
		PackageConnection* packageConnection;
		StreamConnection* streamConnection;
	} provider;
	// stateLock protected members end

	int32_t refCount;
};


static void BFX_STDCALL ConnectionAsUserDataAddRef(void* userData)
{
	BdtAddRefConnection((BdtConnection*)userData);
}

static void BFX_STDCALL ConnectionAsUserDataRelease(void* userData)
{
	BdtReleaseConnection((BdtConnection*)userData);
}

static void ConnectionAsUserData(BdtConnection* connection, BfxUserData* outUserData)
{
	outUserData->lpfnAddRefUserData = ConnectionAsUserDataAddRef;
	outUserData->lpfnReleaseUserData = ConnectionAsUserDataRelease;
	outUserData->userData = connection;
}

static Bdt_TcpInterfaceOwner* ConnectionAsTcpInterfaceOwner(BdtConnection* connection)
{
	return (Bdt_TcpInterfaceOwner*)connection;
}

static BdtConnection* ConnectionFromTcpInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	return (BdtConnection*)owner;
}

const char* Bdt_ConnectionGetName(const BdtConnection* connection)
{
	return connection->name;
}

const BdtPeerid* Bdt_ConnectionGetRemotePeerid(const BdtConnection* connection)
{
	return &(connection->remotePeerid);
}

uint32_t Bdt_ConnectionGetId(const BdtConnection* connection)
{
	return connection->id;
}

static uint32_t ConnectionGetRemoteId(const BdtConnection* connection)
{
	return connection->remoteId;
}

static BdtConnectionState ConnectionGetState(const BdtConnection* connection)
{
	return connection->state;
}

uint32_t Bdt_ConnectionGetConnectSeq(const BdtConnection* connection)
{
	return connection->connectSeq;
}

uint16_t Bdt_ConnectionGetConnectPort(const BdtConnection* connection)
{
	return connection->connectPort;
}

const Bdt_PackageWithRef* Bdt_ConnectionGenSynPackage(BdtConnection* connection)
{
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_SESSION_DATA_PACKAGE);
	Bdt_SessionDataPackage* sessionData = (Bdt_SessionDataPackage*)Bdt_PackageWithRefGet(pkg);
	sessionData->cmdflags = BDT_SESSIONDATA_PACKAGE_FLAG_SYN;
	sessionData->sessionId = BDT_UINT32_ID_INVALID_VALUE;
	size_t datalen = 0;

	BFX_BUFFER_HANDLE fq = connection->firstQuestion;
	Bdt_SessionDataSynPart* synPart = (Bdt_SessionDataSynPart*)BfxMalloc(sizeof(Bdt_SessionDataSynPart) + BfxBufferGetLength(fq));
	synPart->synControl = 0;
	synPart->ccType = 0;
	synPart->fromSessionId = connection->id;
	synPart->toVPort = connection->connectPort;
	synPart->synSeq = connection->connectSeq;
	sessionData->synPart = synPart;
	
	BfxBufferRead(fq, 0, BfxBufferGetLength(fq), (uint8_t*)(synPart + 1), 0);
	if (BfxBufferGetLength(fq))
	{
		sessionData->payload = (uint8_t*)(synPart + 1);
		assert(datalen < 0xffff);
		sessionData->payloadLength = (uint16_t)BfxBufferGetLength(fq);
	}
	else
	{
		sessionData->payload = NULL;
		sessionData->payloadLength = 0;
	}
	
	return pkg;
}

static void ConnectionSetFirstQuestion(BdtConnection* connection, const void* firstQuestion, size_t len)
{
	BFX_BUFFER_HANDLE cpy = BfxCreateBuffer(len);
	BfxBufferWrite(cpy, 0, len, firstQuestion, 0);
	if (NULL != BfxAtomicCompareAndSwapPointer(&connection->firstQuestion, NULL, cpy))
	{
		BfxBufferRelease(cpy);
	}
}

static BFX_BUFFER_HANDLE ConnectionGetFirstQuestion(const BdtConnection* connection)
{
	assert(connection->firstQuestion);
	BfxBufferAddRef(connection->firstQuestion);
	return connection->firstQuestion;
}

Bdt_TunnelContainer* Bdt_ConnectionGetTunnelContainer(const BdtConnection* connection)
{
	Bdt_TunnelContainerAddRef(connection->tunnelContainer);
	return connection->tunnelContainer;
}

const BdtConnectionConfig* Bdt_ConnectionGetConfig(const BdtConnection* connection)
{
	return &connection->config;
}

bool Bdt_ConnectionIsPassive(const BdtConnection* connection)
{
	return connection->connectFlags & CONNECTION_CONNECT_FLAG_PASSIVE;
}

bool Bdt_ConnectionIsConfirmed(const BdtConnection* connection)
{
	return !!connection->firstAnswer;
}

static BdtConnection* ConnectionCreate(
	BdtSystemFramework* framework, 
	Bdt_StackTls* tls, 
	Bdt_NetManager* netManager,
	BdtPeerFinder* peerFinder,
	const BdtPeerid* remote,
	Bdt_TunnelContainer* tunnelContainer,
	Bdt_ConnectionManager* connectionManager,
	const BdtConnectionConfig* config,
	uint32_t id,
	uint32_t connectSeq, 
	uint32_t remoteId, 
	int32_t flags
)
{
	BdtConnection* connection = BFX_MALLOC_OBJ(BdtConnection);
	memset(connection, 0, sizeof(BdtConnection));
	connection->refCount = 1;
	connection->framework = framework;
	connection->tls = tls;
	connection->peerFinder = peerFinder;
	connection->netManager = netManager;
	connection->tunnelContainer = tunnelContainer;
	Bdt_TunnelContainerAddRef(tunnelContainer);
	connection->connectionManager = connectionManager;
	connection->remotePeerid = *remote;
	connection->config = *config;
	connection->id = id;
	connection->connectSeq = connectSeq;
	connection->remoteId = remoteId;
	connection->state = BDT_CONNECTION_STATE_NONE;
	connection->connectFlags = flags;
	BfxRwLockInit(&connection->stateLock);
	
	ConnectionInitAsInterfaceOwner(connection);

	const char* prename = "BdtConnection:";
	size_t prenameLen = strlen(prename);
	size_t nameLen = prenameLen 
		+ BDT_PEERID_STRING_LENGTH + 1 /*seperator -*/
		+ 13/*max uint32_t dec string length*/ + 1 /*seperator -*/
		+ 13/*max uint32_t dec string length*/ + 1 /*terminate 0*/;
	connection->name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(connection->name, 0, nameLen);
	memcpy(connection->name, prename, prenameLen);
	BdtPeeridToString(remote, connection->name + prenameLen);
	sprintf(connection->name + prenameLen + BDT_PEERID_STRING_LENGTH, "-%u-%u", connection->id, connection->connectSeq);
		
	return connection;
}

static uint32_t ConnectionTryFinishConnectingWithError(
	BdtConnection* connection, 
	uint32_t error)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	ConnectionConnector* connector = NULL;
	ConnectionAcceptor* acceptor = NULL;
	BdtConnectionConnectCallback connectCallback = NULL;
	BfxUserData connectUserData;
	BfxUserData faUserData;
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		connection->state = BDT_CONNECTION_STATE_CLOSED;
		connector = connection->connector;
		connection->connector = NULL;
		acceptor = connection->acceptor;
		connection->acceptor = NULL;


		connectCallback = connection->connectCallback;
		connection->connectCallback = NULL;
		connectUserData = connection->connectUserData;
		connection->connectUserData.lpfnAddRefUserData = NULL;
		connection->connectUserData.lpfnReleaseUserData = NULL;
		connection->connectUserData.userData = NULL;

		connection->faCallback = NULL;
		faUserData = connection->faUserData;
		connection->faUserData.userData = NULL;
		connection->faUserData.lpfnAddRefUserData = NULL;
		connection->faUserData.lpfnReleaseUserData = NULL;
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

	Bdt_RemoveConnection(connection->connectionManager, connection);
	bool tcpOnly = false;
	BdtEndpoint local;
	BdtEndpoint remote;
	if (connector)
	{
		if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
		{
			local = *Bdt_TcpInterfaceGetLocal(connector->ins.stream.tcpInterface);
			remote = *Bdt_TcpInterfaceGetRemote(connector->ins.stream.tcpInterface);
			tcpOnly = true;
		}
		else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM)
		{
			local = connector->ins.reverseStream.local;
			remote = connector->ins.reverseStream.remote;
			tcpOnly = true;
		}
		ConnectionResetConnector(connection, connector);
	}
	else if (acceptor)
	{
		if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
		{
			local = *Bdt_TcpInterfaceGetLocal(acceptor->ins.reverseStream.tcpInterface);
			remote = *Bdt_TcpInterfaceGetRemote(acceptor->ins.reverseStream.tcpInterface);
			tcpOnly = true;
		}
		ConnectionResetAcceptor(connection, acceptor);
	}
	
	if (tcpOnly)
	{
		Bdt_TcpTunnel* tunnel = (Bdt_TcpTunnel*)BdtTunnel_GetTunnelByEndpoint(connection->tunnelContainer, &local, &remote);
		if (tunnel)
		{
			Bdt_TcpTunnelMarkDead(tunnel);
			Bdt_TcpTunnelRelease(tunnel);
		}
	}
	
    BdtStat_OnConnectionFailed(error, Bdt_ConnectionIsPassive(connection));

	ConnectionFireConnectResult(connection, error, connectCallback, &connectUserData);
	if (connectUserData.lpfnReleaseUserData)
	{
		connectUserData.lpfnReleaseUserData(connectUserData.userData);
	}
	if (faUserData.lpfnReleaseUserData)
	{
		faUserData.lpfnReleaseUserData(faUserData.userData);
	}
	Bdt_RemoveConnection(connection->connectionManager, connection);

	return BFX_RESULT_SUCCESS;
}


uint32_t Bdt_ConnectionFinishConnectingWithError(BdtConnection* connection, uint32_t error)
{
	return ConnectionTryFinishConnectingWithError(
		connection,
		error
	);
}

static void BFX_STDCALL ConnectionOnConnectTimeout(
	BDT_SYSTEM_TIMER timer,
	void* userData
)
{
	BdtConnection* connection = (BdtConnection*)userData;
	uint32_t result = ConnectionTryFinishConnectingWithError(
		connection, 
		BFX_RESULT_TIMEOUT);
}

static void ConnectionStartConnectTimeout(BdtConnection* connection)
{
	BfxUserData ud;
	ConnectionAsUserData(connection, &ud);
	BdtCreateTimeout(connection->framework, ConnectionOnConnectTimeout, &ud, connection->config.connectTimeout);
}

BDT_API(uint32_t) BdtConnectionConnect(
	BdtConnection* connection, 
	uint16_t port,
	const BdtBuildTunnelParams* params,
	const uint8_t* firstQuestion,
	size_t len,
	BdtConnectionConnectCallback connectCallback,
	const BfxUserData* connectUserData, 
	BdtConnectionFirstAnswerCallback faCallback,
	const BfxUserData* faUserData)
{
    BdtStat_OnConnectionCreate();

	uint32_t result = ConnectionTryEnterConnectWaiting(
		connection, 
		connectCallback, 
		connectUserData, 
		faCallback, 
		faUserData);
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_DEBUG("connection %s connect failed for invalid state", connection->name);
		return result;
	}
	connection->connectPort = port;
	BLOG_DEBUG("connection %s begin connecting to port %u", connection->name, port);
	connection->buildParams = BFX_MALLOC_OBJ(BdtBuildTunnelParams);
	Bdt_BuildTunnelParamsClone(params, connection->buildParams);
	
	connection->firstQuestion = BfxCreateBuffer(len);
	BfxBufferWrite(connection->firstQuestion, 0, len, firstQuestion, 0);
	return BdtTunnel_ConnectConnection(connection->tunnelContainer, connection);
}

const BdtBuildTunnelParams* Bdt_ConnectionGetBuildParams(const BdtConnection* connection)
{
	return connection->buildParams;
}

static void ConnectionUninit(BdtConnection* connection)
{
	if (connection->state != BDT_CONNECTION_STATE_CLOSED)
	{
		assert(false);
		BLOG_ERROR("%s released but not closed!!!", connection->name);
		// may crashed later!!!
	}
	
	if (connection->name)
	{
		BFX_FREE(connection->name);
		connection->name = NULL;
	}
	if (connection->buildParams)
	{
		Bdt_BuildTunnelParamsUninit(connection->buildParams);
		BfxFree(connection->buildParams);
	}
	BfxRwLockDestroy(&connection->stateLock);
	Bdt_TunnelContainerRelease(connection->tunnelContainer);

	if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		PackageConnectionDestroy(connection->provider.packageConnection);
	}
	else if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		StreamConnectionDestroy(connection->provider.streamConnection);
	}
}

BDT_API(int32_t) BdtAddRefConnection(BdtConnection* connection)
{
	return BfxAtomicInc32(&connection->refCount);
}

BDT_API(int32_t) BdtReleaseConnection(BdtConnection* connection)
{
	int32_t refCount = BfxAtomicDec32(&connection->refCount);
	if (refCount <= 0)
	{
		ConnectionUninit(connection);
	}
	return refCount;
}

BDT_API(uint32_t) BdtConnectionClose(BdtConnection* connection)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;
	uint8_t providerType = BDT_CONNECTION_PROVIDER_TYPE_WAITING;
	ConnectionConnector* connector = NULL;
	ConnectionAcceptor* acceptor = NULL;

	BfxUserData userData = { NULL, NULL, NULL };
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		oldState = connection->state;
		if (oldState < BDT_CONNECTION_STATE_ESTABLISH)
		{
			connector = connection->connector;
			acceptor = connection->acceptor;
			connection->state = BDT_CONNECTION_STATE_CLOSED;

			connection->connectCallback = NULL;
			connection->faCallback = NULL;
			userData = connection->connectUserData;
			connection->connectUserData.lpfnAddRefUserData = NULL;
			connection->connectUserData.lpfnReleaseUserData = NULL;
			connection->connectUserData.userData = NULL;
		}
		if (oldState == BDT_CONNECTION_STATE_ESTABLISH)
		{
			connection->state = BDT_CONNECTION_STATE_CLOSING;
			providerType = connection->providerType;
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);
	
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_DEBUG("%s ignore close for invalid state", connection->name);
		return result;
	}
	
	if (oldState < BDT_CONNECTION_STATE_ESTABLISH)
	{
		if (userData.lpfnReleaseUserData)
		{
			userData.lpfnReleaseUserData(userData.userData);
		}

		if (connector)
		{
			ConnectionResetConnector(connection, connector);
		}
		else if (acceptor)
		{
			ConnectionResetAcceptor(connection, acceptor);
		}
		//tofix: may fire closed ?
		Bdt_RemoveConnection(connection->connectionManager, connection);
		return result;
	}

	if (providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		return PackageConnectionClose(connection->provider.packageConnection);
	}
	else if (providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		return StreamConnectionClose(connection->provider.streamConnection);
	}
	else
	{
		BLOG_DEBUG("%s enter closing and wait close when provider set to connection", connection->name);
		return BFX_RESULT_SUCCESS;
	}
}


BDT_API(uint32_t) BdtConnectionReset(BdtConnection* connection)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;
	uint8_t providerType = BDT_CONNECTION_PROVIDER_TYPE_WAITING;
	ConnectionConnector* connector = NULL;
	ConnectionAcceptor* acceptor = NULL;
	BfxUserData userData = { NULL, NULL, NULL };

	BfxRwLockWLock(&connection->stateLock);
	do
	{
		oldState = connection->state;
		providerType = connection->providerType;
		if (oldState == BDT_CONNECTION_STATE_CLOSED)
		{
			result = BFX_RESULT_INVALID_STATE;
		}
		else if (oldState < BDT_CONNECTION_STATE_ESTABLISH)
		{
			connector = connection->connector;
			acceptor = connection->acceptor;

			connection->connectCallback = NULL;
			connection->faCallback = NULL;
			userData = connection->connectUserData;
			connection->connectUserData.lpfnAddRefUserData = NULL;
			connection->connectUserData.lpfnReleaseUserData = NULL;
			connection->connectUserData.userData = NULL;
		}
		connection->state = BDT_CONNECTION_STATE_CLOSED;
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_DEBUG("%s ignore reset for invalid state", connection->name);
		return result;
	}
	Bdt_RemoveConnection(connection->connectionManager, connection);

	if (oldState < BDT_CONNECTION_STATE_ESTABLISH)
	{
		if (userData.lpfnReleaseUserData)
		{
			userData.lpfnReleaseUserData(userData.userData);
		}

		if (connector)
		{
			ConnectionResetConnector(connection, connector);
		}
		else if (acceptor)
		{
			ConnectionResetAcceptor(connection, acceptor);
		}
		return result;
	}

	if (providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		return PackageConnectionReset(connection->provider.packageConnection);
	}
	else if (providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		return StreamConnectionReset(connection->provider.streamConnection);
	}
	else
	{
		BLOG_DEBUG("%s enter closed and wait reset when provider set to connection", connection->name);
		return BFX_RESULT_SUCCESS;
	}
}

static uint32_t ConnectionConfirmWithStream(BdtConnection* connection, Bdt_TcpInterface* tcpInterface)
{
	Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	uint8_t count = 0;

	uint32_t result = BFX_RESULT_SUCCESS;
	if (Bdt_IsTcpEndpointPassive(
		Bdt_TcpInterfaceGetLocal(tcpInterface),
		Bdt_TcpInterfaceGetRemote(tcpInterface)))
	{
		Bdt_ConnectionGenTcpAckPackage(connection, BDT_PACKAGE_RESULT_SUCCESS, NULL, packages, &count);

		result = BdtTunnel_SendFirstTcpResp(
			connection->tls,
			tcpInterface,
			packages,
			count);
	}
	else
	{
		Bdt_TunnelEncryptOptions encrypt;
		BdtTunnel_GetEnryptOptions(connection->tunnelContainer, &encrypt);
		Bdt_TcpInterfaceSetAesKey(tcpInterface, encrypt.key);
		Bdt_ConnectionGenTcpAckPackage(connection, BDT_PACKAGE_RESULT_SUCCESS, &encrypt, packages, &count);

		result = BdtTunnel_SendFirstTcpPackage(
			connection->tls,
			tcpInterface,
			packages,
			count,
			&encrypt);
	}
	
	for (uint8_t ix = 0; ix < count; ++ix)
	{
		Bdt_PackageFree(packages[ix]);
	}
	
	BLOG_DEBUG("connection %s send ack tcp connection with first answer with result %u", connection->name, result);
	return result;
}


BDT_API(uint32_t) BdtConnectionSend(
	BdtConnection* connection, 
	const uint8_t* buffer,
	size_t length,
	BdtConnectionSendCallback callback,
	const BfxUserData* userData)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockRLock(&connection->stateLock);
	if ((connection->state != BDT_CONNECTION_STATE_ESTABLISH
		&& connection->state != BDT_CONNECTION_STATE_CLOSING)
		|| connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_WAITING)
	{
		result = BFX_RESULT_INVALID_STATE;
	}
	BfxRwLockRUnlock(&connection->stateLock);
	
	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}
	if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		return StreamConnectionSend(connection->provider.streamConnection, buffer, length, callback, userData);
	}
	else if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		return PackageConnectionSend(connection->provider.packageConnection, buffer, length, callback, userData);
	}
	else
	{
		assert(false);
		BLOG_DEBUG("should not reach here");
		return BFX_RESULT_FAILED;
	}
}

BDT_API(uint32_t) BdtConnectionRecv(
	BdtConnection* connection,
	uint8_t* buffer,
	size_t length,
	BdtConnectionRecvCallback callback,
	const BfxUserData* userData)
{
    BLOG_DEBUG("%s connect post recv-opt len: %d", Bdt_ConnectionGetName(connection), (int)length);
    
    uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockRLock(&connection->stateLock);
	if ((connection->state != BDT_CONNECTION_STATE_ESTABLISH
		&& connection->state != BDT_CONNECTION_STATE_CLOSING)
		|| connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_WAITING)
	{
		result = BFX_RESULT_INVALID_STATE;
	}
	BfxRwLockRUnlock(&connection->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

	if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		return StreamConnectionRecv(connection->provider.streamConnection, buffer, length, callback, userData);
	}
	else if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		return PackageConnectionRecv(connection->provider.packageConnection, buffer, length, callback, userData);
	}
	else
	{
		assert(false);
		BLOG_DEBUG("should not reach here");
		return BFX_RESULT_FAILED;
	}
}

uint32_t Bdt_ConnectionFireFirstAnswer(BdtConnection* connection, uint32_t remoteId, const uint8_t* data, size_t len)
{
	BFX_BUFFER_HANDLE fa = BfxCreateBuffer(len);
	BfxBufferWrite(fa, 0, len, data, 0);
	if (NULL != BfxAtomicCompareAndSwapPointer(&connection->firstAnswer, NULL, fa))
	{
		BLOG_DEBUG("%s ignore fire first answer already fired", connection->name);
		BfxBufferRelease(fa);
		return BFX_RESULT_ALREADY_EXISTS;
	}
	connection->remoteId = remoteId;
	
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionFirstAnswerCallback faCallback = NULL;
	BfxUserData userData;
	BfxRwLockRLock(&connection->stateLock);
	do
	{
		faCallback = connection->faCallback;
		connection->faCallback = NULL;
		userData = connection->faUserData;
		connection->faUserData.userData = NULL;
		connection->faUserData.lpfnAddRefUserData = NULL;
		connection->faUserData.lpfnReleaseUserData = NULL;
	} while (false);
	BfxRwLockRUnlock(&connection->stateLock);

	if (faCallback)
	{
		faCallback(data, len, userData.userData);
	}
	if (userData.lpfnReleaseUserData)
	{
		userData.lpfnReleaseUserData(userData.userData);
	}
	
	BLOG_DEBUG("%s fire first answer", connection->name);
	return BFX_RESULT_SUCCESS;
}

static void ConnectionFireConnectResult(
	BdtConnection* connection,
	uint32_t result,
	BdtConnectionConnectCallback callback,
	const BfxUserData* userData)
{
	BLOG_DEBUG("connection %s fire connect result %u", connection->name, result);
	if (callback)
	{
		callback(connection, result, userData->userData);
	}
	else
	{
		// todo: reset it 
		// BdtConnectionReset(connection);
	}
}

static BFX_BUFFER_HANDLE ConnectionGetFirstAnswer(BdtConnection* connection)
{
	BFX_BUFFER_HANDLE fa = connection->firstAnswer;
	if (fa)
	{
		BfxBufferAddRef(fa);
	}
	return fa;
}

const Bdt_PackageWithRef* ConnectionGenAckPackage(
	BdtConnection* connection
)
{
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_SESSION_DATA_PACKAGE);
	Bdt_SessionDataPackage* ack = (Bdt_SessionDataPackage*)Bdt_PackageWithRefGet(pkg);
	Bdt_SessionDataPackageInit(ack);
	ack->cmdflags |= (BDT_SESSIONDATA_PACKAGE_FLAG_SYN | BDT_SESSIONDATA_PACKAGE_FLAG_ACK);
	size_t faLen = 0;
	const uint8_t* fa = BfxBufferGetData(connection->firstAnswer, &faLen);
	ack->payloadLength = (uint16_t)faLen;
	ack->synPart = (Bdt_SessionDataSynPart*)BfxMalloc(sizeof(Bdt_SessionDataSynPart) + ack->payloadLength);
	if (ack->payloadLength)
	{
		ack->payload = (uint8_t*)(ack->synPart + 1);
		memcpy(ack->payload, fa, ack->payloadLength);
	}

	ack->synPart->fromSessionId = connection->id;
	ack->synPart->synSeq = connection->connectSeq;
	ack->toSessionId = connection->remoteId;

	return pkg;
}

static uint32_t ConnectionTryEnterConnectWaiting(
	BdtConnection* connection,
	BdtConnectionConnectCallback connectCallback,
	const BfxUserData* connectUserData, 
	BdtConnectionFirstAnswerCallback faCallback,
	const BfxUserData* faUserData)
{
	if (connectUserData != NULL && connectUserData->lpfnAddRefUserData)
	{
		connectUserData->lpfnAddRefUserData(connectUserData->userData);
	}
	if (faUserData != NULL && faUserData->lpfnAddRefUserData)
	{
		faUserData->lpfnAddRefUserData(faUserData->userData);
	}

	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state != BDT_CONNECTION_STATE_NONE)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		connection->state = BDT_CONNECTION_STATE_CONNECTING;
		connection->connectCallback = connectCallback;
		connection->connectUserData = *connectUserData;
		connection->faCallback = faCallback;
        if (faUserData != NULL)
        {
            connection->faUserData = *faUserData;
        }
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		if (connectUserData != NULL && connectUserData->lpfnReleaseUserData)
		{
			connectUserData->lpfnReleaseUserData(connectUserData->userData);
		}
		if (faUserData != NULL && faUserData->lpfnReleaseUserData)
		{
			faUserData->lpfnReleaseUserData(faUserData->userData);
		}
	}

	BLOG_DEBUG("connection %s try enter connect waiting state with result %u", connection->name, result);

	return result;
}

static uint32_t ConnectionTryEnterConnecting(
	BdtConnection* connection, 
	ConnectionConnector* connector)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		connection->connector = connector;
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);
	ConnectionStartConnectTimeout(connection);
	BLOG_DEBUG("connection %s try enter connecting state with result %u", connection->name, result);
	return result;
}

uint32_t Bdt_ConnectionConnectWithBuilder(
	BdtConnection* connection, 
	Bdt_TunnelBuilder* builder)
{
	BLOG_DEBUG("connection %s try connect with builder", connection->name);

	ConnectionConnector* connector = BFX_MALLOC_OBJ(ConnectionConnector);
	memset(connector, 0, sizeof(ConnectionConnector));
	connector->type = BDT_CONNECTION_CONNECTOR_TYPE_BUILDER;
	connector->ins.builder = builder;
	Bdt_TunnelBuilderAddRef(builder);
	
	uint32_t result = ConnectionTryEnterConnecting(connection, connector);
	// must call Bdt_TunnelBuilderBuildForConnectConnection after set builder to connect!!!
	Bdt_TunnelBuilderBuildForConnectConnection(builder, connection);

	if (result != BFX_RESULT_SUCCESS)
	{
		ConnectionResetConnector(connection, connector);
		return result;
	}
	
	return BFX_RESULT_SUCCESS;
}


static uint32_t ConnectionSendTcpReverseSyn(BdtConnection* connection)
{
	const Bdt_PackageWithRef* synPackage = NULL;
	BfxRwLockRLock(&connection->stateLock);
	if (connection->connector)
	{
		synPackage = connection->connector->ins.reverseStream.synPackage;
		Bdt_PackageAddRef(synPackage);
	}
	BfxRwLockRUnlock(&connection->stateLock);
	
	if (!synPackage)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	size_t count = 1;
	const Bdt_Package* toSend = Bdt_PackageWithRefGet(synPackage);
	uint32_t result = BdtTunnel_Send(connection->tunnelContainer, &toSend, &count, NULL);
	Bdt_PackageRelease(synPackage);
	BLOG_DEBUG("%s send tcp reverse syn to remote,result=%d", connection->name, result);
	return result;
}

static void BFX_STDCALL ConnectionOnSendTcpReverseSynTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	BdtConnection* connection = userData;

	if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
	{
		BLOG_DEBUG("connection %s stop send tcp reverse timer for out of connecting state", connection->name);
		return;
	}
	ConnectionSendTcpReverseSyn(connection);
	BfxUserData udConnection;
	ConnectionAsUserData(connection, &udConnection);
	BdtCreateTimeout(
		connection->framework,
		ConnectionOnSendTcpReverseSynTimer,
		&udConnection,
		connection->config.package.resendInterval);
	BLOG_DEBUG("connection %s send tcp reverse syn to remote", connection->name);
}

uint32_t Bdt_ConnectionConnectWithStream(
	BdtConnection* connection, 
	const BdtEndpoint* local, 
	const BdtEndpoint* remote)
{
	{
		char szLocal[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		char szRemote[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BdtEndpointToString(local, szLocal);
		BdtEndpointToString(remote, szRemote);
		BLOG_DEBUG("connection %s begin connect with stream from %s to %s", connection->name, szLocal, szRemote);
	}
	

	uint32_t result = BFX_RESULT_SUCCESS;
	uint8_t connectType = Bdt_IsTcpEndpointPassive(local, remote) ? BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM : BDT_CONNECTION_CONNECTOR_TYPE_STREAM;
	ConnectionConnector* connector = BFX_MALLOC_OBJ(ConnectionConnector);
	memset(connector, 0, sizeof(ConnectionConnector));
	connector->type = connectType;
	
	if (connectType == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
	{
		Bdt_TcpInterface* tcpInterface = NULL;
		Bdt_NetManagerCreateTcpInterface(
			connection->netManager,
			local,
			remote,
			&tcpInterface);
		connector->ins.stream.tcpInterface = tcpInterface;
		Bdt_TcpInterfaceSetOwner(tcpInterface, ConnectionAsTcpInterfaceOwner(connection));
		result = ConnectionTryEnterConnecting(connection, connector);

		if (result == BFX_RESULT_SUCCESS)
		{
			Bdt_TcpInterfaceConnect(tcpInterface);
		}
		else
		{
			ConnectionResetConnector(connection, connector);
		}
	}
	else
	{
		Bdt_PackageWithRef* synPackage = NULL;
		Bdt_ConnectionGenTcpReverseSynPackage(connection, local, &synPackage);
		connector->ins.reverseStream.synPackage = synPackage;
		connector->ins.reverseStream.local = *local;
		connector->ins.reverseStream.local = *remote;

		size_t sendCount = 1;

		BdtTunnel_SendParams sendParams = {
			.flags = BDT_TUNNEL_SEND_FLAGS_BUILD,
			.buildParams = connection->buildParams
		};
		
		result = ConnectionTryEnterConnecting(connection, connector);

		if (result == BFX_RESULT_SUCCESS)
		{
			ConnectionSendTcpReverseSyn(connection);

			BfxUserData udConnection;
			ConnectionAsUserData(connection, &udConnection);
			BdtCreateTimeout(
				connection->framework,
				ConnectionOnSendTcpReverseSynTimer,
				&udConnection,
				connection->config.package.resendInterval);
		}
		else
		{
			ConnectionResetConnector(connection, connector);
		}
	}
	
	return result;
}


static uint32_t ConnectionOnSynSessionData(
	BdtConnection* connection,
	const Bdt_SessionDataPackage* sessionData
)
{
	BLOG_DEBUG("connection %s got session data with syn flag ", connection->name);
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore session data with syn flag for connection is not passive", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}

	// to block most reentry without lock
	if (connection->state != BDT_CONNECTION_STATE_NONE)
	{
		BLOG_DEBUG("connection %s ignore session data with syn flag for connection is not in initial state", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}
	
	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_SESSION_DATA_PACKAGE);
	

	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state == BDT_CONNECTION_STATE_NONE)
		{
			connection->state = BDT_CONNECTION_STATE_CONNECTING;
			connection->acceptor = BFX_MALLOC_OBJ(ConnectionAcceptor);
			memset(connection->acceptor, 0, sizeof(ConnectionAcceptor));
			connection->acceptor->type = CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER;
		}
		else if (connection->state == BDT_CONNECTION_STATE_CONNECTING)
		{
			// attension: should got responsed before route to connection, 
			//	for if connection confirmed, a session data package with ack and first answer has been set to response cache
			//	in tunnel container's implemention, if response cache hit, cached response used
			//	if connection hasn't confirmed, session data with syn should not be responsed for no first answer 
			result = BFX_RESULT_INVALID_STATE;
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);


	if (result == BFX_RESULT_SUCCESS)
	{
		ConnectionStartConnectTimeout(connection);
	}
	BLOG_DEBUG("connection %s try enter connecting for session data with syn flag got result %u", connection->name, result);
	return result;
}

static uint32_t ConnectionOnAckAckSessionData(
	BdtConnection* connection,
	const Bdt_SessionDataPackage* sessionData
)
{
	BLOG_DEBUG("connection %s got session data as ackack", connection->name);
	uint32_t result = BFX_RESULT_SUCCESS;
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore session data for connection is not passive", connection->name);
		result = BFX_RESULT_INVALID_STATE;
	}
	else
	{
		uint8_t acceptorType = CONNECTION_ACCEPTOR_TYPE_NULL;
		Bdt_TunnelBuilder* builder = NULL;
		BfxRwLockRLock(&connection->stateLock);
		do
		{
			if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			acceptorType = connection->acceptor->type;
			if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_BUILDER)
			{
				builder = connection->acceptor->ins.builder;
				Bdt_TunnelBuilderAddRef(builder);
			}
			else if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
			{
				if (connection->acceptor->ins.maybeBuilder.stream.tcpInterface)
				{
					result = BFX_RESULT_INVALID_STATE;
				}
			}
			else
			{
				result = BFX_RESULT_INVALID_STATE;
			}
		} while (false);
		BfxRwLockRUnlock(&connection->stateLock);

	
		if (result == BFX_RESULT_SUCCESS)
		{
			if (acceptorType == CONNECTION_ACCEPTOR_TYPE_BUILDER)
			{
				BLOG_DEBUG("connection %s push session data to builder", connection->name);
				result = Bdt_TunnelBuilderOnSessionData(builder, sessionData);
				Bdt_TunnelBuilderRelease(builder);
			}
			else if (acceptorType == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
			{
				result = Bdt_ConnectionTryEnterEstablish(connection);
				
				if (result == BFX_RESULT_SUCCESS)
				{
					result = Bdt_ConnectionEstablishWithPackage(connection, sessionData);
				}
			}
		}
		else
		{
			BLOG_DEBUG("connection %s ignore session data for checking state failed %u", connection->name, result);
		}
	}
	return result;
}

uint32_t Bdt_ConnectionAcceptWithBuilder(
	BdtConnection* connection, 
	Bdt_TunnelBuilder* builder)
{
	Bdt_TunnelBuilderAddRef(builder);
	// must call Bdt_TunnelBuilderBuildForAcceptConection before set builder to acceptor!!!
	Bdt_TunnelBuilderBuildForAcceptConnection(builder, connection);

	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;
	ConnectionAcceptor* oldAcceptor = NULL;

	uint32_t result = BFX_RESULT_SUCCESS;

	BfxRwLockWLock(&connection->stateLock);
	do
	{
		oldState = connection->state;
		if (connection->state == BDT_CONNECTION_STATE_NONE)
		{
			connection->state = BDT_CONNECTION_STATE_CONNECTING;
			connection->acceptor = BFX_MALLOC_OBJ(ConnectionAcceptor);
			memset(connection->acceptor, 0, sizeof(ConnectionAcceptor));
			connection->acceptor->type = CONNECTION_ACCEPTOR_TYPE_BUILDER;
			connection->acceptor->ins.builder = builder;
		}
		else if (connection->state == BDT_CONNECTION_STATE_CONNECTING)
		{
			if (connection->acceptor->type != CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			// for local tunnel builder building for connection may use a strategy that try package connection, stream connection, 
			//		and SnCall at same time; 
			// if a session data with syn flag received before SnCalled, 
			//		when SnCalled later received, should cover builder acceptor package, and call Bdt_TunnelBuilderPushPackage to sync builder's state 
			// if a tcp interface with TcpSynConnection as first package, 
			//		when SnCalled later received, should ignore SnCalled and finish builder at once
			if (connection->acceptor->ins.maybeBuilder.stream.tcpInterface)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}

			oldAcceptor = connection->acceptor;
			ConnectionAcceptor* acceptor = BFX_MALLOC_OBJ(ConnectionAcceptor);
			memset(acceptor, 0, sizeof(ConnectionAcceptor));
			acceptor->type = CONNECTION_ACCEPTOR_TYPE_BUILDER;
			acceptor->ins.builder = builder;
			connection->acceptor = acceptor;
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		if (oldState == BDT_CONNECTION_STATE_NONE)
		{
			ConnectionStartConnectTimeout(connection);
		}
		else if (oldState == BDT_CONNECTION_STATE_CONNECTING)
		{
			ConnectionResetAcceptor(connection, oldAcceptor);
		}
		else
		{
			BLOG_ERROR("should not reach here");
			assert(false);
		}
	}
	else
	{
		Bdt_TunnelContainerCancelBuild(connection->tunnelContainer, builder);
		Bdt_TunnelBuilderRelease(builder);
	}
	BLOG_DEBUG("connection %s try accepting with builder with result %u", connection->name, result);
	return result;
}

uint32_t Bdt_ConnectionAcceptWithReverseStream(BdtConnection* connection, const BdtEndpoint* remote)
{
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore accept reverse stream for not passive", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}
	// to block most reentry
	if (connection->state != BDT_CONNECTION_STATE_NONE)
	{
		BLOG_DEBUG("connection %s ignore accept reverse stream for in state %u", connection->name, connection->state);
		return BFX_RESULT_INVALID_STATE;
	}

	BdtEndpoint local;
	Bdt_TcpInterface* tcpInterface = NULL;
	BdtTunnel_GetBestTcpReverseConnectEndpoint(connection->tunnelContainer, remote, &local);
	Bdt_NetManagerCreateTcpInterface(
		connection->netManager,
		&local,
		remote,
		&tcpInterface);

	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state == BDT_CONNECTION_STATE_NONE)
		{
			connection->state = BDT_CONNECTION_STATE_CONNECTING;
			connection->acceptor = BFX_MALLOC_OBJ(ConnectionAcceptor);
			memset(connection->acceptor, 0, sizeof(ConnectionAcceptor));
			connection->acceptor->type = CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM;
			connection->acceptor->ins.reverseStream.tcpInterface = tcpInterface;
			Bdt_TcpInterfaceAddRef(tcpInterface);
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		Bdt_TcpInterfaceRelease(tcpInterface);
		BLOG_DEBUG("connection %s ignore accept reverse stream for in state %u", connection->name, connection->state);
		return result;
	}
	
	ConnectionStartConnectTimeout(connection);
	
	Bdt_TcpInterfaceSetOwner(tcpInterface, ConnectionAsTcpInterfaceOwner(connection));
	result = Bdt_TcpInterfaceConnect(tcpInterface);
	Bdt_TcpInterfaceRelease(tcpInterface);

	{
		char szRemote[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BdtEndpointToString(remote, szRemote);
		BLOG_DEBUG("connection %s try to begin reverse connecting to remote endpoint %s with result %u", connection->name, szRemote, result);

		char szLocal[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
		BdtEndpointToString(&local, szLocal);
		BLOG_DEBUG("connection %s begin connect tcp interface from %s to %s with result %u", connection->name, szLocal, szRemote, result);
	}

	return result;
}

static uint32_t ConnectionOnAckSessionData(
	BdtConnection* connection,
	const Bdt_SessionDataPackage* sessionData
)
{
	BLOG_DEBUG("connection %s got session data with ack flag", connection->name)

	if (Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore session data with ack flag for connection is passive", connection->name)
		return BFX_RESULT_INVALID_STATE;
	}
	
	uint32_t result = BFX_RESULT_SUCCESS;

	Bdt_TunnelBuilder* builder = NULL;
	PackageConnection* packageConnection = NULL;
	BfxRwLockRLock(&connection->stateLock);
	do
	{
		if (connection->state == BDT_CONNECTION_STATE_CONNECTING)
		{
			if (connection->connector->type == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
			{
				builder = connection->connector->ins.builder;
				Bdt_TunnelBuilderAddRef(builder);
				break;
			}
			else if (connection->connector->type == BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE)
			{
				break;
			}
			else
			{
				result = BFX_RESULT_INVALID_STATE;
			}
		}
		else if (connection->state == BDT_CONNECTION_STATE_ESTABLISH)
		{
			if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
			{
				packageConnection = connection->provider.packageConnection;
			}
			else
			{
				result = BFX_RESULT_INVALID_STATE;
			}
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockRUnlock(&connection->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		if (builder)
		{
			result = Bdt_TunnelBuilderOnSessionData(builder, sessionData);
			BLOG_DEBUG("connection %s push session data with ack flag to builder with result %u", connection->name, result);
			Bdt_TunnelBuilderRelease(builder);
		}
		else
		{
			if (!packageConnection)
			{
				Bdt_ConnectionFireFirstAnswer(
					connection,
					sessionData->synPart->fromSessionId,
					sessionData->payload,
					sessionData->payloadLength);

				result = Bdt_ConnectionTryEnterEstablish(connection);
				if (result == BFX_RESULT_SUCCESS)
				{
					result = Bdt_ConnectionEstablishWithPackage(connection, sessionData);
				}
			}
			else
			{
				bool handled = true;
				result = PackageConnectionPushPackage(packageConnection, sessionData, &handled);
				BLOG_DEBUG("connection %s push session data with ack to package connection with result %u", connection->name, result);
			}
		}
	}
	else
	{
		BLOG_DEBUG("connection %s ignore session data with ack flag for state checking failed for %u", connection->name, result);
	}

	return result;
}

uint32_t Bdt_ConnectionPushPackage(
	BdtConnection* connection,
	const Bdt_Package* package,
	Bdt_TunnelContainer* tunnelContainer, 
	bool* outHandled)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	if (package->cmdtype == BDT_SESSION_DATA_PACKAGE)
	{
		const Bdt_SessionDataPackage* sessionData = (Bdt_SessionDataPackage*)package;
		//BLOG_DEBUG("recv package, streamPos=%llu, ackStreamPos=%llu, payloadlen=%d, packagetime=%llu", package->streamPos, package->ackStreamPos, package->payloadLength, package->sendTime);
		if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN)
		{
			//On ack
			if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK)
			{
				result = ConnectionOnAckSessionData(connection, sessionData);
			}
			else
			{
				result = ConnectionOnSynSessionData(connection, sessionData);
			}
		}
		else
		{
			if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE && connection->provider.packageConnection != NULL)
			{
				result = PackageConnectionPushPackage(connection->provider.packageConnection, sessionData, outHandled);
			}
			else
			{
				//maybe first session data without syn flag
				result = ConnectionOnAckAckSessionData(connection, sessionData);
			}
		}
	}

	return result;
}

static uint32_t ConnectionOnTcpFirstSynPackage(
	BdtConnection* connection,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_TcpSynConnectionPackage* firstPackage)
{
	BLOG_DEBUG("connection %s got passive tcp interface with first tcp syn connection", connection->name);

	uint32_t result = BFX_RESULT_SUCCESS;
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore passive tcp interface with first tcp syn connection for not passive", connection->name);
		result = BFX_RESULT_INVALID_STATE;
	}
	else
	{
		Bdt_TunnelBuilder* builder = NULL;
		BfxRwLockWLock(&connection->stateLock);
		do
		{
			if (connection->state == BDT_CONNECTION_STATE_NONE)
			{
				connection->state = BDT_CONNECTION_STATE_CONNECTING;
				connection->acceptor = BFX_MALLOC_OBJ(ConnectionAcceptor);
				memset(connection->acceptor, 0, sizeof(ConnectionAcceptor));
				connection->acceptor->type = CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER;
				connection->acceptor->ins.maybeBuilder.stream.tcpInterface = tcpInterface;
				Bdt_TcpInterfaceAddRef(tcpInterface);
			}
			else if (connection->state == BDT_CONNECTION_STATE_CONNECTING)
			{
				if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_BUILDER)
				{
					builder = connection->acceptor->ins.builder;
					Bdt_TunnelBuilderAddRef(builder);
				}
				else if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
				{
					if (connection->acceptor->ins.maybeBuilder.stream.tcpInterface)
					{
						result = BFX_RESULT_INVALID_STATE;
					}
					else
					{
						// for local tunnel builder building for connection may use a strategy that try package connection, stream connection, 
						//		and SnCall at same time; 
						// if a session data with syn flag received before a TcpSynConnection 
						//		when TcpSynConnection later received, should cover builder acceptor package
						connection->acceptor->ins.maybeBuilder.stream.tcpInterface = tcpInterface;
						Bdt_TcpInterfaceAddRef(tcpInterface);
					}
				}
				else
				{
					result = BFX_RESULT_INVALID_STATE;
				}
			}
			else
			{
				result = BFX_RESULT_INVALID_STATE;
			}
		} while (false);
		BfxRwLockWUnlock(&connection->stateLock);

		if (result == BFX_RESULT_SUCCESS)
		{
			if (builder)
			{
				result = Bdt_TunnelBuilderOnTcpFistPackage(builder, tcpInterface, (Bdt_Package*)firstPackage);
				BLOG_DEBUG("connection %s push tcp interface with first tcp syn connection to builder with result %u", connection->name, result);
				Bdt_TunnelBuilderRelease(builder);
			}
			else
			{
				ConnectionStartConnectTimeout(connection);
				// when TcpSynConnection is sent from stream connector, connection must be not confirmed at this time
				// when TcpSynConnection is sent from tunnel builder, a session data with first question may received before TcpSynConnection,
				//		that connection may be confirmed at this time, should send TcpAckConnection at once
				if (connection->firstAnswer)
				{
					result = Bdt_ConnectionTryEnterEstablish(connection);
					if (result == BFX_RESULT_SUCCESS)
					{
						result = ConnectionConfirmWithStream(connection, tcpInterface);
						result = Bdt_ConnectionEstablishWithStream(connection, tcpInterface);
					}
				}
				else
				{
					// do nothing, wait confirm
					BLOG_DEBUG("connection %s bind tcp interface but not confirmed, waiting confirm", connection->name);
				}
			}
		}
		else
		{
			BLOG_DEBUG("connection %s ignore passive tcp interface with first tcp syn connection for checking state failed %u", connection->name, result);
		}
	}

	return result;
}

static uint32_t ConnectionOnTcpFirstAckPackage(
	BdtConnection* connection,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_TcpAckConnectionPackage* firstPackage
)
{
	BLOG_DEBUG("connection %s got passive tcp interface with first tcp ack connection", connection->name);

	uint32_t result = BFX_RESULT_SUCCESS;
	if (Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore passive tcp interface with first tcp ack connection for connection is passive", connection->name);
		result = BFX_RESULT_INVALID_STATE;
	}
	else
	{
		uint8_t connectorType = 0;
		Bdt_TunnelBuilder* builder = NULL;
		BfxRwLockWLock(&connection->stateLock);
		do
		{
			if (connection->state == BDT_CONNECTION_STATE_CONNECTING
				&& connection->connector)
			{
				connectorType = connection->connector->type;
				if (connection->connector->type == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
				{
					builder = connection->connector->ins.builder;
					Bdt_TunnelBuilderAddRef(builder);
				}
				else if (connection->connector->type == BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM)
				{

				}
				else
				{
					result = BFX_RESULT_INVALID_STATE;
				}
			}
			else
			{
				result = BFX_RESULT_INVALID_STATE;
			}
		} while (false);
		BfxRwLockWUnlock(&connection->stateLock);

		if (result == BFX_RESULT_SUCCESS)
		{
			if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
			{
				result = Bdt_TunnelBuilderOnTcpFistPackage(builder, tcpInterface, (Bdt_Package*)firstPackage);
				BLOG_DEBUG("connection %s push passive tcp interface with first tcp ack connection to builder with result %u", connection->name, result);
				Bdt_TunnelBuilderRelease(builder);
			}
			else if (connectorType == BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM)
			{
				result = Bdt_ConnectionTryEnterEstablish(connection);
				if (result == BFX_RESULT_SUCCESS)
				{
					// send ackack, enter establish
					Bdt_TunnelEncryptOptions encrypt;
					BdtTunnel_GetEnryptOptions(connection->tunnelContainer, &encrypt);
					Bdt_Package* package = NULL;
					Bdt_ConnectionGenTcpAckAckPackage(connection, BDT_PACKAGE_RESULT_SUCCESS, &package);
					result = BdtTunnel_SendFirstTcpPackage(connection->tls, tcpInterface, &package, 1, &encrypt);
					Bdt_PackageFree(package);
					BLOG_DEBUG("connection %s send tcp ackack connection with result %u", connection->name, result);
				}
				result = Bdt_ConnectionEstablishWithStream(connection, tcpInterface);
			}
		}
		else
		{
			BLOG_DEBUG("connection %s ignore passive tcp interface with first tcp ack connection for checking state failed for %u", connection->name, result);
		}
	}

	return result;
}

uint32_t Bdt_ConnectionOnTcpFirstPackage(
	BdtConnection* connection, 
	Bdt_TcpInterface* tcpInterface, 
	const Bdt_Package* firstPackage)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	if (firstPackage->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
	{
		result = ConnectionOnTcpFirstSynPackage(connection, tcpInterface, (Bdt_TcpSynConnectionPackage*)firstPackage);
	}
	else if (firstPackage->cmdtype == BDT_TCP_ACK_CONNECTION_PACKAGE)
	{
		result = ConnectionOnTcpFirstAckPackage(connection, tcpInterface, (Bdt_TcpAckConnectionPackage*)firstPackage);
	}
	
	return result;
}

uint32_t Bdt_ConnectionEstablishWithStream(
	BdtConnection* connection, 
	Bdt_TcpInterface* tcpInterface)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;

	BdtConnectionConnectCallback callback;
	BfxUserData userData;

	BfxRwLockWLock(&connection->stateLock);
	do 
	{
		oldState = connection->state;
		connection->providerType = BDT_CONNECTION_PROVIDER_TYPE_STREAM;
		connection->provider.streamConnection = StreamConnectionCreate(
			connection->framework,
			connection->tls,
			connection,
			&connection->config.stream,
			tcpInterface);

		callback = connection->connectCallback;
		connection->connectCallback = NULL;
		userData = connection->connectUserData;
		connection->connectUserData.lpfnAddRefUserData = NULL;
		connection->connectUserData.lpfnReleaseUserData = NULL;
		connection->connectUserData.userData = NULL;
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	Bdt_TcpInterfaceState interfaceState = Bdt_TcpInterfaceGetState(tcpInterface);
	Bdt_TcpInterfaceSetState(tcpInterface, interfaceState, BDT_TCP_INTERFACE_STATE_STREAM);

	if (oldState == BDT_CONNECTION_STATE_ESTABLISH)
	{
        BdtStat_OnConnectionEstablish(false, Bdt_ConnectionIsPassive(connection));
        ConnectionFireConnectResult(connection, BFX_RESULT_SUCCESS, callback, &userData);
	}
	else if (oldState == BDT_CONNECTION_STATE_CLOSING)
	{
		StreamConnectionClose(connection->provider.streamConnection);
	}
	else if (oldState == BDT_CONNECTION_STATE_CLOSED)
	{
		StreamConnectionReset(connection->provider.streamConnection);
	}
	
	if (userData.lpfnReleaseUserData)
	{
		userData.lpfnReleaseUserData(userData.userData);
	}
	BLOG_DEBUG("connection %s try to establish with stream connection with result %u", connection->name, result);
	return result;
}


void Bdt_ConnectionGenTcpSynPackage(
	BdtConnection* connection,
	uint8_t result, 
	const Bdt_TunnelEncryptOptions* encrypt, 
	Bdt_Package* outPackages[BDT_MAX_PACKAGE_MERGE_LENGTH],
	uint8_t* outCount)
{
	
	if (encrypt->exchange)
	{
		Bdt_ExchangeKeyPackage* exchange = Bdt_ExchangeKeyPackageCreate();
		exchange->fromPeerid = *BdtTunnel_GetLocalPeerid(connection->tunnelContainer);
		exchange->seq = Bdt_ConnectionGetConnectSeq(connection);
		exchange->peerInfo = BdtPeerFinderGetLocalPeer(connection->peerFinder);
		outPackages[(*outCount)++] = (Bdt_Package*)exchange;
	}
	Bdt_TcpSynConnectionPackage* syn = Bdt_TcpSynConnectionPackageCreate();
	syn->toVPort = Bdt_ConnectionGetConnectPort(connection);
	syn->fromPeerid = *BdtTunnel_GetLocalPeerid(connection->tunnelContainer);
	syn->seq = Bdt_ConnectionGetConnectSeq(connection);
	syn->toPeerid = *Bdt_ConnectionGetRemotePeerid(connection);
	syn->fromSessionId = Bdt_ConnectionGetId(connection);
	syn->peerInfo = BdtPeerFinderGetLocalPeer(connection->peerFinder);
	syn->payload = connection->firstQuestion;
	BfxBufferAddRef(connection->firstQuestion);
	syn->result = result;
	outPackages[(*outCount)++] = (Bdt_Package*)syn;
}

void Bdt_ConnectionGenTcpReverseSynPackage(
	BdtConnection* connection,
	const BdtEndpoint* local,
	Bdt_PackageWithRef** outPackage
)
{
	Bdt_PackageWithRef* pkg = Bdt_PackageCreateWithRef(BDT_TCP_SYN_CONNECTION_PACKAGE);
	Bdt_TcpSynConnectionPackage* syn = (Bdt_TcpSynConnectionPackage*)Bdt_PackageWithRefGet(pkg);
	syn->toVPort = Bdt_ConnectionGetConnectPort(connection);
	syn->fromPeerid = *BdtTunnel_GetLocalPeerid(connection->tunnelContainer);
	syn->seq = Bdt_ConnectionGetConnectSeq(connection);
	syn->toPeerid = *Bdt_ConnectionGetRemotePeerid(connection);
	syn->fromSessionId = Bdt_ConnectionGetId(connection);
	syn->peerInfo = BdtPeerFinderGetLocalPeer(connection->peerFinder);
	syn->payload = connection->firstQuestion;
	BfxBufferAddRef(connection->firstQuestion);
	syn->result = BDT_PACKAGE_RESULT_SUCCESS;
	BdtEndpointArrayPush(&syn->reverseEndpointArray, local);

	*outPackage = pkg;
}

void Bdt_ConnectionGenTcpAckPackage(
	BdtConnection* connection,
	uint8_t result,
	const Bdt_TunnelEncryptOptions* encrypt,
	Bdt_Package* outPackages[BDT_MAX_PACKAGE_MERGE_LENGTH],
	uint8_t* outCount)
{
	if (encrypt && encrypt->exchange)
	{
		Bdt_ExchangeKeyPackage* exchange = Bdt_ExchangeKeyPackageCreate();
		exchange->fromPeerid = *BdtTunnel_GetLocalPeerid(connection->tunnelContainer);
		exchange->seq = Bdt_ConnectionGetConnectSeq(connection);
		exchange->peerInfo = BdtPeerFinderGetLocalPeer(connection->peerFinder);
		outPackages[(*outCount)++] = (Bdt_Package*)exchange;

		{
			BLOG_DEBUG("%s gen tcp ack connection with exchange", connection->name);
		}
		
	}
	else
	{
		BLOG_DEBUG("%s gen tcp ack connection without exchange", connection->name);
	}
	Bdt_TcpAckConnectionPackage* ack = Bdt_TcpAckConnectionPackageCreate();
	ack->peerInfo = BdtPeerFinderGetLocalPeer(connection->peerFinder);
	ack->toSessionId = connection->remoteId;
	ack->seq = Bdt_ConnectionGetConnectSeq(connection);
	ack->result = result;
	if (result == BDT_PACKAGE_RESULT_SUCCESS)
	{
		ack->payload = ConnectionGetFirstAnswer(connection);
	}
	outPackages[(*outCount)++] = (Bdt_Package*)ack;
}

void Bdt_ConnectionGenTcpAckAckPackage(
	BdtConnection* connection,
	uint8_t result,
	Bdt_Package** outPackage)
{
	Bdt_TcpAckAckConnectionPackage* ackack = Bdt_TcpAckAckConnectionPackageCreate();
	ackack->result = result;
	ackack->seq = Bdt_ConnectionGetConnectSeq(connection);
	*outPackage = (Bdt_Package*)ackack;
}

static int32_t ConnectAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	return BdtAddRefConnection(ConnectionFromTcpInterfaceOwner(owner));
}

static int32_t ConnectAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	return BdtReleaseConnection(ConnectionFromTcpInterfaceOwner(owner));
}

static uint32_t ConnectionOnTcpEstablish(
	Bdt_TcpInterfaceOwner* owner, 
	Bdt_TcpInterface* tcpInterface)
{
	BdtConnection* connection = ConnectionFromTcpInterfaceOwner(owner);
	BLOG_DEBUG("connection %s's active tcp interface established", connection->name);
	uint32_t result = BFX_RESULT_SUCCESS;
	if (!Bdt_ConnectionIsPassive(connection))
	{
		// check connecting state
		BfxRwLockRLock(&connection->stateLock);
		do
		{
			if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
			{
				result = BFX_RESULT_INVALID_STATE;
				break; 
			}
			if (connection->connector->type != BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			if (connection->connector->ins.stream.tcpInterface != tcpInterface)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
		} while (false);
		BfxRwLockRUnlock(&connection->stateLock);

		if (result == BFX_RESULT_SUCCESS)
		{
			// send syn and wait ack to enter establish
			Bdt_TunnelEncryptOptions encrypt;
			BdtTunnel_GetEnryptOptions(connection->tunnelContainer, &encrypt);
			Bdt_TcpInterfaceSetAesKey(tcpInterface, encrypt.key);
			Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
			uint8_t count = 0;
			Bdt_ConnectionGenTcpSynPackage(connection, BDT_PACKAGE_RESULT_SUCCESS, &encrypt, packages, &count);
			Bdt_TcpInterfaceSetState(tcpInterface, BDT_TCP_INTERFACE_STATE_ESTABLISH, BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);
			result = BdtTunnel_SendFirstTcpPackage(connection->tls, tcpInterface, packages, count, &encrypt);
			BLOG_DEBUG("connection %s send tcp syn connection with result %u", connection->name, result);
			for (uint8_t ix = 0; ix < count; ++ix)
			{
				Bdt_PackageFree(packages[ix]);
			}
		}
		else
		{
			BLOG_DEBUG("connection %s's ignore established tcp interface for checking state failed for %u", connection->name, result);
		}
	}
	else
	{
		bool confirmed = false;
		// if confirmed, send ack; if not confirmed, wait until BdtConnectionConfirm called
		BfxRwLockRLock(&connection->stateLock);
		do
		{
			if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			if (connection->acceptor->type != CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			if (connection->acceptor->ins.reverseStream.tcpInterface != tcpInterface)
			{
				result = BFX_RESULT_INVALID_STATE;
				break;
			}
			confirmed = connection->acceptor->ins.reverseStream.confirmed;
		} while (false);
		BfxRwLockRUnlock(&connection->stateLock);

		if (result == BFX_RESULT_SUCCESS)
		{
			if (confirmed)
			{
				result = ConnectionConfirmWithStream(connection, tcpInterface);
			}
			else
			{
				BLOG_DEBUG("connection %s bound tcp interface but not confirmed, waiting confirm", connection->name);
			}
		}
		else
		{
			BLOG_DEBUG("connection %s's ignore established tcp interface for checking state failed for %u", connection->name, result);
		}
	}

	return result;
}

static void ConnectionOnTcpError(
	Bdt_TcpInterfaceOwner* owner, 
	Bdt_TcpInterface* tcpInterface, 
	uint32_t error)
{
	BdtConnection* connection = ConnectionFromTcpInterfaceOwner(owner);
	BLOG_DEBUG("connection %s got tcp interface error %u", connection->name, error);
	uint32_t result = ConnectionTryFinishConnectingWithError(
		connection,
		error);
}

static uint32_t ConnectionOnTcpFirstAckResp(
	BdtConnection* connection, 
	Bdt_TcpInterface* tcpInterface, 
	const Bdt_TcpAckConnectionPackage* package)
{
	BLOG_DEBUG("connection %s got first response tcp ack connection", connection->name);
	if (Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore first response tcp ack connection for connection is passive", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}

	uint32_t result = BFX_RESULT_SUCCESS;
	
	// check connecting state
	BfxRwLockRLock(&connection->stateLock);
	do
	{
		if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		if (connection->connector->type != BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		if (connection->connector->ins.stream.tcpInterface != tcpInterface)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
	} while (false);
	BfxRwLockRUnlock(&connection->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		result = Bdt_ConnectionTryEnterEstablish(connection);
		if (result == BFX_RESULT_SUCCESS)
		{
			result = Bdt_ConnectionEstablishWithStream(connection, tcpInterface);
		}
	}
	else
	{
		BLOG_DEBUG("connection %s ignore first response tcp ack connection for checking state failed", connection->name, result);
	}

	BdtTunnel_OnTcpFirstResp(connection->tunnelContainer, NULL, tcpInterface, package->peerInfo);
	return result;
}


static uint32_t ConnectionOnTcpFirstAckAckResp(
	BdtConnection* connection,
	Bdt_TcpInterface* tcpInterface,
	const Bdt_TcpAckAckConnectionPackage* package)
{
	BLOG_DEBUG("connection %s got first response tcp ack ack connection", connection->name);
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore first response tcp ack ack connection for connection is not passive", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}

	uint32_t result = BFX_RESULT_SUCCESS;

	// check connecting state
	BfxRwLockRLock(&connection->stateLock);
	do
	{
		if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		if (connection->acceptor->type != CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		if (connection->acceptor->ins.reverseStream.tcpInterface != tcpInterface)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
	} while (false);
	BfxRwLockRUnlock(&connection->stateLock);

	if (result == BFX_RESULT_SUCCESS)
	{
		result = Bdt_ConnectionTryEnterEstablish(connection);
		if (result == BFX_RESULT_SUCCESS)
		{
			result = Bdt_ConnectionEstablishWithStream(connection, tcpInterface);
		}
	}
	else
	{
		BLOG_DEBUG("connection %s ignore first response tcp ack connection for checking state failed", connection->name, result);
	}

	// tofix: no remote info in ack ack tcp connection
	BdtTunnel_OnTcpFirstResp(connection->tunnelContainer, NULL, tcpInterface, NULL);
	return result;
}

static uint32_t ConnectionOnTcpFirstResp(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package* package)
{
	BdtConnection* connection = ConnectionFromTcpInterfaceOwner(owner);
	uint32_t result = BFX_RESULT_SUCCESS;
	
	if (package->cmdtype == BDT_TCP_ACK_CONNECTION_PACKAGE)
	{
		result = ConnectionOnTcpFirstAckResp(connection, tcpInterface, (Bdt_TcpAckConnectionPackage*)package);
	}
	else if (package->cmdtype == BDT_TCP_ACKACK_CONNECTION_PACKAGE)
	{
		result = ConnectionOnTcpFirstAckAckResp(connection, tcpInterface, (Bdt_TcpAckAckConnectionPackage*)package);
	}
	else
	{
		BLOG_DEBUG("connection %s ignore first tcp response for invalid package type %u", connection->name, package->cmdtype);
		result = BFX_RESULT_INVALID_PARAM;
	}
	return result;
}

static void ConnectionOnTcpClosed(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	BdtConnection* connection = ConnectionFromTcpInterfaceOwner(owner);
	BLOG_DEBUG("connection %s bound tcp inteface closed", connection->name);
	//tofix: handle tcp close event
}

static void ConnectionOnTcpLastEvent(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	BdtConnection* connection = ConnectionFromTcpInterfaceOwner(owner);
	BLOG_DEBUG("connection %s bound tcp inteface last event", connection->name);
	//tofix: handle tcp last event
}

static void ConnectionInitAsInterfaceOwner(BdtConnection* connection)
{
	connection->addRef = ConnectAsTcpInterfaceOwnerAddRef;
	connection->release = ConnectAsTcpInterfaceOwnerRelease;
	connection->onEstablish = ConnectionOnTcpEstablish;
	connection->onError = ConnectionOnTcpError;
	connection->onFirstResp = ConnectionOnTcpFirstResp;
	connection->onClosed = ConnectionOnTcpClosed;
	connection->onLastEvent = ConnectionOnTcpLastEvent;
}


static uint32_t ConnectionSendSyn(BdtConnection* connection)
{
	const Bdt_PackageWithRef* syn = NULL;

	BfxRwLockRLock(&connection->stateLock);
	if (connection->connector)
	{
		syn = connection->connector->ins.package.synPackage;
		Bdt_PackageAddRef(syn);
	}
	BfxRwLockRUnlock(&connection->stateLock);

	if (!syn)
	{
		BLOG_DEBUG("%s connector reset, ignore send syn", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}


	BdtTunnel_SendParams sendParams = {
		.flags = BDT_TUNNEL_SEND_FLAGS_UDP_ONLY
	};

	const Bdt_Package* toSend = Bdt_PackageWithRefGet(syn);
	size_t count = 1;
	uint32_t result = BdtTunnel_Send(
		connection->tunnelContainer, 
		&toSend, 
		&count, 
		&sendParams);
	Bdt_PackageRelease(syn);

	BLOG_DEBUG("%s send syn to remote,result=%d", connection->name, result);
	return result;
}

static void BFX_STDCALL ConnectionOnSendSynTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	BdtConnection* connection = userData;

	if (BFX_RESULT_SUCCESS == ConnectionSendSyn(connection))
	{

		BfxUserData udConnection;
		ConnectionAsUserData(connection, &udConnection);
		BdtCreateTimeout(
			connection->framework,
			ConnectionOnSendSynTimer,
			&udConnection,
			connection->config.package.resendInterval);
	}
}


static uint32_t ConnectionSendAck(BdtConnection* connection)
{
	const Bdt_PackageWithRef* pkg = NULL;
	BfxRwLockRLock(&connection->stateLock);
	if (connection->acceptor)
	{
		pkg = connection->acceptor->ins.maybeBuilder.package.ackPackage;
		Bdt_PackageAddRef(pkg);
	}
	BfxRwLockRUnlock(&connection->stateLock);
	
	if (!pkg)
	{
		BLOG_DEBUG("%s acceptor reset, ignore send ack", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}

	const Bdt_Package* toSend = Bdt_PackageWithRefGet(pkg);
	size_t count = 1;
	uint32_t result = BdtTunnel_Send(
		connection->tunnelContainer,
		&toSend,
		&count,
		NULL);
	Bdt_PackageRelease(pkg);
	BLOG_DEBUG("%s send ack to remote,result=%d", connection->name, result);
	return result;
}

static void BFX_STDCALL ConnectionOnSendAckTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	BdtConnection* connection = userData;
	if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
	{
		BLOG_DEBUG("connection %s stop send ack to remote timer for state checking failed", connection->name);
		return;
	}
	if (connection->acceptor->type != CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
	{
		// acceptor has cover package with builder
		BLOG_DEBUG("connection %s stop send ack to remote timer for acceptor type checking failed", connection->name);

		return;
	}
	
	if (BFX_RESULT_SUCCESS == ConnectionSendAck(connection))
	{
		BfxUserData udConnection;
		ConnectionAsUserData(connection, &udConnection);
		BdtCreateTimeout(
			connection->framework,
			ConnectionOnSendAckTimer,
			&udConnection,
			connection->config.package.resendInterval);
	}
}

static uint32_t ConnectionConfirmWithPackage(BdtConnection* connection)
{
	BLOG_DEBUG("connection %s begin send first answser with ack package", connection->name);
	// for ackack may miss, should resend ack duratively before establish 
	ConnectionSendAck(connection);

	BfxUserData udConnection;
	ConnectionAsUserData(connection, &udConnection);
	BdtCreateTimeout(
		connection->framework,
		ConnectionOnSendAckTimer,
		&udConnection,
		connection->config.package.resendInterval);
	return BFX_RESULT_SUCCESS;
}


uint32_t Bdt_ConnectionConnectWithPackage(BdtConnection* connection)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	ConnectionConnector* connector = BFX_MALLOC_OBJ(ConnectionConnector);
	memset(connector, 0, sizeof(ConnectionConnector));
	connector->type = BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE;
	connector->ins.package.synPackage = Bdt_ConnectionGenSynPackage(connection);
	BLOG_DEBUG("connection %s begin send syn package", connection->name);
	
	result = ConnectionTryEnterConnecting(connection, connector);
	if (result == BFX_RESULT_SUCCESS)
	{
		ConnectionSendSyn(connection);
		BfxUserData udConnection;
		ConnectionAsUserData(connection, &udConnection);
		BdtCreateTimeout(
			connection->framework,
			ConnectionOnSendSynTimer,
			&udConnection,
			connection->config.package.resendInterval);
	}
	else
	{
		ConnectionResetConnector(connection, connector);
	}

	return result;
}

static uint32_t ConnectionCloseConnector(
	BdtConnection* connection, 
	ConnectionConnector* connector)
{
	BLOG_DEBUG("%s close connector", connection->name);
	if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
	{
		Bdt_TcpInterfaceRelease(connector->ins.stream.tcpInterface);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM)
	{
		Bdt_PackageRelease(connector->ins.reverseStream.synPackage);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE)
	{
		Bdt_PackageRelease(connector->ins.package.synPackage);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
	{
		Bdt_TunnelBuilderRelease(connector->ins.builder);
	}
	BfxFree(connector);
	return BFX_RESULT_SUCCESS;
}

static uint32_t ConnectionCloseAcceptor(BdtConnection* connection, ConnectionAcceptor* acceptor)
{
	BLOG_DEBUG("%s close acceptor", connection->name);
	if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
	{
		if (acceptor->ins.maybeBuilder.package.ackPackage)
		{
			Bdt_PackageRelease(acceptor->ins.maybeBuilder.package.ackPackage);
		}
		if (acceptor->ins.maybeBuilder.stream.tcpInterface)
		{
			Bdt_TcpInterfaceRelease(acceptor->ins.maybeBuilder.stream.tcpInterface);
		}
	}
	else if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_BUILDER)
	{
		Bdt_TunnelBuilderRelease(acceptor->ins.builder);
	}
	else if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
	{
		Bdt_TcpInterfaceRelease(acceptor->ins.reverseStream.tcpInterface);
	}
	BfxFree(acceptor);
	return BFX_RESULT_SUCCESS;
}

static uint32_t ConnectionResetConnector(BdtConnection* connection, ConnectionConnector* connector)
{
	BLOG_DEBUG("%s close connector", connection->name);
	if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_STREAM)
	{
		Bdt_TcpInterfaceReset(connector->ins.stream.tcpInterface);
		Bdt_TcpInterfaceRelease(connector->ins.stream.tcpInterface);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM)
	{
		Bdt_PackageRelease(connector->ins.reverseStream.synPackage);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE)
	{
		Bdt_PackageRelease(connector->ins.package.synPackage);
	}
	else if (connector->type == BDT_CONNECTION_CONNECTOR_TYPE_BUILDER)
	{
		Bdt_TunnelContainerCancelBuild(connection->tunnelContainer, connector->ins.builder);
		Bdt_TunnelBuilderRelease(connector->ins.builder);
	}
	BfxFree(connector);
	return BFX_RESULT_SUCCESS;
}

static uint32_t ConnectionResetAcceptor(BdtConnection* connection, ConnectionAcceptor* acceptor)
{
	BLOG_DEBUG("%s reset acceptor", connection->name);
	if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
	{
		if (acceptor->ins.maybeBuilder.package.ackPackage)
		{
			Bdt_PackageRelease(acceptor->ins.maybeBuilder.package.ackPackage);
		}
		if (acceptor->ins.maybeBuilder.stream.tcpInterface)
		{
			Bdt_TcpInterfaceReset(acceptor->ins.maybeBuilder.stream.tcpInterface);
			Bdt_TcpInterfaceRelease(acceptor->ins.maybeBuilder.stream.tcpInterface);
		}
	}
	else if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_BUILDER)
	{
		Bdt_TunnelContainerCancelBuild(connection->tunnelContainer, acceptor->ins.builder);
		Bdt_TunnelBuilderRelease(acceptor->ins.builder);
	}
	else if (acceptor->type == CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
	{
		Bdt_TcpInterfaceReset(acceptor->ins.reverseStream.tcpInterface);
		Bdt_TcpInterfaceRelease(acceptor->ins.reverseStream.tcpInterface);
	}
	BfxFree(acceptor);
	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_ConnectionTryEnterEstablish(BdtConnection* connection)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;
	ConnectionConnector* connector = NULL;
	ConnectionAcceptor* acceptor = NULL;
	BfxRwLockWLock(&connection->stateLock);
	do
	{
		oldState = connection->state;
		if (oldState != BDT_CONNECTION_STATE_CONNECTING)
		{
			result = BFX_RESULT_INVALID_STATE;
		}
		else
		{
			connector = connection->connector;
			connection->connector = NULL;
			acceptor = connection->acceptor;
			connection->acceptor = NULL;
			connection->state = BDT_CONNECTION_STATE_ESTABLISH;
			connection->providerType = BDT_CONNECTION_PROVIDER_TYPE_WAITING;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (oldState == BDT_CONNECTION_STATE_CONNECTING)
	{
		if (connector)
		{
			ConnectionCloseConnector(connection, connector);
		}
		else if (acceptor)
		{
			ConnectionCloseAcceptor(connection, acceptor);
		}
	}

	BLOG_DEBUG("%s try enter establish with result %u", connection->name, result);
	return result;
}

uint32_t Bdt_ConnectionEstablishWithPackage(
	BdtConnection* connection, 
	const Bdt_SessionDataPackage* sessionData)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtConnectionState oldState = BDT_CONNECTION_STATE_NONE;
	BdtConnectionConnectCallback callback = NULL;
	BfxUserData userData;
	BfxRwLockWLock(&connection->stateLock);
	do 
	{
		oldState = connection->state;
		connection->providerType = BDT_CONNECTION_PROVIDER_TYPE_PACKAGE;
		PackageConnection* packageConn = PackageConnectionCreate(
			connection->framework,
			connection
		);
		BfxAtomicExchangePointer(&connection->provider.packageConnection, packageConn);
		callback = connection->connectCallback;
		connection->connectCallback = NULL;
		connection->faCallback = NULL;
		userData = connection->connectUserData;
		connection->connectUserData.lpfnAddRefUserData = NULL;
		connection->connectUserData.lpfnReleaseUserData = NULL;
		connection->connectUserData.userData = NULL;
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);

	if (oldState == BDT_CONNECTION_STATE_ESTABLISH)
	{
		if (sessionData)
		{
			bool handled = false;
			result = PackageConnectionPushPackage(connection->provider.packageConnection, sessionData, &handled);
		}
		else if (!Bdt_ConnectionIsPassive(connection))
		{
			PackageConnectionSendAckAck(connection->provider.packageConnection);
		}

        BdtStat_OnConnectionEstablish(true, Bdt_ConnectionIsPassive(connection));
		ConnectionFireConnectResult(connection, BFX_RESULT_SUCCESS, callback, &userData);
	}
	else if (oldState == BDT_CONNECTION_STATE_CLOSING)
	{
		PackageConnectionClose(connection->provider.packageConnection);
	}
	else if (oldState == BDT_CONNECTION_STATE_CLOSED)
	{
		PackageConnectionReset(connection->provider.packageConnection);
	}

	if (userData.lpfnReleaseUserData)
	{
		userData.lpfnReleaseUserData(userData.userData);
	}

	BLOG_DEBUG("connection %s try to establish with package connection with result %u", connection->name, result);
	return result;
}


BDT_API(uint32_t) BdtConfirmConnection(
	BdtConnection* connection,
	uint8_t* answer,
	size_t len, 
	BdtConnectionConnectCallback connectCallback,
	const BfxUserData* userData
)
{
	if (!Bdt_ConnectionIsPassive(connection))
	{
		BLOG_DEBUG("connection %s ignore confirm for not passive", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}
	if (connection->state != BDT_CONNECTION_STATE_CONNECTING)
	{
		BLOG_DEBUG("connection %s ignore confirm for not connecting", connection->name);
		return BFX_RESULT_INVALID_STATE;
	}
	BFX_BUFFER_HANDLE firstAnswer = BfxCreateBuffer(len);
	BfxBufferWrite(firstAnswer, 0, len, answer, 0);
	if (NULL != BfxAtomicCompareAndSwapPointer(&connection->firstAnswer, NULL, firstAnswer))
	{
		BLOG_DEBUG("connection %s ignore confirm for confirmed", connection->name);

		BfxBufferRelease(firstAnswer);
		return BFX_RESULT_INVALID_STATE;
	}

	BLOG_DEBUG("connection %s confirmed", connection->name);

	if (userData->lpfnAddRefUserData)
	{
		userData->lpfnAddRefUserData(userData->userData);
	}

	const Bdt_PackageWithRef* ack = ConnectionGenAckPackage(connection);

	// in BdtTunnel_SetCachedResponse will call TunnelBuilder's confirm 
	BdtTunnel_SetCachedResponse(
		connection->tunnelContainer,
		Bdt_ConnectionGetConnectSeq(connection),
		ack);

	Bdt_TunnelBuilder* builder = NULL;
	Bdt_TcpInterface* tcpInterface = NULL;

	uint32_t result = BFX_RESULT_SUCCESS;
	uint8_t acceptorType = CONNECTION_ACCEPTOR_TYPE_NULL;

	BfxRwLockWLock(&connection->stateLock);
	do
	{
		if (connection->state == BDT_CONNECTION_STATE_CONNECTING)
		{
			connection->connectCallback = connectCallback;
			connection->connectUserData = *userData;

			acceptorType = connection->acceptor->type;
			if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_BUILDER)
			{
				builder = connection->acceptor->ins.builder;
				Bdt_TunnelBuilderAddRef(builder);
			}
			else if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
			{
				if (connection->acceptor->ins.maybeBuilder.stream.tcpInterface)
				{
					tcpInterface = connection->acceptor->ins.maybeBuilder.stream.tcpInterface;
					Bdt_TcpInterfaceAddRef(tcpInterface);
				}
				else
				{
					connection->acceptor->ins.maybeBuilder.package.ackPackage = ack;
					Bdt_PackageAddRef(ack);
				}
			}
			else if (connection->acceptor->type == CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
			{
				connection->acceptor->ins.reverseStream.confirmed = true;
				if (connection->acceptor->ins.reverseStream.tcpInterface 
					&& connection->acceptor->ins.reverseStream.established)
				{
					tcpInterface = connection->acceptor->ins.reverseStream.tcpInterface;
					Bdt_TcpInterfaceAddRef(tcpInterface);
				}
			}
		}
		else
		{
			result = BFX_RESULT_INVALID_STATE;
		}
	} while (false);
	BfxRwLockWUnlock(&connection->stateLock);


	Bdt_PackageRelease(ack);
	if (result != BFX_RESULT_SUCCESS)
	{
		if (userData->lpfnReleaseUserData)
		{
			userData->lpfnReleaseUserData(userData->userData);
		}

		BLOG_DEBUG("connection %s ignore confirm for checking state failed %u", connection->name, result);

		return result;
	}

	if (acceptorType == CONNECTION_ACCEPTOR_TYPE_BUILDER)
	{
		// do nothing
		Bdt_TunnelBuilderRelease(builder);
	}
	else if (acceptorType == CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER)
	{
		if (tcpInterface)
		{
			result = Bdt_ConnectionTryEnterEstablish(connection);
			if (result == BFX_RESULT_SUCCESS)
			{
				result = ConnectionConfirmWithStream(connection, tcpInterface);
			}
			// send ack with first answer
			result = Bdt_ConnectionEstablishWithStream(connection, tcpInterface);
			Bdt_TcpInterfaceRelease(tcpInterface);
		}
		else
		{
			ConnectionConfirmWithPackage(connection);
		}
	}
	else if (acceptorType == CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM)
	{
		// if tcpInterface established, confirm with stream
		// if tcpInterface not established, wait tcpInterface establish and then confirm with stream
		if (tcpInterface)
		{
			ConnectionConfirmWithStream(connection, tcpInterface);
			Bdt_TcpInterfaceRelease(tcpInterface);
		}
	}

	Bdt_PackageRelease(ack);

	return result;
}

BDT_API(const char*) BdtConnectionGetName(BdtConnection* connection)
{
    return connection->name;
}

BDT_API(uint32_t) BdtConnectionProviderType(BdtConnection* connection)
{
    return connection->providerType;
}

BDT_API(const char*) BdtConnectionGetProviderName(BdtConnection* connection)
{
	if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_STREAM)
	{
		return StreamConnectionGetName(connection->provider.streamConnection);
	}
	else if (connection->providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
	{
		return PackageConnectionGetName(connection->provider.packageConnection);
	}
	else
	{
		return NULL;
	}
}
