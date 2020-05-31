#ifndef __BDT_TUNNEL_TUNNEL_BUILDER_H__
#define __BDT_TUNNEL_TUNNEL_BUILDER_H__
#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "../Interface/InterfaceModule.h"

typedef struct Bdt_TunnelBuilder Bdt_TunnelBuilder;

int32_t Bdt_TunnelBuilderAddRef(Bdt_TunnelBuilder* builder);
int32_t Bdt_TunnelBuilderRelease(Bdt_TunnelBuilder* builder);

uint32_t Bdt_TunnelBuilderBuildForConnectConnection(
	Bdt_TunnelBuilder* builder,
	BdtConnection* connection
);

uint32_t Bdt_TunnelBuilderBuildForAcceptConnection(
	Bdt_TunnelBuilder* builder,
	BdtConnection* connection
);

uint32_t Bdt_TunnelBuilderOnTcpFistPackage(Bdt_TunnelBuilder* builder, Bdt_TcpInterface* tcpInterface, const Bdt_Package* package);
uint32_t Bdt_TunnelBuilderOnSessionData(Bdt_TunnelBuilder* builder, const Bdt_SessionDataPackage* sessionData);

#endif //__BDT_TUNNEL_TUNNEL_BUILDER_H__