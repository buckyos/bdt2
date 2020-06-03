#ifndef __BDT_TUNNEL_CONNECTION_INL__
#define __BDT_TUNNEL_CONNECTION_INL__

#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "../../Tunnel/TunnelModule.h"
#include "./SendQueue.inl"
#include "./RecvQueue.inl"

typedef struct PackageConnection PackageConnection;

typedef enum PackageConnectionState
{
	PACKAGE_CONNECTION_STATE_NONE = 0, 
	PACKAGE_CONNECTION_STATE_ESTABLISH,
	PACKAGE_CONNECTION_STATE_CLOSING, 
	PACKAGE_CONNECTION_STATE_CLOSE,
} PackageConnectionState;

typedef enum PackageConnectionCloseSubState
{
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE = 0,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN2,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSING,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_TIMEWAIT,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_WAITCLOSE,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_LASTACK,
	PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED,
}PackageConnectionCloseSubState;

static const char* PackageConnectionGetName(PackageConnection* connection);
static PackageConnection* PackageConnectionCreate(BdtSystemFramework* framework, BdtConnection* owner);
static void PackageConnectionDestroy(PackageConnection* connection);

// thread safe
// reenter
static uint32_t PackageConnectionPushPackage(
	PackageConnection* connection,
	const Bdt_SessionDataPackage* package,
	bool* outHandled
);
static uint32_t PackageConnectionClose(PackageConnection* connection);
static uint32_t PackageConnectionReset(PackageConnection* connection);
static uint32_t PackageConnectionSend(
	PackageConnection* connection,
	const uint8_t* buffer,
	size_t length,
	BdtConnectionSendCallback callback,
	const BfxUserData* userData);



static Bdt_TunnelContainer* PackageConnectionGetTunnel(
	PackageConnection* connection
);

static uint32_t PackageConnectionGetRemoteId(
	PackageConnection* connection
);


static uint32_t PackageConnectionRecv(
	PackageConnection* connection, 
	uint8_t* data, 
	size_t len, 
	BdtConnectionRecvCallback callback, 
	const BfxUserData* userData
);

static uint32_t PackageConnectionSendAckAck(PackageConnection* connection);

static void PackageConnectionOnPreSendPackage(PackageConnection* connection, Bdt_SessionDataPackage* package);
static uint32_t PackageConnectionSendPackage(PackageConnection* connection, Bdt_SessionDataPackage* package);
static uint32_t PackageConnectionFillPackage(PackageConnection* connection, Bdt_SessionDataPackage* package, uint16_t payloadLength);
static void PackageConnectionSendDrive(PackageConnection* connection);
static void PackageConnectionTrySendPackage(PackageConnection* connection);

static void PackageConnectionMaybeCloseFinish(PackageConnection* connection);

static RecvQueue* PackageConnectionGetRecvQueue(PackageConnection* connection);
static SendQueue* PackageConnectionGetSendQueue(PackageConnection* connection);

static void BFX_STDCALL PackageConnectionFinTimerCallback(BDT_SYSTEM_TIMER timer, void* userData);
static void PackageConnectionBeginFinTimer(PackageConnection* connection);

static void BFX_STDCALL PackageConnectionTimewaitTimerCallback(BDT_SYSTEM_TIMER timer, void* userData);
static void PackageConnectionBeginTimewaitTimer(PackageConnection* connection);

static void BFX_STDCALL PackageConnectionRtoTimerCallback(BDT_SYSTEM_TIMER timer, void* userData);
static void PackageConnectionBeginRtoTimer(PackageConnection* connection);

static void BFX_STDCALL PackageConnectionRecvTimerCallback(BDT_SYSTEM_TIMER timer, void* userData);
static void PackageConnectionBeginRecvTimer(PackageConnection* connection);

static void PackageConnectionUpdateCloseSubState(PackageConnection* connection, uint16_t cmdflags, bool recv);

static void PackageConnectionNotifyRecvReadyBuffers(PackageConnection* connection, BfxList* buffers);
static void PackageConnectionNotifySendCompleteBuffers(PackageConnection* connection, BfxList* buffers);

static void PackageConnectionEstimateRtt(PackageConnection* connection, const Bdt_SessionDataPackage* package);

#endif //__BDT_TUNNEL_CONNECTION_INL__

