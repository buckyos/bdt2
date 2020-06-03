#ifndef __BDT_CONNECTION_H__
#define __BDT_CONNECTION_H__

#include "../BdtCore.h"
#include "../Tunnel/TunnelModule.h"
#include "../Protocol/Package.h"

#define BDT_CONNECTION_CONNECTOR_TYPE_NONE				0
#define BDT_CONNECTION_CONNECTOR_TYPE_BUILDER			1
#define BDT_CONNECTION_CONNECTOR_TYPE_STREAM			2
#define BDT_CONNECTION_CONNECTOR_TYPE_REVERSE_STREAM	3
#define BDT_CONNECTION_CONNECTOR_TYPE_PACKAGE			4

const char* Bdt_ConnectionGetName(const BdtConnection* connection);
uint32_t Bdt_ConnectionGetId(const BdtConnection* connection);
uint32_t Bdt_ConnectionGetConnectSeq(const BdtConnection* connection);
uint16_t Bdt_ConnectionGetConnectPort(const BdtConnection* connection);
const BdtPeerid* Bdt_ConnectionGetRemotePeerid(const BdtConnection* connection);
Bdt_TunnelContainer* Bdt_ConnectionGetTunnelContainer(const BdtConnection* connection);
bool Bdt_ConnectionIsPassive(const BdtConnection* connection);
bool Bdt_ConnectionIsConfirmed(const BdtConnection* connection);
const BdtConnectionConfig* Bdt_ConnectionGetConfig(const BdtConnection* connection);
const BdtBuildTunnelParams* Bdt_ConnectionGetBuildParams(const BdtConnection* connection);

uint32_t Bdt_ConnectionConnectWithBuilder(
	BdtConnection* connection, 
	Bdt_TunnelBuilder* builder
);
uint32_t Bdt_ConnectionConnectWithStream(
	BdtConnection* connection, 
	const BdtEndpoint* local, 
	const BdtEndpoint* remote);
uint32_t Bdt_ConnectionConnectWithPackage(BdtConnection* connection);
uint32_t Bdt_ConnectionAcceptWithBuilder(BdtConnection* connection, Bdt_TunnelBuilder* builder);
uint32_t Bdt_ConnectionAcceptWithReverseStream(BdtConnection* connection, const BdtEndpoint* remote);
uint32_t Bdt_ConnectionOnTcpFirstPackage(BdtConnection* connection, Bdt_TcpInterface* tcpInterface, const Bdt_Package* firstPackage);
uint32_t Bdt_ConnectionPushPackage(
	BdtConnection* connection,
	const Bdt_Package* package,
	Bdt_TunnelContainer* tunnelContainer, 
	bool* outHandled);

uint32_t Bdt_ConnectionFinishConnectingWithError(BdtConnection* connection, uint32_t error);
uint32_t Bdt_ConnectionTryEnterEstablish(BdtConnection* connection);
uint32_t Bdt_ConnectionEstablishWithStream(BdtConnection* connection, Bdt_TcpInterface* tcpInterface);
uint32_t Bdt_ConnectionEstablishWithPackage(BdtConnection* connection, const Bdt_SessionDataPackage* sessionData);
uint32_t Bdt_ConnectionFireFirstAnswer(BdtConnection* connection, uint32_t remoteId, const uint8_t* data, size_t len);

const Bdt_PackageWithRef* Bdt_ConnectionGenSynPackage(
	BdtConnection* connection
);

void Bdt_ConnectionGenTcpSynPackage(
	BdtConnection* connection, 
	uint8_t result, 
	const Bdt_TunnelEncryptOptions* encrypt,
	Bdt_Package* outPackages[BDT_MAX_PACKAGE_MERGE_LENGTH], 
	uint8_t* outCount);

void Bdt_ConnectionGenTcpReverseSynPackage(
	BdtConnection* connection,
	const BdtEndpoint* local, 
	Bdt_PackageWithRef** outPackage
);
void Bdt_ConnectionGenTcpAckPackage(
	BdtConnection* connection,
	uint8_t result,
	const Bdt_TunnelEncryptOptions* encrypt,
	Bdt_Package* outPackages[BDT_MAX_PACKAGE_MERGE_LENGTH],
	uint8_t* outCount);

void Bdt_ConnectionGenTcpAckAckPackage(
	BdtConnection* connection,
	uint8_t result,
	Bdt_Package** outPackage);

#endif //__BDT_CONNECTION_H__