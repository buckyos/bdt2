#ifndef __BDT_UDP_INTERFACE_INL__
#define __BDT_UDP_INTERFACE_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__
#include "./UdpInterface.h"

struct Bdt_UdpInterface
{
	BdtSystemFramework* framework;
	BDT_SYSTEM_UDP_SOCKET socket;
	BdtEndpoint localEndpoint;
	int32_t refCount;
};

typedef Bdt_UdpInterface UdpInterface;

#endif //__BDT_UDP_INTERFACE_INL__