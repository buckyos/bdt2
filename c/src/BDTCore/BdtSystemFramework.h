#ifndef __BDT_SYSTEM_FRAMEWORK_H__
#define __BDT_SYSTEM_FRAMEWORK_H__
#include "./BdtCore.h"

//下面这组希望外部框架实现的tcp socket抽象，和Bdt自己的Connection对象的抽象类似
// 需要抽象 属性，方法，和事件。而事件，又是其中的核心，一个native模块如何在不拥有假设框架的事件模型的情况下，实现event?
// natvie event的关键要素：
//      如何触发
//      如果外部可以控制事件是否持续触发，那么如何操作可以保障不丢事件：停止事件很多时候不是不关心，而是停止工作（pause/resume与set eventflags的区别)
//      如何设置响应函数，在没有框架支持的情况下，触发调用和响应函数是否一定在同一个callstack中？ 
//      方便包装到脚本引擎中，这需要在封装者在事件触发后进行一定程度的同步

#define BDT_TCP_SOCKET_EVENT_ESTABLISH	2
// tcp socket called tcpSocketConnect got establish
// eventParam is NULL
#define BDT_TCP_SOCKET_EVENT_CLOSE		4
// tcp socket called tcpSocketClose(false) got completely closed
// eventParam is NULL
#define BDT_TCP_SOCKET_EVENT_CONNECTION	5
// new tcp socket accept by listener
// implemention of this event should fill outUserData as userData of new tcp socket
// eventParam is struct align(1) {BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint remoteEndpoint, BfxUserData* outUserData}
#define BDT_TCP_SOCKET_EVENT_DRAIN		6
// tcp socket can send more data again
// eventParam is NULL
#define BDT_TCP_SOCKET_EVENT_ERROR		10
// tcp socket got error
// eventParam is uint32_t errorCode

#define BDT_UDP_SOCKET_EVENT_ERROR		10
// udp socket got error
// eventParam is uint32_t errorCode

#define BDT_UDP_SOCKET_MTU		1472

typedef void* BDT_SYSTEM_TCP_SOCKET;
typedef void* BDT_SYSTEM_UDP_SOCKET;
typedef void* BDT_SYSTEM_TIMER;

typedef void (BFX_STDCALL* bdtSystemFrameworkTimeoutEvent)(BDT_SYSTEM_TIMER timer, void* userData);
typedef void (BFX_STDCALL* bdtSystemFrameworkImmediateEvent)(void* userData);

typedef struct BdtSystemFramework
{
	BdtStack* stack;
	BDT_SYSTEM_TCP_SOCKET(BFX_STDCALL* createTcpSocket)(struct BdtSystemFramework* framework, const BfxUserData* userData);
	void (BFX_STDCALL* tcpSocketInitUserdata)(BDT_SYSTEM_TCP_SOCKET socket, const BfxUserData* userData);
	int (BFX_STDCALL* tcpSocketListen)(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* endpoint);
    int (BFX_STDCALL* tcpSocketConnect)(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* local, const BdtEndpoint* remote);
    int (BFX_STDCALL* tcpSocketSend)(BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* buffer, size_t* inoutSent);
    int (BFX_STDCALL* tcpSocketPause)(BDT_SYSTEM_TCP_SOCKET socket);
    int (BFX_STDCALL* tcpSocketResume)(BDT_SYSTEM_TCP_SOCKET socket);
	void (BFX_STDCALL* tcpSocketClose)(BDT_SYSTEM_TCP_SOCKET socket, bool ifBreak);
	void (BFX_STDCALL* destroyTcpSocket)(BDT_SYSTEM_TCP_SOCKET socket);

	BDT_SYSTEM_UDP_SOCKET(BFX_STDCALL* createUdpSocket)(struct BdtSystemFramework* framework, const BfxUserData* userData);
    int (BFX_STDCALL* udpSocketOpen)(BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* local);
    int (BFX_STDCALL* udpSocketSendto)(BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* remote, const uint8_t* buffer, size_t size);
	void (BFX_STDCALL* udpSocketClose)(BDT_SYSTEM_UDP_SOCKET socket);
	void (BFX_STDCALL* destroyUdpSocket)(BDT_SYSTEM_UDP_SOCKET socket);
	
	BDT_SYSTEM_TIMER(BFX_STDCALL* createTimeout)(struct BdtSystemFramework* framework, bdtSystemFrameworkTimeoutEvent onTimeout, const BfxUserData* userData, int32_t timeout);
	void (BFX_STDCALL* destroyTimeout)(BDT_SYSTEM_TIMER timer);

	void (BFX_STDCALL* immediate)(struct BdtSystemFramework* framework, bdtSystemFrameworkImmediateEvent onImmediate, const BfxUserData* userData);

	void (BFX_STDCALL* destroySelf)(struct BdtSystemFramework* framework);
} BdtSystemFramework;

typedef struct BdtStorage BdtStorage;

BDT_API(uint32_t) BdtCreateStack(
	BdtSystemFramework* framework,
	const BdtPeerInfo* localPeer,
	const BdtPeerInfo** initialPeers,
	size_t initialPeerCount,
	BdtStorage* storage,
	StackEventCallback callback,
	const BfxUserData* userData,
	BdtStack** outStack
);

BDT_API(BdtStorage*) BdtCreateSqliteStorage(const BfxPathCharType* filePath, uint32_t pendingLimit);

typedef struct BdtSystemFrameworkEvents {
    void (*bdtPushTcpSocketEvent)(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, uint32_t eventId, void* eventParam, const void* userData);
    void (*bdtPushTcpSocketData)(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* data, size_t bytes, const void* userData);
    void (*bdtPushUdpSocketData)(BdtStack* stack, BDT_SYSTEM_UDP_SOCKET socket, const uint8_t* data, size_t bytes, const BdtEndpoint* remote, const void* userData);
} BdtSystemFrameworkEvents;

typedef struct _BdtTcpSocketConnectionEvent {
    BDT_SYSTEM_TCP_SOCKET socket;
    const BdtEndpoint* remoteEndpoint;
}BdtTcpSocketConnectionEvent;

//驱动调用，由外部来驱动
BDT_API(void) BdtPushTcpSocketEvent(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, uint32_t eventId, void* eventParam, const void* userData);
BDT_API(void) BdtPushTcpSocketData(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* data, size_t bytes, const void* userData);
BDT_API(void) BdtPushUdpSocketData(BdtStack* stack, BDT_SYSTEM_UDP_SOCKET socket, const uint8_t* data, size_t bytes, const BdtEndpoint* remote, const void* userData);


//wrappers
BDT_SYSTEM_TCP_SOCKET BdtCreateTcpSocket(BdtSystemFramework* framework, const BfxUserData* userData);
void BdtTcpSocketInitUserData(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BfxUserData* userData);
uint32_t BdtTcpSocketListen(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* endpoint);
uint32_t BdtTcpSocketConnect(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* local, const BdtEndpoint* remote);
uint32_t BdtTcpSocketSend(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* buffer, size_t* outSent);
uint32_t BdtTcpSocketResume(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket);
uint32_t BdtTcpSocketPause(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket);
void BdtTcpSocketClose(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, bool ifBreak);
void BdtDestroyTcpSocket(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket);

BDT_SYSTEM_UDP_SOCKET BdtCreateUdpSocket(BdtSystemFramework* framework, const BfxUserData* userData);
uint32_t BdtUdpSocketOpen(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* local);
uint32_t BdtUdpSocketSendTo(
	BdtSystemFramework* framework, 
	BDT_SYSTEM_UDP_SOCKET socket, 
	const BdtEndpoint* to, 
	const uint8_t* buffer, 
	size_t size);
void BdtUdpSocketClose(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket);
void BdtDestroyUdpSocket(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket);


BDT_SYSTEM_TIMER BdtCreateTimeout(
	BdtSystemFramework* framework,
	void(BFX_STDCALL* onTimerEvent)(BDT_SYSTEM_TIMER timer, void* userData), 
	const BfxUserData* userData, 
	int32_t timeout);
void BdtDestroyTimeout(
	BdtSystemFramework* framework,
	BDT_SYSTEM_TIMER timer);

void BdtImmediate(
	BdtSystemFramework* framework, 
	void (BFX_STDCALL* callback)(void* userData), 
	const BfxUserData* userData);


#endif //__BDT_SYSTEM_FRAMEWORK_H__