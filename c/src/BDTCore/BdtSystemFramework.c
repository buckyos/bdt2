#include "./BdtSystemFramework.h"

BDT_SYSTEM_TCP_SOCKET BdtCreateTcpSocket(BdtSystemFramework* framework, const BfxUserData* userData)
{
	return framework->createTcpSocket(framework, userData);
}

void BdtTcpSocketInitUserData(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BfxUserData* userData)
{
	framework->tcpSocketInitUserdata(socket, userData);
}

uint32_t BdtTcpSocketListen(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* endpoint)
{
	return framework->tcpSocketListen(socket, endpoint);
}

uint32_t BdtTcpSocketConnect(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const BdtEndpoint* local, const BdtEndpoint* remote)
{
	return framework->tcpSocketConnect(socket, local, remote);
}

uint32_t BdtTcpSocketSend(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* buffer, size_t* inoutSent)
{
	return framework->tcpSocketSend(socket, buffer, inoutSent);
}

uint32_t BdtTcpSocketResume(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket)
{
	return framework->tcpSocketResume(socket);
}

uint32_t BdtTcpSocketPause(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket)
{
	return framework->tcpSocketPause(socket);
}

void BdtTcpSocketClose(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket, bool ifBreak)
{
	framework->tcpSocketClose(socket, ifBreak);
}

void BdtDestroyTcpSocket(BdtSystemFramework* framework, BDT_SYSTEM_TCP_SOCKET socket)
{
	framework->destroyTcpSocket(socket);
}

BDT_SYSTEM_UDP_SOCKET BdtCreateUdpSocket(BdtSystemFramework* framework, const BfxUserData* userData)
{
	return framework->createUdpSocket(framework, userData);
}

uint32_t BdtUdpSocketOpen(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* local)
{
	return framework->udpSocketOpen(socket, local);
}

void BdtUdpSocketClose(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket)
{
	framework->udpSocketClose(socket);
}


uint32_t BdtUdpSocketSendTo(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket, const BdtEndpoint* to, const uint8_t* buffer, size_t size)
{
	return framework->udpSocketSendto(socket, to, buffer, size);
}

void BdtDestroyUdpSocket(BdtSystemFramework* framework, BDT_SYSTEM_UDP_SOCKET socket)
{
	framework->destroyUdpSocket(socket);
}


BDT_SYSTEM_TIMER BdtCreateTimeout(
	BdtSystemFramework* framework, 
	void(BFX_STDCALL* onTimeoutEvent)(BDT_SYSTEM_TIMER timer, void* userData), 
	const BfxUserData* userData, 
	int32_t timeout)
{
	return framework->createTimeout(framework, onTimeoutEvent, userData, timeout);
}

void BdtDestroyTimeout(
	BdtSystemFramework* framework, 
	BDT_SYSTEM_TIMER timer
)
{
	framework->destroyTimeout(timer);
}

void BdtImmediate(
	BdtSystemFramework* framework,
	void (BFX_STDCALL* callback)(void* userData),
	const BfxUserData* userData)
{
	framework->immediate(framework, callback, userData);
}