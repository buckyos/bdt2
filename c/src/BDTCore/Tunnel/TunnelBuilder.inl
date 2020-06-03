#ifndef __BDT_TUNNEL_BUILDER_INL__
#define __BDT_TUNNEL_BUILDER_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__
#include "./TunnelBuilder.h"
#include "../Interface/NetManager.h"
#include "../PeerFinder.h"
#include "../SnClient/Client.h"
#include "../Interface/NetManager.h"
#include "./TunnelContainer.h"
#include "BDTCore/Connection/Connection.h"
#include "./TunnelBuildAction.inl"
#include "./TcpTunnel.inl"

typedef enum TunnelBuilderState
{
	TUNNEL_BUILDER_STATE_NONE = 0, 
	TUNNEL_BUILDER_STATE_BUILDING,
	TUNNEL_BUILDER_STATE_FINISHED
} TunnelBuilderState;

struct Bdt_TunnelBuilder
{
	int32_t refCount;
	Bdt_TunnelContainer* container;
	BdtSystemFramework* framework;
	BdtPeerFinder* peerFinder;
	BdtHistory_KeyManager* keyManager;
	Bdt_NetManager* netManager;
	BdtSuperNodeClient* snClient;
	Bdt_StackTls* tls;

	const BdtTunnelBuilderConfig config;
	BdtBuildTunnelParams params;
	uint32_t seq;

	int32_t buildingFlags;
	BdtConnection* connection;
	
	// should always check firstPackages but not firstPackageLength
	const Bdt_PackageWithRef** firstPackages;
	size_t firstPackageLength;

	BfxRwLock stateLock;
	// stateLock protected members begin
	TunnelBuilderState state;
	BuildActionRuntime* buildingRuntime;
	// stateLock protected members end 
};

typedef Bdt_TunnelBuilder TunnelBuilder;

static TunnelBuilder* TunnelBuilderCreate(
	struct Bdt_TunnelContainer* container, 
	const BdtTunnelBuilderConfig* config,
	BdtSystemFramework* framework, 
	Bdt_StackTls* tls, 
	BdtPeerFinder* peerFinder,
	BdtHistory_KeyManager* keyManager,
	Bdt_NetManager* netManager,
	BdtSuperNodeClient* snClient);
static uint32_t TunnelBuilderGetSeq(TunnelBuilder* builder);

static uint32_t TunnelBuilderBuildForConnectConnection(
	TunnelBuilder* builder,
	const BdtBuildTunnelParams* params,
	BdtConnection* connection
);
static uint32_t TunnelBuilderBuildForAcceptConection(
	TunnelBuilder* builder,
	const uint8_t key[BDT_AES_KEY_LENGTH],
	const Bdt_SessionDataPackage* sessionData,
	BdtConnection* connection
);

static uint32_t TunnelBuilderBuildForConnectTunnel(
	TunnelBuilder* builder, 
	const Bdt_Package** packages,
	size_t* inoutPackagesCount, 
	const BdtBuildTunnelParams* params
);

static uint32_t TunnelBuilderBuildForAcceptTunnel(
	TunnelBuilder* builder,
	const uint8_t key[BDT_AES_KEY_LENGTH],
	uint32_t seq
);

static uint32_t TunnelBuilderFinishWithRuntime(
	TunnelBuilder* builder,
	BuildActionRuntime* fromRuntime
);

// thread safe, add ref
static BuildActionRuntime* TunnelBuilderGetBuildingRuntime(TunnelBuilder* builder);

#define TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH	3

// no add ref
static const Bdt_PackageWithRef** TunnelBuilderGetFirstPackages(TunnelBuilder* builder, size_t* outPackageLength);

static bool TunnelBuilderIsPassive(TunnelBuilder* builder);

static bool TunnelBuilderIsForConnection(TunnelBuilder* builder);

static uint32_t TunnelBuilderConfirm(Bdt_TunnelBuilder* builder, const Bdt_PackageWithRef* firstResp);

static uint32_t TunnelBuilderOnTcpTunnelReverseConnected(Bdt_TunnelBuilder* builder, TcpTunnel* tunnel);


#endif //__BDT_TUNNEL_BUILDER_INL__
