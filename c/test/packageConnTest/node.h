#ifndef _BDT_PACKAGE_CONNECTION_TEST_NODE_H_
#define _BDT_PACKAGE_CONNECTION_TEST_NODE_H_

#include <BDTCore/BdtCore.h>
#include <BuckyFramework/framework.h>
#include "./exchangePeerinfo.h"
#include <libuv-1.29.1/include/uv.h>

#define BDT_TEST_NODE_FIRST_BUFFER_LEN 1000
#define BDT_TEST_NODE_RECV_BUFFER_LEN 1024*1024
#define BDT_TEST_NODE_FILE_UNIT_LEN 1308 * 5000
#define BDT_TEST_NODE_FILE_UNIT_COUNT 8

struct BdtTestNodeConnection;
typedef struct BdtTestNodeFileUnit
{
	BfxListItem base;
	struct BdtTestNodeConnection* conn;
	uint8_t data[BDT_TEST_NODE_FILE_UNIT_LEN];
	size_t len;
}BdtTestNodeFileUnit;

typedef struct BdtTestNodeRecvBuffer
{
	BfxListItem base;
	uint8_t* data;
	size_t len;
}BdtTestNodeRecvBuffer;

struct BdtTestNode;
typedef struct BdtTestNodeConnection
{
	BfxListItem base;
	struct BdtTestNode* node;
	BdtConnection* connection;
	char fq[BDT_TEST_NODE_FIRST_BUFFER_LEN];
	char fa[BDT_TEST_NODE_FIRST_BUFFER_LEN];
	const BdtPeerInfo* toPeerInfo;

	//////////////send//////////////////////////////////////////////
	char filepath[MAX_PATH];
	uint64_t totalsend;
	uint64_t fileSize;

	volatile int32_t readFinish;
	BfxList freeUnits;
	BfxThreadLock freeLock;
	uv_sem_t freeSem;

	BFX_THREAD_HANDLE readFileThread;


	////////////////recv///////////////////////////////////////////////
	char recvFilePath[MAX_PATH];
	volatile int32_t whetherSize;
	uint64_t totalrecv;
	uint64_t recvFileSize;
	BFX_THREAD_HANDLE recvFileThread;
	BfxList recvBuffer;
	uv_sem_t recvSem;
	BfxThreadLock recvLock;
}BdtTestNodeConnection;

void BdtTestNodeConnectionFreeUnit(BdtTestNodeConnection* conn, BdtTestNodeFileUnit* unit);


typedef struct BdtTestNode {
	BdtStack* stack;
	BdtSystemFramework* framework;
	const BdtPeerInfo* peerInfo;

	const BdtPeerInfo* remotePeerinfo;
	BdtTestNodeConnection* conn;

	ExchangePeerInfo* exchange;
}BdtTestNode;

BdtTestNode* BdtTestNodeCreate(const char* deviceId, const char* ip, uint16_t port, const char* anotherPeerIp, bool bRemote);
const BdtPeerInfo* BdtTestNodeGetPeerInfo(BdtTestNode* node);
void BdtTestNodeAccept(BdtTestNode* node);
uint32_t BdtTestNodeSendFile(BdtTestNode* node, const char* filename);

#endif
