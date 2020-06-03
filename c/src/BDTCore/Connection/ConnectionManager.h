#ifndef __BDT_CONNECTION_MANAGER_H__
#define __BDT_CONNECTION_MANAGER_H__
#include "../BdtCore.h"
#include "../BdtSystemFramework.h"
#include "../Global/Module.h"
#include "../Interface/InterfaceModule.h"
#include "../Tunnel/TunnelModule.h"
#include "../PeerFinder.h"

typedef struct Bdt_ConnectionManager Bdt_ConnectionManager;
typedef struct Bdt_RemoteConnectionIterator Bdt_RemoteConnectionIterator;

typedef struct BdtConnectionListener BdtConnectionListener;

int32_t BdtConnectionListenerAddRef(BdtConnectionListener* listener);

int32_t BdtConnectionListenerRelease(BdtConnectionListener* listener);

// not thread safe
Bdt_ConnectionManager* Bdt_ConnectionManagerCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls,
	Bdt_NetManager* netManager,
	BdtPeerFinder* peerFinder,
	Bdt_TunnelContainerManager* tunnelManager,
	Bdt_Uint32IdCreator* idCreator,
	const BdtConnectionConfig* connectionConfig,
	const BdtAcceptConnectionConfig* acceptConfig);


// thread safe
BdtConnection* Bdt_AddActiveConnection(
	Bdt_ConnectionManager* manager,
	const BdtPeerid* remotePeerid
);

uint32_t Bdt_AddPassiveConnection(
	Bdt_ConnectionManager* manager,
	uint16_t port,
	const BdtPeerid* remote,
	uint32_t remoteId,
	uint32_t seq, 
	const uint8_t* firstQuestion,
	size_t firstQuestionLen, 
	BdtConnection** outConnection,
	BdtConnectionListener** ountListener
);

uint32_t Bdt_RemoveConnection(
	Bdt_ConnectionManager* manager, 
	BdtConnection* connection
);

uint32_t Bdt_FirePreAcceptConnection(
	Bdt_ConnectionManager* manager,
	BdtConnectionListener* listener,
	BdtConnection* connection);

// thread safe
// add ref
BdtConnection* Bdt_GetConnectionById(
	Bdt_ConnectionManager* manager,
	uint32_t id);

// thread safe
// add ref
BdtConnection* Bdt_GetConnectionByRemoteSeq(
	Bdt_ConnectionManager* manager,
	const BdtPeerid* peerid,
	uint32_t seq);

uint32_t Bdt_AddReverseConnectStreamConnection(
	Bdt_ConnectionManager* manager, 
	const Bdt_TcpSynConnectionPackage* synPackage);

uint32_t Bdt_ConnectionManagerPushPackage(
	Bdt_ConnectionManager* manager,
	const Bdt_Package* package,
	Bdt_TunnelContainer* tunnelContainer,
	bool* outHandled);

#endif