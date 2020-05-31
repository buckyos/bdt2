#ifndef __BDT_STACK_TLS_H__
#define __BDT_STACK_TLS_H__
#include "../BdtSystemFramework.h"
#include "../Protocol/Package.h"

typedef struct Bdt_StackTls
{
	BdtSystemFramework* framework;
	BfxTlsKey slot;
} Bdt_StackTls;

typedef struct Bdt_StackTlsData
{
	uint8_t udpEncodeBuffer[BDT_UDP_SOCKET_MTU];
	uint8_t udpDecryptBuffer[BDT_PROTOCOL_MAX_UDP_DATA_SIZE];
	uint8_t tcpEncodeBuffer[BDT_PROTOCOL_MAX_TCP_DATA_SIZE];
} Bdt_StackTlsData;

uint32_t Bdt_StackTlsInit(Bdt_StackTls* tls);
void Bdt_StackTlsUninit(Bdt_StackTls* tls);
Bdt_StackTlsData* Bdt_StackTlsGetData(Bdt_StackTls* tls);

#endif //__BDT_STACK_TLS_H__