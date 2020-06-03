#ifndef _BDT_AUTO_TEST_LPC_CLIENT_H_
#define _BDT_AUTO_TEST_LPC_CLIENT_H_

#include <BDTCore/BdtCore.h>
#include <BuckyFramework/thread/bucky_thread.h>
#include <SGLib/SGLib.h>

typedef struct LpcCommand
{
	uint16_t bytesLen;
	uint8_t* buffer;
	cJSON* jsonRoot;
} LpcCommand;
typedef void (*LpcCommandHandler)(LpcCommand* command, void* data);
typedef void (*IdleHandler)(void* data);

typedef struct LpcClient LpcClient;

typedef void (*LpcClientStartedCallback)(void* data);
LpcClient* LpcClientCreate(int port, LpcClientStartedCallback cb, void* data);
void LpcClientDestory(LpcClient* client);
uint32_t LpcClientStart(LpcClient* client);
uint32_t LpcClientAddHandler(LpcClient* client, const char* name, LpcCommandHandler handler);
uint32_t LpcClientSendCommand(LpcClient* client, LpcCommand* command);
uint32_t LpcClientAddIldeHandler(LpcClient* client, IdleHandler handler);

#endif