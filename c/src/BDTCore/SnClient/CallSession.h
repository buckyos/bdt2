#ifndef __BDT_SUPERNODE_CLIENT_CALL_SESSION_H__
#define __BDT_SUPERNODE_CLIENT_CALL_SESSION_H__

#include <BuckyBase/BuckyBase.h>

typedef struct Bdt_TcpInterface Bdt_TcpInterface;
typedef struct BdtSnClient_CallClient BdtSnClient_CallClient;
typedef struct BdtSnClient_CallSession BdtSnClient_CallSession;
typedef void (*BdtSnClient_CallSessionCallback)(
    uint32_t errorCode,
    BdtSnClient_CallSession* session,
    const BdtPeerInfo* remotePeerInfo,
    void* userData
    );

uint32_t BdtSnClient_CallSessionStart(
    BdtSnClient_CallSession* session,
    BFX_BUFFER_HANDLE payload
);

int32_t BdtSnClient_CallSessionAddRef(const BdtSnClient_CallSession* session);
int32_t BdtSnClient_CallSessionRelease(const BdtSnClient_CallSession* session);

#endif // __BDT_SUPERNODE_CLIENT_CALL_SESSION_H__