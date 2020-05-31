#include "./demo.h"
#include "./conn.h"
#include <libuv-1.29.1/include/uv.h>
#include "./lpc_client.h"
#include "sys_res.h"

#define TEST_DEMO_ID_LEN 64
#define TEST_DEMO_KEEP_LIVE_TIME_OUT 10*1000
struct TestDemo
{
	BdtStack* stack;
	LpcClient* client;
	BfxThreadLock connListLock;
	BfxList connList;
	char name[TEST_DEMO_ID_LEN];
	const BdtPeerInfo* peerInfo;
	const BdtPeerInfo* remotePeerInfo;
	const BdtPeerInfo* snPeerInfo;
	uv_sem_t exitSem;
	int64_t latestRecvTime;
};

LpcCommand* NewTestDemoCommand(TestDemo* demo, const char* name)
{
	LpcCommand* command = BFX_MALLOC_OBJ(LpcCommand);
	memset(command, 0, sizeof(LpcCommand));

	command->jsonRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(command->jsonRoot, "name", cJSON_CreateString(name));
	cJSON_AddItemToObject(command->jsonRoot, "peerName", cJSON_CreateString(demo->name));

	return command;
}

void OnBeginConnect(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	BdtPeerInfo* remotePeerInfo = BdtPeerInfoCreateForTest();
	BfxBufferStream stream;
	BfxBufferStreamInit(&stream, (void*)command->buffer, command->bytesLen);
	BdtPeerInfoDecodeForTest(&stream, remotePeerInfo);
	
	cJSON* nameObj = cJSON_GetObjectItem(command->jsonRoot, "connName");
	assert(nameObj);
	char* connName = cJSON_GetStringValue(nameObj);
	TestDemoConnection* conn = TestDemoConnectionCreate(demo, connName, remotePeerInfo, !!demo->remotePeerInfo);
	
	BfxThreadLockLock(&demo->connListLock);
	BfxListPushBack(&demo->connList, (BfxListItem*)conn);
	BfxThreadLockUnlock(&demo->connListLock);

	TestDemoConnectionConnect(conn);

	BLOG_INFO("on begin connect finish");
}

void OnCloseConnection(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	cJSON* connNameObj = cJSON_GetObjectItem(command->jsonRoot, "connName");
	assert(connNameObj);
	char* connName = cJSON_GetStringValue(connNameObj);

	TestDemoConnection* conn = NULL;

	BfxThreadLockLock(&demo->connListLock);
	conn = (TestDemoConnection*)BfxListFirst(&demo->connList);
	do
	{
		if (conn)
		{
			break;
		}
		if (strcmp(TestDemoConnectionGetName(conn), connName) == 0)
		{
			break;
		}

		conn = (TestDemoConnection*)BfxListNext(&demo->connList, (BfxListItem*)conn);
	} while (true);
	BfxThreadLockUnlock(&demo->connListLock);

	uint32_t errcode = BFX_RESULT_SUCCESS;
	do 
	{
		if (!conn)
		{
			errcode = BFX_RESULT_NOT_FOUND;
			break;
		}
		errcode = TestDemoConnectionClose(conn);
	} while (false);

	LpcCommand* commandResp = NewTestDemoCommand(demo, "closeConnectionResp");
	cJSON_AddItemToObject(commandResp->jsonRoot, "connName", cJSON_CreateString(connName));
	cJSON_AddItemToObject(commandResp->jsonRoot, "errcode", cJSON_CreateNumber(errcode));

	LpcClientSendCommand(demo->client, commandResp);

	BLOG_INFO("on close connection");
}

void OnResetConnection(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	cJSON* connNameObj = cJSON_GetObjectItem(command->jsonRoot, "connName");
	assert(connNameObj);
	char* connName = cJSON_GetStringValue(connNameObj);

	TestDemoConnection* conn = NULL;

	BfxThreadLockLock(&demo->connListLock);
	conn = (TestDemoConnection*)BfxListFirst(&demo->connList);
	do
	{
		if (conn)
		{
			break;
		}
		if (strcmp(TestDemoConnectionGetName(conn), connName) == 0)
		{
			break;
		}

		conn = (TestDemoConnection*)BfxListNext(&demo->connList, (BfxListItem*)conn);
	} while (true);
	BfxThreadLockUnlock(&demo->connListLock);

	uint32_t errcode = BFX_RESULT_SUCCESS;
	do
	{
		if (!conn)
		{
			errcode = BFX_RESULT_NOT_FOUND;
			break;
		}
		errcode = TestDemoConnectionReset(conn);
	} while (false);

	LpcCommand* commandResp = NewTestDemoCommand(demo, "resetConnectionResp");
	cJSON_AddItemToObject(commandResp->jsonRoot, "connName", cJSON_CreateString(connName));
	cJSON_AddItemToObject(commandResp->jsonRoot, "errcode", cJSON_CreateNumber(errcode));

	LpcClientSendCommand(demo->client, commandResp);

	BLOG_INFO("on reset connection");
}

void OnGetConnectionStat(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	cJSON* connNameObj = cJSON_GetObjectItem(command->jsonRoot, "connName");
	assert(connNameObj);
	char* connName = cJSON_GetStringValue(connNameObj);

	TestDemoConnection* conn = NULL;

	BfxThreadLockLock(&demo->connListLock);
	conn = (TestDemoConnection*)BfxListFirst(&demo->connList);
	do
	{
		if (conn)
		{
			break;
		}
		if (strcmp(TestDemoConnectionGetName(conn), connName) == 0)
		{
			break;
		}

		conn = (TestDemoConnection*)BfxListNext(&demo->connList, (BfxListItem*)conn);
	} while (true);
	BfxThreadLockUnlock(&demo->connListLock);

	LpcCommand* commandResp = NewTestDemoCommand(demo, "getConnectionStatResp");
	cJSON_AddItemToObject(commandResp->jsonRoot, "connName", cJSON_CreateString(connName));
	cJSON* statObj = cJSON_CreateObject();
	cJSON_AddItemToObject(commandResp->jsonRoot, "stat", statObj);
	uint32_t errcode = BFX_RESULT_SUCCESS;
	do
	{
		if (!conn)
		{
			errcode = BFX_RESULT_NOT_FOUND;
			break;
		}
		cJSON_AddItemToObject(statObj, "sendSpeed", cJSON_CreateNumber(TestDemoConnectionGetSendSpeed(conn)));
	} while (false);
	cJSON_AddItemToObject(commandResp->jsonRoot, "errcode", cJSON_CreateNumber(errcode));

	LpcClientSendCommand(demo->client, commandResp);

	BLOG_INFO("on get connection stat");
}

void OnSendFile(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	cJSON* connNameObj = cJSON_GetObjectItem(command->jsonRoot, "connName");
	assert(connNameObj);
	char* connName = cJSON_GetStringValue(connNameObj);

	cJSON* fileNameObj = cJSON_GetObjectItem(command->jsonRoot, "fileName");
	assert(fileNameObj);
	char* fileName = cJSON_GetStringValue(fileNameObj);

	cJSON* fileSizeObj = cJSON_GetObjectItem(command->jsonRoot, "fileSize");
	assert(fileNameObj);
#if defined(BFX_OS_WIN)
	uint64_t fileSize = _atoi64(cJSON_GetStringValue(fileSizeObj));
#else
	uint64_t fileSize = (uint64_t)atoll(cJSON_GetStringValue(fileSizeObj));
#endif

	TestDemoConnection* conn = NULL;

	BfxThreadLockLock(&demo->connListLock);
	conn = (TestDemoConnection*)BfxListFirst(&demo->connList);
	do 
	{
		if (!conn)
		{
			break;
		}
		if (strcmp(TestDemoConnectionGetName(conn), connName) == 0)
		{
			break;
		}

		conn = (TestDemoConnection*)BfxListNext(&demo->connList, (BfxListItem*)conn);
	} while (true);
	BfxThreadLockUnlock(&demo->connListLock);

	uint32_t errcode = 1;
	if (conn)
	{
		errcode = TestDemoConnectionSendFile(conn, fileSize, fileName);
	}

	LpcCommand* commandResp = NewTestDemoCommand(demo, "sendFileResp");
	cJSON_AddItemToObject(commandResp->jsonRoot, "errcode", cJSON_CreateNumber(errcode));
	cJSON_AddItemToObject(commandResp->jsonRoot, "connName", cJSON_CreateString(connName));

	LpcClientSendCommand(demo->client, commandResp);

	BLOG_INFO("on send file");
}

void OnResourceUsage(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

    int64_t memBytes = 0;
    int64_t vmemBytes = 0;
    uint32_t handleCount = 0;
    uint32_t errcode = SysResUsageQuery(&memBytes, &vmemBytes, &handleCount);

	LpcCommand* commandResp = NewTestDemoCommand(demo, "resUsageResp");
    cJSON_AddItemToObject(commandResp->jsonRoot, "errcode", cJSON_CreateNumber(errcode));
	cJSON_AddItemToObject(commandResp->jsonRoot, "mem", cJSON_CreateNumber((double)memBytes));
    cJSON_AddItemToObject(commandResp->jsonRoot, "vmem", cJSON_CreateNumber((double)vmemBytes));
    cJSON_AddItemToObject(commandResp->jsonRoot, "handle", cJSON_CreateNumber(handleCount));

	LpcClientSendCommand(demo->client, commandResp);
}

void OnExit(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;
	BLOG_INFO("recv exit command");
	uv_sem_post(&demo->exitSem);
}

void OnPing(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;
	demo->latestRecvTime = BfxTimeGetNow(false) / 1000;

	LpcCommand* commandResp = NewTestDemoCommand(demo, "ping");

	LpcClientSendCommand(demo->client, commandResp);
	BLOG_INFO("on ping and send ping");
}

static void _TestDemoStackEventCB(BdtStack* stack, uint32_t eventId, const void* eventParams, void* userData)
{

}

static void BFX_STDCALL _TestDemoAcceptConnectionOnConnect(BdtConnection* connection, uint32_t result, void* userData)
{
	TestDemo* demo = (TestDemo*)userData;
	TestDemoConnection* conn = NULL;
	BfxThreadLockLock(&demo->connListLock);
	conn = (TestDemoConnection*)BfxListFirst(&demo->connList);
	while (conn)
	{
		if (TestDemoConnectionIsEqual(conn, connection)) 
		{
			break;
		}

		conn = (TestDemoConnection*)BfxListNext(&demo->connList, (BfxListItem*)conn);
	}
	BfxThreadLockUnlock(&demo->connListLock);

	if (!conn)
	{
		BLOG_ERROR("not found test demo connection");
		return;
	}
	
	BLOG_INFO("begin add  recv buffer on confire callback");
	TestDemoConnectionBeginRecv(conn);
}

static void _TestDemoAcceptConnectionCB(uint32_t result,
	const uint8_t* buffer,
	size_t recv,
	BdtConnection* connection,
	void* userData)
{
	TestDemo* demo = (TestDemo*)userData;

	char name[64] = { 0 };
	sprintf(name, "%lld", BfxTimeGetNow(false) / 1000);
	BLOG_DEBUG("accept new connection, name=%s", name);
	TestDemoConnection* conn = TestDemoConnectionCreateFromBdtConnection(demo, connection, name);
	BfxThreadLockLock(&demo->connListLock);
	BfxListPushBack(&demo->connList, (BfxListItem*)conn);
	BfxThreadLockUnlock(&demo->connListLock);

	BfxUserData newUserData = { .userData = demo,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BdtConfirmConnection(connection, NULL, 0, _TestDemoAcceptConnectionOnConnect, &newUserData);

	LpcCommand* command = NewTestDemoCommand(demo, "newConnection");
	cJSON_AddItemToObject(command->jsonRoot, "connName", cJSON_CreateString(name));


	LpcClientSendCommand(demo->client, command);
}

void HexToBinary(const char* hex, uint8_t* binary, size_t* outLen)
{
	size_t i = 0;
	for (; hex[i] != '\0'; i += 2)
	{
		if (hex[i] >= '0' && hex[i] <= '9')
		{
			binary[i / 2] = (uint8_t)(hex[i] - '0');
		}
		else if (hex[i] >= 'a' && hex[i] <= 'f')
		{
			binary[i / 2] = (uint8_t)(hex[i] - 'a') + 10;
		}
		else if (hex[i] >= 'A' && hex[i] <= 'F')
		{
			binary[i / 2] = (uint8_t)(hex[i] - 'A') + 10;
		}
		else
		{
			assert(false);
		}

		binary[i / 2] <<= 4;
		if (hex[i + 1] >= '0' && hex[i + 1] <= '9')
		{
			binary[i / 2] += (uint8_t)(hex[i + 1] - '0');
		}
		else if (hex[i + 1] >= 'a' && hex[i + 1] <= 'f')
		{
			binary[i / 2] += (uint8_t)(hex[i + 1] - 'a') + 10;
		}
		else if (hex[i + 1] >= 'A' && hex[i + 1] <= 'F')
		{
			binary[i / 2] += (uint8_t)(hex[i + 1] - 'A') + 10;
		}
		else
		{
			assert(false);
		}
	}

	*outLen = i / 2;
}

const BdtPeerInfo* CreateSnPeerInfo(bool bLocalSn)
{
	BdtPeerConstInfo snPeerConstInfo = {
		{0, 0, 0, 0, 0},
		{'b', 'u', 'c', 'k', 'y', '-', 's', 'n', '-', 's', 'e', 'r', 'v', 'e', 'r', '\0'},
		BDT_PEER_PUBLIC_KEY_TYPE_RSA1024,
		{
			{'\0'}
		}
	};

	BdtEndpoint snEndpointUdp = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 10020,
		.address = {106, 75, 175, 123}
	};
	BdtEndpoint snEndpointTcp = {
		.flag = BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 10030,
		.address = {106, 75, 175, 123}
	};

	if (bLocalSn)
	{
		snEndpointUdp.address[0] = 192;
		snEndpointUdp.address[1] = 168;
		snEndpointUdp.address[2] = 100;
		snEndpointUdp.address[3] = 51;

		snEndpointTcp.address[0] = 192;
		snEndpointTcp.address[1] = 168;
		snEndpointTcp.address[2] = 100;
		snEndpointTcp.address[3] = 51;
	}
	size_t publicKeyLength = 0;
	HexToBinary("30819f300d06092a864886f70d010101050003818d0030818902818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a50203010001",
		snPeerConstInfo.publicKey.rsa1024, &publicKeyLength);

	BdtPeerInfoBuilder* infoBuilder = BdtPeerInfoBuilderBegin(0);
	BdtPeerInfoSetConstInfo(infoBuilder, &snPeerConstInfo);
	BdtPeerInfoAddEndpoint(infoBuilder, &snEndpointUdp);
	BdtPeerInfoAddEndpoint(infoBuilder, &snEndpointTcp);
	const BdtPeerInfo* snPeerinfo = BdtPeerInfoBuilderFinish(infoBuilder, NULL);

	return snPeerinfo;
}

void OnCreateStack(LpcCommand* command, void* data)
{
	TestDemo* demo = (TestDemo*)data;

	if (command->bytesLen && command->buffer)
	{
		BdtPeerInfo* remotePeerInfo = BdtPeerInfoCreateForTest();
		BfxBufferStream stream;
		BfxBufferStreamInit(&stream, (void*)command->buffer, command->bytesLen);
		BdtPeerInfoDecodeForTest(&stream, remotePeerInfo);
		demo->remotePeerInfo = remotePeerInfo;
	}

	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET | BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST);
	BdtPeerArea area = {
		.continent = 0,
		.country = 0,
		.carrier = 0,
		.city = 0,
		.inner = 0
	};

	BdtPeerid peerid = { 0 };
	BdtPeerConstInfo peerConst = { 0 };
	BdtPeerSecret peerSecret = { 0 };
	BdtCreatePeerid(&area, demo->name, BDT_PEER_PUBLIC_KEY_TYPE_RSA1024, &peerid, &peerConst, &peerSecret);
	BdtPeerInfoSetConstInfo(builder, &peerConst);
	BdtPeerInfoSetSecret(builder, &peerSecret);

	/*BdtEndpoint ep;
	BdtEndpointFromString(&ep, "v4udp192.168.100.124:10000");
	ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
	BdtPeerInfoAddEndpoint(builder, &ep);*/

	cJSON* addrInfoObj = cJSON_GetObjectItem(command->jsonRoot, "addrInfo");
	if (addrInfoObj)
	{
		assert(cJSON_IsArray(addrInfoObj));
		int index = 0;
		while (true)
		{
			cJSON* addrObj = cJSON_GetArrayItem(addrInfoObj, index);
			if (!addrObj)
			{
				break;
			}

			char* str = cJSON_GetStringValue(addrObj);
			BdtEndpoint ep;
			BdtEndpointFromString(&ep, str+1);
			if (strncmp(str, "1", 1) == 0) {
				ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
			}

			BdtPeerInfoAddEndpoint(builder, &ep);
			index++;
		}
	}

	do 
	{
		bool bUseSn = false;
		cJSON* snObj = cJSON_GetObjectItem(command->jsonRoot, "sn");
		if (snObj)
		{
			if (strcmp(cJSON_GetStringValue(snObj), "1") == 0)
			{
				BLOG_INFO("=========== create sn");
				demo->snPeerInfo = CreateSnPeerInfo(true);
				BdtPeerInfoAddSn(builder, BdtPeerInfoGetPeerid(demo->snPeerInfo));
			}
			else if (strcmp(cJSON_GetStringValue(snObj), "2") == 0)
			{
				BLOG_INFO("=========== create sn");
				demo->snPeerInfo = CreateSnPeerInfo(false);
				BdtPeerInfoAddSn(builder, BdtPeerInfoGetPeerid(demo->snPeerInfo));
			}
		}

		////如果不认识remotepeerinfo，那么必须用sn
		//if (!bUseSn && !demo->remotePeerInfo)
		//{
		//	bUseSn = true;
		//}

		/*if (!bUseSn)
		{
			break;
		}

		BLOG_INFO("=========== create sn");
		demo->snPeerInfo = CreateSnPeerInfo();
		BdtPeerInfoAddSn(builder, BdtPeerInfoGetPeerid(demo->snPeerInfo));*/
	} while (false);

	demo->peerInfo = BdtPeerInfoBuilderFinish(builder, &peerSecret);

	const BdtPeerid* pPeerid = BdtPeerInfoGetPeerid(demo->peerInfo);
	char strPeerid[128];
	BdtPeeridToString(pPeerid, strPeerid);
	BLOG_INFO("===========peerid=%s, name=%s", strPeerid, demo->name);

	BuckyFrameworkOptions options = {
	.size = sizeof(BuckyFrameworkOptions),
	.tcpThreadCount = 1,
		.udpThreadCount = 1,
		.timerThreadCount = 1,
		.dispatchThreadCount = 1,
	};

	BdtSystemFrameworkEvents events = {
		.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent,
		.bdtPushTcpSocketData = BdtPushTcpSocketData,
		.bdtPushUdpSocketData = BdtPushUdpSocketData,
	};

	BdtSystemFramework* framework = createBuckyFramework(&events, &options);
	BfxUserData userData = {
		.userData = demo,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	const BdtPeerInfo* initPeers[2] = { 0 };
	size_t initCount = 0;
	if (demo->snPeerInfo) 
	{
		initPeers[initCount] = demo->snPeerInfo;
		char strPeerid[128] = { 0 };
		BdtPeeridToString(BdtPeerInfoGetPeerid(demo->snPeerInfo), strPeerid);
		BLOG_DEBUG("==============self sn peerid=%s", strPeerid);
		initCount++;
	}
	if (demo->remotePeerInfo)
	{
		initPeers[initCount] = demo->remotePeerInfo;
		size_t count = 0;
		const BdtPeerid* ids = BdtPeerInfoListSn(demo->remotePeerInfo, &count);
		BLOG_DEBUG("==============remote peerinfo has '%d' sn", count);
		for (size_t i = 0; i < count; i++)
		{
			char strPeerid[128] = { 0 };
			BdtPeeridToString(ids + i, strPeerid);
			BLOG_DEBUG("==============%i remote peer sn id=%s", i, strPeerid);
		}
		initCount++;
	}

	BdtCreateStack(framework, demo->peerInfo, initPeers, initCount, NULL, _TestDemoStackEventCB, &userData, &(demo->stack));
	framework->stack = demo->stack;

	BdtListenConnection(demo->stack, 0, NULL);

	//投递20个accept
	for (int i = 0; i < 20; i++)
	{
		BdtAcceptConnection(demo->stack, 0, _TestDemoAcceptConnectionCB, &userData);
	}

	LpcCommand* commandResp = NewTestDemoCommand(demo, "createStackResp");
	uint8_t* peerinfo = BFX_MALLOC_ARRAY(uint8_t, 1024 * 3);
	BfxBufferStream stream;
	BfxBufferStreamInit(&stream, (void*)peerinfo, 1024 * 3);
	BdtPeerInfoEncodeForTest(demo->peerInfo, &stream);

	uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, 1024 * 3);
	BfxBufferStream stream1;
	BfxBufferStreamInit(&stream1, (void*)buffer, 1024 * 3);
	size_t nWrite = 0;
	BfxBufferWriteUInt16(&stream1, (uint16_t)stream.pos, &nWrite);
	BfxBufferWriteByteArray(&stream1, peerinfo, stream.pos);
	BFX_FREE(peerinfo);
	BfxBufferWriteUInt16(&stream1, sizeof(BdtPeerid), &nWrite);
	BfxBufferWriteByteArray(&stream1, (const uint8_t*)(pPeerid), sizeof(BdtPeerid));

	
	commandResp->bytesLen = (uint16_t)stream1.pos;
	commandResp->buffer = buffer;

	LpcClientSendCommand(demo->client, commandResp);
}

void LpcClientStartedCB(void* data)
{
	TestDemo* demo = (TestDemo*)data;
	LpcCommand* command = NewTestDemoCommand(demo, "started");

	LpcClientSendCommand(demo->client, command);
}

TestDemo* TestDemoCreate(uint16_t port, const char* id)
{
	TestDemo* demo = BFX_MALLOC_OBJ(TestDemo);
	memset(demo, 0, sizeof(TestDemo));
	strcpy(demo->name, id);
	demo->client = LpcClientCreate(port, LpcClientStartedCB, demo);

	BfxListInit(&demo->connList);
	BfxThreadLockInit(&demo->connListLock);

	uv_sem_init(&demo->exitSem, 0);

    SysResQueryEnable();

	LpcClientAddHandler(demo->client, "beginConnect", OnBeginConnect);
	LpcClientAddHandler(demo->client, "closeConnection", OnCloseConnection);
	LpcClientAddHandler(demo->client, "resetConnection", OnResetConnection);
	LpcClientAddHandler(demo->client, "createStack", OnCreateStack);
	LpcClientAddHandler(demo->client, "sendFile", OnSendFile);
	LpcClientAddHandler(demo->client, "exit", OnExit);
	LpcClientAddHandler(demo->client, "ping", OnPing);
	LpcClientAddHandler(demo->client, "resUsage", OnResourceUsage);
	LpcClientAddHandler(demo->client, "getConnectionStat", OnGetConnectionStat);

	return demo;
}

BdtStack* TestDemoGetBdtStack(TestDemo* stack)
{
	return stack->stack;
}

const BdtPeerInfo* TestDemoGetSnPeerInfo(TestDemo* stack)
{
	return stack->snPeerInfo;
}

void OnIdle(void* data)
{
	TestDemo* demo = (TestDemo*)data;
	if (!demo->latestRecvTime) 
	{
		demo->latestRecvTime = BfxTimeGetNow(false) / 1000;
		return;
	}

	if ((BfxTimeGetNow(false) / 1000 - demo->latestRecvTime) > TEST_DEMO_KEEP_LIVE_TIME_OUT)
	{
		BLOG_INFO("not recv ping, exit, time=%d", TEST_DEMO_KEEP_LIVE_TIME_OUT);
		uv_sem_post(&demo->exitSem);
	}
}

void TestDemoRun(TestDemo* stack)
{
	LpcClientStart(stack->client);
	LpcClientAddIldeHandler(stack->client, OnIdle);

	uv_sem_wait(&stack->exitSem);
}

void TestDemoOnConnectionFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, uint64_t time)
{
	TestDemoConnectionBeginRecv(conn);

	LpcCommand* command = NewTestDemoCommand(demo, "connectCallback");
	cJSON_AddItemToObject(command->jsonRoot, "connName", cJSON_CreateString(TestDemoConnectionGetName(conn)));
	cJSON_AddItemToObject(command->jsonRoot, "errcode", cJSON_CreateNumber(errcode));
	cJSON_AddItemToObject(command->jsonRoot, "time", cJSON_CreateNumber((double)time));

	LpcClientSendCommand(demo->client, command);

	BLOG_INFO("connect finish and send command");
}

void TestDemoOnConnectionRecvFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, const char* name, uint64_t totaltime)
{
	LpcCommand* command = NewTestDemoCommand(demo, "onRecv");
	cJSON_AddItemToObject(command->jsonRoot, "connName", cJSON_CreateString(TestDemoConnectionGetName(conn)));
	cJSON_AddItemToObject(command->jsonRoot, "errcode", cJSON_CreateNumber(errcode));
	cJSON_AddItemToObject(command->jsonRoot, "fileName", cJSON_CreateString(name));
	cJSON_AddItemToObject(command->jsonRoot, "time", cJSON_CreateNumber((double)totaltime));

	LpcClientSendCommand(demo->client, command);
	BLOG_INFO("recv finish and send command");
}

void TestDemoOnConnectionSendFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, const char* name, uint64_t totaltime)
{
	LpcCommand* command = NewTestDemoCommand(demo, "onSend");
	cJSON_AddItemToObject(command->jsonRoot, "connName", cJSON_CreateString(TestDemoConnectionGetName(conn)));
	cJSON_AddItemToObject(command->jsonRoot, "errcode", cJSON_CreateNumber(errcode));
	cJSON_AddItemToObject(command->jsonRoot, "fileName", cJSON_CreateString(name));
	cJSON_AddItemToObject(command->jsonRoot, "time", cJSON_CreateNumber((double)totaltime));

	LpcClientSendCommand(demo->client, command);
	BLOG_INFO("send finish and send command");
}
