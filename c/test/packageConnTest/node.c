#include "./node.h"
static void _TestNodeStackEvent(BdtStack* stack, uint32_t eventId, const void* eventParams, void* userData)
{
	
}


static void BFX_STDCALL _TestNodeConnectionRecvCallback(uint8_t* data, size_t recvSize, void* userData)
{
	BdtTestNodeConnection* conn = (BdtTestNodeConnection*)userData;

	size_t recvSizeBak = recvSize;
	if (recvSize > 0)
	{
		//Sleep(100);
		BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
		uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, BDT_TEST_NODE_RECV_BUFFER_LEN);
		uint32_t errcode = BdtConnectionRecv(conn->connection, buffer, BDT_TEST_NODE_RECV_BUFFER_LEN, _TestNodeConnectionRecvCallback, &newUserData);
		if (errcode)
		{
			BFX_FREE(buffer);
		}

		size_t nCopy = 0;
		if (!BfxAtomicCompareAndSwap32(&conn->whetherSize, 0, 1))
		{
			BfxThreadLockLock(&(conn->recvLock));
			conn->recvFileSize = *((uint64_t*)data);
			BfxThreadLockUnlock(&(conn->recvLock));
			nCopy = sizeof(uint64_t);
			recvSize -= nCopy;
		}

		if (recvSize > 0)
		{
			BdtTestNodeRecvBuffer* recvBuffer = BFX_MALLOC_OBJ(BdtTestNodeRecvBuffer);
			memset(recvBuffer, 0, sizeof(BdtTestNodeRecvBuffer));
			recvBuffer->data = BFX_MALLOC_ARRAY(uint8_t, recvSize);
			recvBuffer->len = recvSize;

			memcpy(recvBuffer->data, data + nCopy, recvSize);

			BfxThreadLockLock(&(conn->recvLock));
			BfxListPushBack(&conn->recvBuffer, (BfxListItem*)recvBuffer);
			BfxThreadLockUnlock(&(conn->recvLock));
			uv_sem_post(&conn->recvSem);
		}
	}

	BFX_FREE(data);
}

static int _RecvThreadMain(void* data)
{
	BdtTestNodeConnection* conn = (BdtTestNodeConnection*)data;

	FILE* file = fopen("./testRecv", "w");
	DWORD dw = GetLastError();
	BLOG_DEBUG("=======open file finish,errcode=%d", dw);

	do 
	{
		uv_sem_wait(&conn->recvSem);
		while (true)
		{
			BfxThreadLockLock(&(conn->recvLock));
			BdtTestNodeRecvBuffer* recvBuffer = (BdtTestNodeRecvBuffer*)BfxListPopFront(&(conn->recvBuffer));
			BfxThreadLockUnlock(&(conn->recvLock));

			if (!recvBuffer)
			{
				break;
			}
			conn->totalrecv += recvBuffer->len;
			fwrite(recvBuffer->data, 1, recvBuffer->len, file);
			fflush(file);

			BFX_FREE(recvBuffer->data);
			BFX_FREE(recvBuffer);
		}

		if (BfxAtomicCompareAndSwap32(&conn->whetherSize, 1, 1) && conn->totalrecv == conn->recvFileSize)
		{
			BLOG_DEBUG("=======recv file finish,total=%llu", conn->totalrecv);
			BdtConnectionClose(conn->connection);
			break;
		}
	} while (true);
	fclose(file);

	return 0;
}

static void BFX_STDCALL _TestNodeAcceptConnectionOnConnect(BdtConnection* connection, uint32_t result, void* userData)
{
	// do nothing
	BdtTestNodeConnection* conn = (BdtTestNodeConnection*)userData;
	if (!result)
	{
		BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
		for (int i = 0; i < 1; i++)
		{
			uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, BDT_TEST_NODE_RECV_BUFFER_LEN);
			BdtConnectionRecv(conn->connection, buffer, BDT_TEST_NODE_RECV_BUFFER_LEN, _TestNodeConnectionRecvCallback, &newUserData);
		}
	}
}

static void _TestNodeAcceptConnection(uint32_t result,
	const uint8_t* buffer,
	size_t recv,
	BdtConnection* connection,
	void* userData)
{
	BdtTestNodeConnection* conn = (BdtTestNodeConnection*)userData;
	conn->connection = connection;

	BfxListInit(&conn->recvBuffer);
	BfxThreadLockInit(&conn->recvLock);
	uv_sem_init(&(conn->recvSem), 0);

	BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	/*for (int i = 0; i < 4; i++)
	{
		uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, BDT_TEST_NODE_RECV_BUFFER_LEN);
		BdtConnectionRecv(conn->connection, buffer, BDT_TEST_NODE_RECV_BUFFER_LEN, _TestNodeConnectionRecvCallback, &newUserData);
	}*/

	BdtConfirmConnection(conn->connection, NULL, 0, _TestNodeAcceptConnectionOnConnect , &newUserData);
	BLOG_DEBUG("=========accept connection");

	BfxThreadOptions options = { 0 };
	BfxCreateThread(_RecvThreadMain, conn, options, &conn->recvFileThread);
}

static void BFX_STDCALL _TestNodeConnectionSendCallback(uint32_t result,
	const uint8_t* buffer,
	size_t length,
	void* userData)
{
	BdtTestNodeFileUnit* unit = (BdtTestNodeFileUnit*)userData;
	BdtTestNodeConnection* conn = unit->conn;
	BdtTestNodeConnectionFreeUnit(conn, unit);
}


static int _ReadFileThreadMain(void* data)
{
	BdtTestNodeConnection* conn = (BdtTestNodeConnection*)data;
	FILE* file = fopen(conn->filepath, "rb");
	if (!file) 
	{
		BLOG_DEBUG("lasterror=%d, file=%s", GetLastError(), conn->filepath);
	}

	_fseeki64(file, 0, SEEK_END);
	conn->fileSize = (uint64_t)_ftelli64(file);
	BLOG_DEBUG("=====open file,size=%llu", conn->fileSize);
	BdtTestNodeFileUnit* unit = (BdtTestNodeFileUnit*)BfxListFirst(&(conn->freeUnits));
	memcpy(unit->data, (void*)(&conn->fileSize), sizeof(uint64_t));
	unit->len = sizeof(uint64_t);
	_fseeki64(file, 0, SEEK_SET);

	do
	{
		uv_sem_wait(&(conn->freeSem));

		while (true)
		{
			BfxThreadLockLock(&(conn->freeLock));
			BdtTestNodeFileUnit* unit = (BdtTestNodeFileUnit*)BfxListPopFront(&(conn->freeUnits));
			BfxThreadLockUnlock(&(conn->freeLock));
			if (!unit)
			{
				break;
			}

			unit->conn = conn;
			size_t nread = fread(unit->data + unit->len, 1, BDT_TEST_NODE_FILE_UNIT_LEN - unit->len, file);
			unit->len += nread;
			if (unit->len)
			{
				BfxUserData newUserData = { .userData = unit,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
				uint32_t errcode = BdtConnectionSend(conn->connection, unit->data, unit->len, _TestNodeConnectionSendCallback, &newUserData);
				//BLOG_DEBUG("unit data len=%lu, errcode=%u", unit->len, errcode);
			}
			else
			{
				BLOG_DEBUG("===unit data len=0");
				BfxThreadLockLock(&(conn->freeLock));
				BfxListPushBack(&(conn->freeUnits), (BfxListItem*)unit);
				BfxThreadLockUnlock(&(conn->freeLock));
			}
			if (unit->len < BDT_TEST_NODE_FILE_UNIT_LEN || !nread)
			{
				BLOG_DEBUG("-=-=============read finish");
				BfxAtomicExchange32(&conn->readFinish, 1);
				BdtConnectionClose(conn->connection);
				break;
			}
		}

		if (BfxAtomicCompareAndSwap32(&conn->readFinish, 1, 1))
		{
			break;
		}
	} while (true);

	fclose(file);

	return 0;
}

static void _TestNodeConnectionEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	if (result == BFX_RESULT_SUCCESS)
	{
		BdtTestNodeConnection* conn = (BdtTestNodeConnection*)userData;
		BLOG_DEBUG("test node connection establish");

		BfxThreadOptions options = { 0 };
		BfxCreateThread(_ReadFileThreadMain, conn, options, &conn->readFileThread);
	}
}

void BdtTestNodeConnectionFreeUnit(BdtTestNodeConnection* conn, BdtTestNodeFileUnit* unit)
{
	BfxThreadLockLock(&(conn->freeLock));
	conn->totalsend += unit->len;
	if (conn->totalsend == conn->fileSize + sizeof(uint64_t))
	{
		BLOG_DEBUG("============send file finish, totalsend=%llu", conn->totalsend - sizeof(uint64_t));
	}
	memset(unit, 0, sizeof(BdtTestNodeFileUnit));
	BfxListPushBack(&conn->freeUnits, (BfxListItem*)unit);
	BfxThreadLockUnlock(&(conn->freeLock));
	uv_sem_post(&(conn->freeSem));
}

BdtTestNode* BdtTestNodeCreate(const char* deviceId, const char* ip, uint16_t port, const char* anotherPeerIp, bool bRemote)
{
	BdtTestNode* node = BFX_MALLOC_OBJ(BdtTestNode);
	memset(node, 0, sizeof(BdtTestNode));

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
	BdtCreatePeerid(&area, deviceId, BDT_PEER_PUBLIC_KEY_TYPE_RSA1024, &peerid, &peerConst, &peerSecret);

	BdtEndpoint ep;
	char str[64] = { 0 };
	sprintf(str, "v4udp%s:%d", ip, port);
	BdtEndpointFromString(&ep, str);
	ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;

	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET | BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
	BdtPeerInfoSetConstInfo(builder, &peerConst);
	BdtPeerInfoSetSecret(builder, &peerSecret);
	BdtPeerInfoAddEndpoint(builder, &ep);
	node->peerInfo = BdtPeerInfoBuilderFinish(builder, &peerSecret);

	// node->exchange = (ExchangePeerInfo*)ShareMemoryExchangePeerInfoCreate();
	node->exchange = (ExchangePeerInfo*)NetworkExchangePeerInfoCreate(ip, anotherPeerIp);
	if (bRemote)
	{
		node->exchange->sendPeerInfoTo(node->exchange, node->peerInfo);
	}
	else
	{
		node->remotePeerinfo = node->exchange->getPeerInfoFrom(node->exchange);
	}
	
	BuckyFrameworkOptions options = {
	    .size = sizeof(BuckyFrameworkOptions),
        .tcpThreadCount = 4,
        .udpThreadCount = 4,
        .timerThreadCount = 4,
        .dispatchThreadCount = 4
	};

	BdtSystemFrameworkEvents events = {
		.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent,
		.bdtPushTcpSocketData = BdtPushTcpSocketData,
		.bdtPushUdpSocketData = BdtPushUdpSocketData,
	};

	node->framework = createBuckyFramework(&events, &options);
	BfxUserData userData = {
		.userData = node,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};
	if (node->remotePeerinfo)
	{
		BdtCreateStack(node->framework, node->peerInfo, &node->remotePeerinfo, 1, NULL, _TestNodeStackEvent, &userData, &(node->stack));
	}
	else
	{
		BdtCreateStack(node->framework, node->peerInfo, NULL, 0, NULL, _TestNodeStackEvent, &userData, &(node->stack));
	}
	node->framework->stack = node->stack;
	BLOG_DEBUG("node->framework = 0x%lx, stack=0x%lx", node->framework, node->stack);

	BdtListenConnection(node->stack, 0, NULL);

	return node;
}

const BdtPeerInfo* BdtTestNodeGetPeerInfo(BdtTestNode* node)
{
	return node->peerInfo;
}

void BdtTestNodeAccept(BdtTestNode* node)
{
	BdtTestNodeConnection* conn = BFX_MALLOC_OBJ(BdtTestNodeConnection);
	conn->node = node;
	node->conn = conn;
	memset(conn, 0, sizeof(BdtTestNodeConnection));

	BfxUserData userData = {
		.userData = conn,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BdtAcceptConnection(node->stack, 0, _TestNodeAcceptConnection, &userData);
}

uint32_t BdtTestNodeSendFile(BdtTestNode* node,const char* filename)
{
	BdtTestNodeConnection * conn = BFX_MALLOC_OBJ(BdtTestNodeConnection);
	node->conn = conn;
	memset(conn, 0, sizeof(BdtTestNodeConnection));
	conn->node = node;
	conn->toPeerInfo = node->remotePeerinfo;
	BfxListInit(&(conn->freeUnits));
	BfxThreadLockInit(&(conn->freeLock));
	uv_sem_init(&(conn->freeSem), 1);
	if (filename)
	{
		for (int i = 0 ; i < BDT_TEST_NODE_FILE_UNIT_COUNT; i++)
		{
			BdtTestNodeFileUnit* unit = BFX_MALLOC_OBJ(BdtTestNodeFileUnit);
			memset(unit, 0, sizeof(BdtTestNodeFileUnit));
			BfxListPushBack(&(conn->freeUnits), (BfxListItem*)unit);
		}
		strcpy(conn->filepath, filename);
	}

	sprintf(conn->fq, "firstQ");

	BfxUserData userData = {
		.userData = conn,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BdtBuildTunnelParams params;
	memset(&params, 0, sizeof(BdtBuildTunnelParams));
	params.remoteConstInfo = BdtPeerInfoGetConstInfo(conn->node->remotePeerinfo);
	size_t fqLen = 0;
	BdtCreateConnection(
		node->stack,
		BdtPeerInfoGetPeerid(conn->toPeerInfo),
		true,
		&(conn->connection)
	);


	BdtConnectionConnect(
		conn->connection,
		0,
		&params,
		NULL,
		0,
		_TestNodeConnectionEvent,
		&userData, 
		NULL,
		NULL
	);

	return 0;
}
