#include "../../framework.h"
#include "../../internal.h"
#include "./uv_udp.inl"
#include "./uv_tcp.inl"


static void BFX_STDCALL _buckyFrameworkSocketInitUserData(void* sock, const BfxUserData* userData) {
    _UVSocketCommon* common = (_UVSocketCommon*)sock;
    _uvSocketInitUserData(common, *userData);
}

BFX_API(void) buckyFrameworkSocketGetUserData(void* sock, BfxUserData* userData) {
    _UVSocketCommon* common = (_UVSocketCommon*)sock;
    _uvSocketGetUserData(common, userData);
}


//////////////////////////////////////////////////////////////////////////
// tcp

static BDT_SYSTEM_TCP_SOCKET BFX_STDCALL _uvCreateTcpSocket(struct BdtSystemFramework* framework, const BfxUserData* userData) {
    _BuckyFramework* bframe = (_BuckyFramework*)framework;

    return (BDT_SYSTEM_TCP_SOCKET)_createTcpSocketSync(bframe, userData);
}

static int BFX_STDCALL _uvTcpSocketListen(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* endpoint) {
    return _tcpSocketListenSync((_UVTCPSocket*)socket, endpoint);
}

static int BFX_STDCALL _uvTcpSocketConnect(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* local, const BdtEndpoint* remote) {
    return _tcpSocketConnectSync((_UVTCPSocket*)socket, local, remote);
}

static int BFX_STDCALL _uvTcpSocketSend(BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* buffer, size_t* inoutSent) {
    return _tcpSocketSendAsync((_UVTCPSocket*)socket, buffer, inoutSent);
}

static int BFX_STDCALL _uvTcpSocketPause(BDT_SYSTEM_TCP_SOCKET socket) {
    return _tcpSocketPauseSync((_UVTCPSocket*)socket);
}

static int BFX_STDCALL _uvTcpSocketResume(BDT_SYSTEM_TCP_SOCKET socket) {
    return _tcpSocketResumeSync((_UVTCPSocket*)socket);
}

static void BFX_STDCALL _uvTcpSocketClose(BDT_SYSTEM_TCP_SOCKET socket, bool ifBreak) {
    _tcpSocketCloseSync((_UVTCPSocket*)socket);
}

static void BFX_STDCALL _uvDestroyTcpSocket(BDT_SYSTEM_TCP_SOCKET socket) {
    _tcpSocketDestroySync((_UVTCPSocket*)socket);
}

// tcp events
static void _uvTcpOnConnection(_UVTCPSocket* socket, _UVTCPSocket* comingSocket) {
    _BuckyFramework* bframe = socket->framework;

    BdtTcpSocketConnectionEvent event = {
        .socket = (BDT_SYSTEM_TCP_SOCKET)comingSocket,
        .remoteEndpoint = &comingSocket->remote, 
    };

    bframe->events.bdtPushTcpSocketEvent(bframe->base.stack, (BDT_SYSTEM_TCP_SOCKET)socket, BDT_TCP_SOCKET_EVENT_CONNECTION, &event, socket->udata.userData);
}

static void _uvTcpOnEstablished(_UVTCPSocket* socket) {
    _BuckyFramework* bframe = socket->framework;

    bframe->events.bdtPushTcpSocketEvent(bframe->base.stack, (BDT_SYSTEM_TCP_SOCKET)socket, BDT_TCP_SOCKET_EVENT_ESTABLISH, NULL, socket->udata.userData);
}

static void _uvTcpOnDrain(_UVTCPSocket* socket) {
    _BuckyFramework* bframe = socket->framework;

    bframe->events.bdtPushTcpSocketEvent(bframe->base.stack, (BDT_SYSTEM_TCP_SOCKET)socket, BDT_TCP_SOCKET_EVENT_DRAIN, NULL, socket->udata.userData);
}

static void _uvTcpOnData(_UVTCPSocket* socket, const char* data, size_t bytes) {
    _BuckyFramework* bframe = socket->framework;

    bframe->events.bdtPushTcpSocketData(bframe->base.stack, (BDT_SYSTEM_TCP_SOCKET)socket, (const uint8_t*)data, bytes, socket->udata.userData);
}

static void _uvTcpOnError(_UVTCPSocket* socket, int errCode, const char* errMsg) {
    _BuckyFramework* bframe = socket->framework;
	uint32_t tErr = BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, errCode);
    bframe->events.bdtPushTcpSocketEvent(bframe->base.stack, (BDT_SYSTEM_TCP_SOCKET)socket, BDT_TCP_SOCKET_EVENT_ERROR, (void*)&tErr, socket->udata.userData);
}

//////////////////////////////////////////////////////////////////////////
// udp

static BDT_SYSTEM_UDP_SOCKET BFX_STDCALL _uvCreateUdpSocket(struct BdtSystemFramework* framework, const BfxUserData* userData) {
    _BuckyFramework* bframe = (_BuckyFramework*)framework;

    _UVUDPSocket* sock = _createUdpSocketSync(bframe, userData);
    return (BDT_SYSTEM_UDP_SOCKET)sock;
}

static int BFX_STDCALL _uvUdpSocketOpen(BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* local) {
    return _udpSocketOpenSync((_UVUDPSocket*)socket, local);
}

static int BFX_STDCALL _uvUdpSocketSendto(BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* remote, const uint8_t* buffer, size_t size) {
    return _udpSocketSendToAsync((_UVUDPSocket*)socket, remote, buffer, size);
}

static void BFX_STDCALL _uvUdpSocketClose(BDT_SYSTEM_UDP_SOCKET socket) {
    _udpSocketCloseSync((_UVUDPSocket*)socket);
}

static void BFX_STDCALL _uvDestroyUdpSocket(BDT_SYSTEM_UDP_SOCKET socket) {
    _udpSocketDestroySync((_UVUDPSocket*)socket);
}

// udp events
/*
static void _uvUdpSocketOnData(_UVUDPSocket* socket, const char* data, size_t bytes, const BdtEndpoint* remote) {
    _BuckyFramework* bframe = socket->framework;

    bframe->events.bdtPushUdpSocketData(bframe->base.stack, (BDT_SYSTEM_UDP_SOCKET)socket, data, bytes, remote, socket->udata.userData);
}
*/
static void _uvUdpSocketError(_UVUDPSocket* socket, int errCode, const char* errMsg) {
    // TODO
}

void _fillNetFramework(_BuckyFramework* framework) {
    BdtSystemFramework* bdtFrame = &framework->base;

    bdtFrame->createTcpSocket = _uvCreateTcpSocket;
    bdtFrame->tcpSocketListen = _uvTcpSocketListen;
	bdtFrame->tcpSocketInitUserdata = _buckyFrameworkSocketInitUserData;
    bdtFrame->tcpSocketConnect = _uvTcpSocketConnect;
    bdtFrame->tcpSocketSend = _uvTcpSocketSend;
    bdtFrame->tcpSocketPause = _uvTcpSocketPause;
    bdtFrame->tcpSocketResume = _uvTcpSocketResume;
    bdtFrame->tcpSocketClose = _uvTcpSocketClose;
    bdtFrame->destroyTcpSocket = _uvDestroyTcpSocket;

    framework->tcpEvents.connection = _uvTcpOnConnection;
    framework->tcpEvents.established = _uvTcpOnEstablished;
    framework->tcpEvents.data = _uvTcpOnData;
    framework->tcpEvents.error = _uvTcpOnError;
    framework->tcpEvents.drain = _uvTcpOnDrain;


    bdtFrame->createUdpSocket = _uvCreateUdpSocket;
    bdtFrame->udpSocketOpen = _uvUdpSocketOpen;
    bdtFrame->udpSocketSendto = _uvUdpSocketSendto;
    bdtFrame->udpSocketClose = _uvUdpSocketClose;
    bdtFrame->destroyUdpSocket = _uvDestroyUdpSocket;

    framework->udpEvents.data = _uvUdpSocketOnData;
    framework->udpEvents.error = _uvUdpSocketError;
}




