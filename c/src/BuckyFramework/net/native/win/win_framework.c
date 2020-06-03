#include "./win_tcp.inl"
#include "./win_udp.inl"


//////////////////////////////////////////////////////////////////////////
// tcp

static BDT_SYSTEM_TCP_SOCKET BFX_STDCALL _uvCreateTcpSocket(struct BdtSystemFramework* framework, const BfxUserData* userData) {
    _BuckyFramework* bframe = (_BuckyFramework*)framework;

    return (BDT_SYSTEM_TCP_SOCKET)_newTCPSocket(bframe, userData);
}

static int BFX_STDCALL _uvTcpSocketListen(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* endpoint) {
    
    return _nativeTcpSocketListen((_NativeTCPSocket*)socket, endpoint);
}

static int BFX_STDCALL _uvTcpSocketConnect(BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* local, const BdtEndpoint* remote) {
    return _nativeTcpSocketConnect((_NativeTCPSocket*)socket, local, remote);
}

static int BFX_STDCALL _uvTcpSocketSend(BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* buffer, size_t* inoutSent) {

    return _nativeTcpSocketSend((_NativeTCPSocket*)socket, buffer, inoutSent);
}

static int BFX_STDCALL _uvTcpSocketPause(BDT_SYSTEM_TCP_SOCKET socket) {
    return _nativeTcpSocketPause((_NativeTCPSocket*)socket);
}

static int BFX_STDCALL _uvTcpSocketResume(BDT_SYSTEM_TCP_SOCKET socket) {
    return _nativeTcpSocketResume((_NativeTCPSocket*)socket);
}

static void BFX_STDCALL _uvTcpSocketClose(BDT_SYSTEM_TCP_SOCKET socket, bool ifBreak) {
    _closeTCPSocket((_NativeTCPSocket*)socket);
}

static void BFX_STDCALL _uvDestroyTcpSocket(BDT_SYSTEM_TCP_SOCKET socket) {
    _tcpSocketRelease((_NativeTCPSocket*)socket);
}


//////////////////////////////////////////////////////////////////////////
// udp

