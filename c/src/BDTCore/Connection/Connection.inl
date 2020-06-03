#ifndef __BDT_CONNECTION_CONTAINER_INL__
#define __BDT_CONNECTION_CONTAINER_INL__

#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "./Connection.h"
#include "../Global/Module.h"
#include "../PeerFinder.h"


#define CONNECTION_CONNECT_FLAG_PASSIVE				(1<<0)


struct Bdt_ConnectionManager;
static BdtConnection* ConnectionCreate(
	BdtSystemFramework* framework, 
	Bdt_StackTls* tls,
	Bdt_NetManager* netManager,
	BdtPeerFinder* peerFinder,
	const BdtPeerid* remote, 
	Bdt_TunnelContainer* tunnelContainer, 
	struct Bdt_ConnectionManager* connectionManager, 
	const BdtConnectionConfig* config, 
	uint32_t id, 
	uint32_t connectSeq, 
	uint32_t remoteId,
	int32_t flags
);

static void ConnectionInitAsInterfaceOwner(BdtConnection* connection);
static void ConnectionAsUserData(BdtConnection* connection, BfxUserData* outUserData);

static uint32_t ConnectionGetRemoteId(const BdtConnection* connection);
static BdtConnectionState ConnectionGetState(const BdtConnection* connection);

static void ConnectionFireConnectResult(
	BdtConnection* connection, 
	uint32_t result, 
	BdtConnectionConnectCallback callback, 
	const BfxUserData* userData);
static void ConnectionSetFirstQuestion(BdtConnection* connection, const void* firstQuestion, size_t len);
static BFX_BUFFER_HANDLE ConnectionGetFirstQuestion(const BdtConnection* connection);
static uint32_t ConnectionTryEnterConnectWaiting(
	BdtConnection* connection, 
	BdtConnectionConnectCallback connectCallback, 
	const BfxUserData* connectUserData, 
	BdtConnectionFirstAnswerCallback faCallback, 
	const BfxUserData* faUserData);

static BFX_BUFFER_HANDLE ConnectionGetFirstAnswer(BdtConnection* connection);

typedef struct ConnectionConnector
{
	uint8_t type;
	union
	{
		Bdt_TunnelBuilder* builder;
		struct
		{
			Bdt_TcpInterface* tcpInterface;
		} stream;
		struct
		{
			const Bdt_PackageWithRef* synPackage;
		} package;
		struct
		{
			BdtEndpoint local;
			BdtEndpoint remote;
			Bdt_PackageWithRef* synPackage;
		} reverseStream;
	} ins;
} ConnectionConnector;


#define CONNECTION_ACCEPTOR_TYPE_NULL			0
#define CONNECTION_ACCEPTOR_TYPE_MAYBE_BUILDER	1
#define CONNECTION_ACCEPTOR_TYPE_BUILDER		2
#define CONNECTION_ACCEPTOR_TYPE_REVERSE_STREAM	3
typedef struct ConnectionAcceptor
{
	uint8_t type;
	union
	{
		Bdt_TunnelBuilder* builder;
		struct
		{
			struct
			{
				Bdt_TcpInterface* tcpInterface;
			} stream;
			struct
			{
				const Bdt_PackageWithRef* ackPackage;
			} package;
		} maybeBuilder;
		struct
		{
			bool confirmed;
			bool established;
			Bdt_TcpInterface* tcpInterface;
		} reverseStream;
	} ins;

} ConnectionAcceptor;


static uint32_t ConnectionCloseConnector(BdtConnection* connection, ConnectionConnector* connector);
static uint32_t ConnectionCloseAcceptor(BdtConnection* connection, ConnectionAcceptor* acceptor);
static uint32_t ConnectionResetConnector(BdtConnection* connection, ConnectionConnector* connector);
static uint32_t ConnectionResetAcceptor(BdtConnection* connection, ConnectionAcceptor* acceptor);

const Bdt_PackageWithRef* ConnectionGenAckPackage(
	BdtConnection* connection
);

#endif //__BDT_CONNECTION_CONTAINER_INL__