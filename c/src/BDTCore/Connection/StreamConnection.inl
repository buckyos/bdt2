#ifndef __BDT_STREAM_CONNECTION_INL__
#define __BDT_STREAM_CONNECTION_INL__

#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "../BdtCore.h"
#include "../Tunnel/TunnelContainer.h"
#include "../Global/Module.h"


typedef struct StreamConnection StreamConnection;

static StreamConnection* StreamConnectionCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls,
	BdtConnection* owner,
	const BdtStreamConnectionConfig* config,
	Bdt_TcpInterface* tcpInterface
);


static void StreamConnectionDestroy(StreamConnection* stream);

static uint32_t StreamConnectionClose(
	StreamConnection* stream
);

static uint32_t StreamConnectionReset(
	StreamConnection* stream
);

static uint32_t StreamConnectionRecv(
	StreamConnection* stream,
	uint8_t* data,
	size_t len,
	BdtConnectionRecvCallback callback,
	const BfxUserData* userData
);

static uint32_t StreamConnectionSend(
	StreamConnection* connection,
	const uint8_t* buffer,
	size_t length,
	BdtConnectionSendCallback callback,
	const BfxUserData* userData
);

static const char* StreamConnectionGetName(StreamConnection* stream);

#endif //__BDT_STREAM_CONNECTION_H__