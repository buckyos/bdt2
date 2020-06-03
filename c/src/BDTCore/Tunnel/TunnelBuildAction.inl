#ifndef __BDT_TUNNEL_BUILD_ACTION_INL__
#define __BDT_TUNNEL_BUILD_ACTION_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__
#include "./TunnelBuilder.h"

// use endpoint local: v4udp0.0.0.0:0 remote: v4udp0.0.0.0:0 as PackageConnection's building tunnel key
static const BdtEndpoint* BuildActionGetPackageConnectionEndpoint();

typedef struct BuildActionRuntime BuildActionRuntime;
static int32_t BuildActionRuntimeAddRef(BuildActionRuntime* runtime);
static int32_t BuildActionRuntimeRelease(BuildActionRuntime* runtime);

typedef struct BuildAction BuildAction;

typedef void (*BuildActionRuntimeOnBuildingTunnelActive)(
	BuildActionRuntime* runtime, 
	const BdtEndpoint* local, 
	const BdtEndpoint* remote, 
	BuildAction* action, 
	void* userData
);
typedef void (*BuildActionRuntimeOnBuildingTunnelFinish)(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action, 
	uint32_t result, 
	void* userData
);

static BuildActionRuntime* BuildActionRuntimeCreate(
	Bdt_TunnelBuilder* builder,
	const BdtPeerInfo* remoteInfo, 
	BuildActionRuntimeOnBuildingTunnelActive onBuildingTunnelActive, 
	BuildActionRuntimeOnBuildingTunnelFinish onBuildingTunnelFinish, 
	const BfxUserData* userData);
static void BuildActionRuntimeOnBegin(BuildActionRuntime* runtime);
static void BuildActionRuntimeOnFinish(BuildActionRuntime* runtime);

static void BuildActionRuntimeFinishBuilder(BuildActionRuntime* runtime);

static void BuildActionRuntimeUpdateRemoteInfo(
	BuildActionRuntime* runtime,
	const BdtPeerInfo* remoteInfo);
static const BdtPeerInfo* BuildActionRuntimeGetRemoteInfo(BuildActionRuntime* runtime);

typedef struct BuildingTunnelActionIterator BuildingTunnelActionIterator;
// thread safe but should not enter another lock or do long time operations between enumation
// no add ref
BuildAction* BuildActionRuntimeEnumBuildingTunnelAction(
	BuildActionRuntime* runtime,
	BuildingTunnelActionIterator** outIter);
// thread safe but should not enter another lock or do long time operations between enumation
// no add ref
BuildAction* BuildActionRuntimeEnumBuildingTunnelActionNext(
	BuildingTunnelActionIterator* iter);
// thread safe but should not enter another lock or do long time operations between enumation
void BuildActionRuntimeEnumBuildingTunnelActionFinish(
	BuildingTunnelActionIterator* iter);



typedef void (*BuildActionOnResolve)(BuildAction* action, void* userData);
typedef void (*BuildActionOnReject)(BuildAction* action, void* userData, uint32_t error);

typedef enum BuildActionState
{
	BUILD_ACTION_STATE_NONE = 0,
	BUILD_ACTION_STATE_BOUND, 
	BUILD_ACTION_STATE_EXECUTING,
	BUILD_ACTION_STATE_EXECUTED, 
	BUILD_ACTION_STATE_RESOLVED,
	BUILD_ACTION_STATE_REJECTED,
	BUILD_ACTION_STATE_CANCELED
} BuildActionState;

static int32_t BuildActionAddRef(BuildAction* action);
static int32_t BuildActionRelease(BuildAction* action);

#define BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION	(1<<0)
#define BUILD_ACTION_CALLBACK_RESOLVE_ACTION	(1<<1)
#define BUILD_ACTION_CALLBACK_REJECT_FUNCTION	(1<<2)
#define BUILD_ACTION_CALLBACK_REJECT_ACTION		(1<<3)

typedef struct BuildActionCallbacks
{
	uint8_t flags;
	union
	{
		struct
		{
			BuildActionOnResolve function;
			BfxUserData userData;
		} function;
		struct
		{
			BuildAction* action;
			struct BuildActionCallbacks* callbacks;
		} action;
	} resolve;
	
	union
	{
		struct
		{
			BuildActionOnReject function;
			BfxUserData userData;
		} function;
		struct
		{
			BuildAction* action;
			struct BuildActionCallbacks* callbacks;
		} action;
	} reject;
} BuildActionCallbacks;

//TODO: 可读性 构建串行Action的代码，读起来不太明显
// static chained actions use action as callback
// dynamic chained actions or tail action of chain use function as callback
// no seprate set callback methods to save state checking 
// add ref action, should always call one of BuildActionCancel/BuildActionResolve/BuildActionReject to finish execute
static uint32_t BuildActionExecute(BuildAction* action, BuildActionCallbacks* callback);

// after BuildActionCancel called, action maybe released!
static uint32_t BuildActionCancel(BuildAction* action);

// after BuildActionResolve called, action maybe released!
static void BuildActionResolve(BuildAction* action);

// after BuildActionReject called, action maybe released!
static void BuildActionReject(BuildAction* action, uint32_t error);

// no add ref!!!
static BuildActionRuntime* BuildActionGetRuntime(BuildAction* action);



//add ref
static BuildAction* BuildActionRuntimeGetBuildingTunnelAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote);

static uint32_t BuildActionRuntimeAddBuildingTunnelAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	BuildActionCallbacks* callbacks,
	bool actived);

static void BuildActionRuntimeActiveBuildingTunnel(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action
);

static void BuildActionRuntimeFinishBuildingTunnel(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result
);



// when action list released, all action in list will be canceled
typedef struct BuildActionList BuildActionList;
static BuildActionList* BuildActionListCreate();
static int32_t BuildActionListAddRef(BuildActionList* list);
static int32_t BuildActionListRelease(BuildActionList* list);
static void BuildActionListAddAction(BuildActionList* list, BuildAction* action);

typedef struct ListBuildAction ListBuildAction;
// same as Promise.all
// all added actions resolved, resolved
// one of added actions rejected, rejected
static ListBuildAction* ListBuildActionAllCreate(BuildActionRuntime* runtime, const char* name);
// same as Promise.race TODO：不完全一样
// one of added actions resolved, resolved
// all of added actions rejected, rejected
static ListBuildAction* ListBuildActionRaceCreate(BuildActionRuntime* runtime, const char* name);
// not thread safe!!!
static uint32_t ListBuildActionAdd(ListBuildAction* actions, BuildAction* action);


// only for inherit types 
#define DEFINE_BUILD_ACTION() \
	int32_t refCount;\
	BuildActionState state;\
	BuildActionOnResolve resolve;\
	BfxUserData resolveUserData;\
	BuildActionOnReject reject;\
	BfxUserData rejectUserData;\
	BuildActionRuntime* runtime;\
	char* name; \
	void (*uninit)(BuildAction* action);\
	void (*onCanceled)(BuildAction* action);\
	void (*execute)(BuildAction* action, uint32_t preResult);

static void BuildActionInit(BuildAction* action, BuildActionRuntime* runtime);
static const char* BuildActionGetName(BuildAction* action);
static void BuildActionAsUserData(BuildAction* action, BfxUserData* outUserData);
static void BuildActionAsCallbacks(BuildAction* action, BuildActionCallbacks* callbacks);

#define DEFINE_BUILD_ACTION_LIST() \
	DEFINE_BUILD_ACTION()\
	BfxList actions;\
	BuildActionOnResolve oneResolve;\
	BuildActionOnReject oneReject;

static void ListBuildActionInit(ListBuildAction* action, BuildActionRuntime* runtime, const char* name);


typedef void (*BuildTimeoutActionTickCallback)(void* userData);
typedef struct TimerTickAction TimerTickAction;
typedef struct BuildTimeoutAction BuildTimeoutAction;
// not thread safe!!!
static uint32_t BuildTimeoutActionAddTickAction(BuildTimeoutAction* timeoutAction, BuildTimeoutActionTickCallback callback, const BfxUserData* userData);
static BuildTimeoutAction* CreateBuildTimeoutAction(BuildActionRuntime* runtime, int timeout, int cycle, const char* name);


// action send SynTunnelPackage duratively 
// 1. send SynTunnelPackage merged with session object's first package (for connection it's session data package with syn flag) with timeoutAction's tick cycle
//		from udpInterface's local endpoint to remote endpoint 
// 2. check lastRecv of UdpTunnel from udpInterface's local endpoint to remote endpoint, if lastRecv not 0, fire tunnel actived, action resolved
//    TODO：重要 通过timer来验证lastRecv的方式来resolve,会引入固有延时。
typedef struct SynUdpTunnelAction
{
	DEFINE_BUILD_ACTION()
	const Bdt_UdpInterface* udpInterface;
	BdtEndpoint remote;
	Bdt_UdpTunnel* tunnel;
	int64_t startingLastRecv;
} SynUdpTunnelAction;
static SynUdpTunnelAction* CreateSynUdpTunnelAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction, 
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote);



#define DEFINE_PASSIVE_BUILD_ACTION() \
	DEFINE_BUILD_ACTION() \
	uint32_t(*confirm)(struct PassiveBuildAction* action, const Bdt_PackageWithRef* firstResp);

typedef struct PassiveBuildAction
{
	DEFINE_PASSIVE_BUILD_ACTION()
} PassiveBuildAction;
uint32_t PassiveBuildActionConfirm(PassiveBuildAction* action, const Bdt_PackageWithRef* firstResp);

// action send AckTunnelPackage duratively
// 1. if waitConfirm not set, begin send AckTunnelPackage with timeoutAction's tick cycle
// 2. if waitConfirm set, do nothing before time of waitConfirm or session object confirmed;  
//		if timeout reached waitConfirm but session object not confirmed,  begin send AckTunnelPackage with timeoutAction's tick cycle;
//		if session object confirmed, begin send AckTunnelPackage merged with session object's first resp (for connection it's session data package with ack flag)
// 3. check lastRecv of UdpTunnel from udpInterface's local endpoint to remote endpoint, if lastRecv not 0, fire tunnel actived, action resolved
typedef struct AckUdpTunnelAction
{
	DEFINE_PASSIVE_BUILD_ACTION()
	const Bdt_UdpInterface* udpInterface;
	BdtEndpoint remote;
	Bdt_UdpTunnel* tunnel;
	uint32_t waitConfirm;
	int64_t startTime;
	int64_t startingLastRecv;
} AckUdpTunnelAction;

static AckUdpTunnelAction* CreateAckUdpTunnelAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction, 
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote, 
	uint32_t waitConfirm);




// action connect tcp interface and send SynTunnelPackage
// 1. connect tcp socket from local to remote
// 2. when tcp socket established send SynTunnelPackage merged with session object's first package (for connection it's session data package with syn flag)
// 4. when got AckTunnelPackage, fire tunnel actived, action resolved
typedef struct SynTcpTunnelAction
{
	DEFINE_BUILD_ACTION()
	BdtEndpoint local;
	BdtEndpoint remote;
	Bdt_TcpTunnel* tunnel;
} SynTcpTunnelAction;

static SynTcpTunnelAction* CreateSynTcpTunnelAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction, 
	const BdtEndpoint* local,
	const BdtEndpoint* remote
);


// base of connect connection action
// action resolved when connector can got established
typedef struct ConnectConnectionAction ConnectConnectionAction;
#define DEFINE_CONNECT_CONNECTION_ACTION() \
	DEFINE_BUILD_ACTION() \
	uint32_t(*continueConnect)(struct ConnectConnectionAction* action); 

static uint32_t ConnectConnectionActionInit(
	ConnectConnectionAction* action,
	BuildActionRuntime* runtime
);
// called when active builder decide to use this action as connection's provider
static uint32_t ConnectConnectionActionContinueConnect(
	ConnectConnectionAction* action
);


// connect connection with tcp stream action
// 1. connect tcp socket from local to remote
// 2. when tcp socket established, fire tunnel actived
// 3. when ConnectConnectionContinueConnect called, send TcpSynConnection with result BDT_PACKAGE_RESULT_SUCCESS
// 4. when got TcpAckConnection, action resolved
typedef struct ConnectStreamAction {
	DEFINE_CONNECT_CONNECTION_ACTION()
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	Bdt_TcpInterface* tcpInterface; 
	BdtEndpoint local;
	BdtEndpoint remote;
} ConnectStreamAction;
static ConnectStreamAction* CreateConnectStreamAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local, 
	const BdtEndpoint* remote);

// accept reverse connect stream from remote
// 1. when got TcpAckConnection as first package from tcp socket, create AcceptReverseStreamAction, fire tunnel actived
// 2. when ConnectConnectionContinueConnect called, send TcpAckAckConnection with result BDT_PACKAGE_RESULT_SUCCESS, action resolved
typedef struct AcceptReverseStreamAction {
	DEFINE_CONNECT_CONNECTION_ACTION()
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	Bdt_TcpInterface* tcpInterface;
} AcceptReverseStreamAction;
static AcceptReverseStreamAction* CreateAcceptReverseStreamAction(
	BuildActionRuntime* runtime,
	Bdt_TcpInterface* tcpInterface);


// connect connection with package connection action
// 1. create ConnectPackageConnectionAction if udp is not disabled, before ConnectPackageConnectionAction created, several SynUdpTunnel action should be executing
// 2. SynUdpTunnelAction duratively send SynTunnel package merged with SessionData package with syn until resolved or canceled, 
//		but SynUdpTunnelAction may get resolved before ConnectPackageConnectionAction fire tunnel actived event, 
//		because remote SynUdpTunnelAction may send AckTunnel package without SessionData package with ack for Bdt_ConnectionConfirm not called		
// 3. in some strategy implemention, call ConnectPackageConnectionActionBeginSyn when SynUdpTunnelAction resolved but no SessionData package with ack received
// 4. SessionData package with ack received, fire tunnel actived
// 5. when ConnectConnectionActionContinueConnect called, ConnectPackageConnectionAction got resolved, connection got established with package connection
// 6. package connection should send SessionData package with ackack imediately or wait first Bdt_ConnectionSend called  
typedef struct ConnectPackageConnectionAction {
	DEFINE_CONNECT_CONNECTION_ACTION()
	const Bdt_PackageWithRef* synPackage;
	bool beginSyn;
} ConnectPackageConnectionAction;
static ConnectPackageConnectionAction* CreateConnectPackageConnectionAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction
);
static uint32_t ConnectPackageConnectionActionBeginSyn(
	ConnectPackageConnectionAction* action
);
static uint32_t ConnectPackageConnectionActionOnSessionData(
	ConnectPackageConnectionAction* action, 
	const Bdt_SessionDataPackage* session
);


// accept connect stream from local
// 1. when got TcpSynConnection as first package from tcp socket, create AcceptStreamAction, fire tunnel actived
// 2. if confirm has called, send TcpAckConnection with first answer, action resolved, otherwise begin waiting confirm
// 3. if confirm called and action is waiting confirm, send TcpAckConnection with first answer, action resolved
typedef struct AcceptStreamAction {
	DEFINE_PASSIVE_BUILD_ACTION()
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	Bdt_TcpInterface* tcpInterface;
} AcceptStreamAction;
static AcceptStreamAction* CreateAcceptStreamAction(
	BuildActionRuntime* runtime, 
	Bdt_TcpInterface* tcpInterface
);

// reverse connect stream to local
// 1. connect tcp socket from remote to local
// 2. when tcp socket established, wait confirm 
// 3. when confirm called, send TcpAckConnection with first answer
// 4. if TcpAckAckConnection received, action resolved
typedef struct ConnectReverseStreamAction {
	DEFINE_PASSIVE_BUILD_ACTION()
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	BdtEndpoint local;
	BdtEndpoint remote;
	Bdt_TcpInterface* tcpInterface;
	
	BfxThreadLock tcpEstablishedLock;
	// tcpEstablishedLock protected members begin
	bool tcpEstablished;
	bool confirmed;
	// tcpEstablishedLock protected members end
} ConnectReverseStreamAction;
static ConnectReverseStreamAction* CreateConnectReverseStreamAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local, 
	const BdtEndpoint* remote
);

// accept connect package connection from local
// 1. create AcceptPackageConnectionAction if udp's not disabled
// 2. before AcceptPackageConnectionAction created, several AckUdpTunnel action should be executing
// 3. when confirm called, set session data package with first answer to response cache, and send it from tunnel
// 4. if first SessionData without syn part received, action resolved
typedef struct AcceptPackageConnectionAction {
	DEFINE_PASSIVE_BUILD_ACTION()
	bool beginAck;
	const Bdt_PackageWithRef* ackPackage;
} AcceptPackageConnectionAction;
static AcceptPackageConnectionAction* CreateAcceptPackageConnectionAction(
	BuildActionRuntime* runtime,
	BuildTimeoutAction* timeoutAction
);
static uint32_t AcceptPackageConnectionActionBeginAck(
	AcceptPackageConnectionAction* action
);
static uint32_t AcceptPackageConnectionActionOnSessionData(
	AcceptPackageConnectionAction* action,
	const Bdt_SessionDataPackage* session);



typedef struct SnCallAction SnCallAction;
static SnCallAction* CreateSnCallAction(
	BuildActionRuntime* runtime,
	const BdtPeerid* snPeerid,
	bool updateRemoteInfo);




#endif //__BDT_TUNNEL_BUILD_ACTION_INL__
