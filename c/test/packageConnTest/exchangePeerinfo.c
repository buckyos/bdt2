#include "./exchangePeerinfo.h"

static void ShareMemorySendPeerInfoTo(ExchangePeerInfo* exchange, const BdtPeerInfo* peerinfo)
{
	ShareMemoryExchangePeerInfo* shareExchange = (ShareMemoryExchangePeerInfo*)exchange;
	shareExchange->hRemoteEvent = CreateEventA(NULL, FALSE, FALSE, "testnode_remote_event");
	if (shareExchange->hRemoteEvent == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		shareExchange->hRemoteEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "testnode_remote_event");
	}

	shareExchange->hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, TEST_NODE_PEERINFO_LEN, "test_node_peerinfo");
	if (shareExchange->hMap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		shareExchange->hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "test_node_peerinfo");
	}

	SYSTEM_INFO stSysInfo = { 0 };
	GetSystemInfo(&stSysInfo);

	uint8_t* peerInfoBuffer = BFX_MALLOC_ARRAY(uint8_t, TEST_NODE_PEERINFO_LEN);
	BfxBufferStream stream;
	BfxBufferStreamInit(&stream, (void*)peerInfoBuffer, TEST_NODE_PEERINFO_LEN);

	BdtPeerInfoEncodeForTest(peerinfo, &stream);
	//Ð´Èë¹²ÏíÄÚ´æ

	size_t total = 0;
	while (total < TEST_NODE_PEERINFO_LEN)
	{
		size_t size = stSysInfo.dwAllocationGranularity < TEST_NODE_PEERINFO_LEN - total ? stSysInfo.dwAllocationGranularity : TEST_NODE_PEERINFO_LEN - total;
		LPVOID lpShareMemory = MapViewOfFile(shareExchange->hMap, FILE_MAP_ALL_ACCESS, 0, (DWORD)total, size);
		memcpy(lpShareMemory, peerInfoBuffer + total, size);
		UnmapViewOfFile(lpShareMemory);
		total += size;
	}
	BFX_FREE(peerInfoBuffer);

	SetEvent(shareExchange->hRemoteEvent);
}

static const BdtPeerInfo* ShareMemoryGetPeerInfoFrom(ExchangePeerInfo* exchange)
{
	ShareMemoryExchangePeerInfo* shareExchange = (ShareMemoryExchangePeerInfo*)exchange;
	shareExchange->hRemoteEvent = CreateEventA(NULL, FALSE, FALSE, "testnode_remote_event");
	if (shareExchange->hRemoteEvent == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		shareExchange->hRemoteEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "testnode_remote_event");
	}

	shareExchange->hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, TEST_NODE_PEERINFO_LEN, "test_node_peerinfo");
	if (shareExchange->hMap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		shareExchange->hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "test_node_peerinfo");
	}

	SYSTEM_INFO stSysInfo = { 0 };
	GetSystemInfo(&stSysInfo);

	DWORD dwObj = WaitForSingleObject(shareExchange->hRemoteEvent, INFINITE);
	if (dwObj == WAIT_OBJECT_0)
	{
		uint8_t* peerInfoBuffer = BFX_MALLOC_ARRAY(uint8_t, TEST_NODE_PEERINFO_LEN);
		size_t total = 0;
		while (total < TEST_NODE_PEERINFO_LEN)
		{
			size_t size = stSysInfo.dwAllocationGranularity < TEST_NODE_PEERINFO_LEN - total ? stSysInfo.dwAllocationGranularity : TEST_NODE_PEERINFO_LEN - total;
			LPVOID lpShareMemory = MapViewOfFile(shareExchange->hMap, FILE_MAP_ALL_ACCESS, 0, (DWORD)total, size);
			memcpy(peerInfoBuffer + total, lpShareMemory, size);
			UnmapViewOfFile(lpShareMemory);
			total += size;
		}

		BdtPeerInfo* remotePeerInfo = BdtPeerInfoCreateForTest();
		BfxBufferStream stream;
		BfxBufferStreamInit(&stream, (void*)peerInfoBuffer, TEST_NODE_PEERINFO_LEN);

		BdtPeerInfoDecodeForTest(&stream, remotePeerInfo);
		
		BFX_FREE(peerInfoBuffer);

		return remotePeerInfo;
	}

	return NULL;
}

ShareMemoryExchangePeerInfo* ShareMemoryExchangePeerInfoCreate()
{
	ShareMemoryExchangePeerInfo* exchange = BFX_MALLOC_OBJ(ShareMemoryExchangePeerInfo);
	memset(exchange, 0, sizeof(ShareMemoryExchangePeerInfo));
	exchange->base.getPeerInfoFrom = ShareMemoryGetPeerInfoFrom;
	exchange->base.sendPeerInfoTo = ShareMemorySendPeerInfoTo;
	return exchange;
}

static void NetworkSendPeerInfoTo(ExchangePeerInfo* exchange, const BdtPeerInfo* peerinfo)
{
	NetworkExchangePeerInfo* networkExchange = (NetworkExchangePeerInfo*)exchange;


	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in remote = { 0 };
	remote.sin_family = AF_INET;
	remote.sin_port = networkExchange->port;
	inet_pton(AF_INET, networkExchange->ip, &(remote.sin_addr.s_addr));

	int ret = bind(s, (struct sockaddr*)(&remote), (int)(sizeof(struct sockaddr)));
	if (ret == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		return;
	}
	ret = listen(s, 1);
	if (ret == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		return;
	}
	
	int addrlen = (int)(sizeof(struct sockaddr));
	SOCKET remoteSocket = accept(s, (struct sockaddr*)(&remote), &addrlen);

	uint8_t* peerInfoBuffer = BFX_MALLOC_ARRAY(uint8_t, TEST_NODE_PEERINFO_LEN);
	BfxBufferStream stream;
	BfxBufferStreamInit(&stream, (void*)peerInfoBuffer, TEST_NODE_PEERINFO_LEN);
	BdtPeerInfoEncodeForTest(peerinfo, &stream);

	uint32_t len = (uint32_t)(stream.pos);
	send(remoteSocket, (const char*)(&len), sizeof(uint32_t), 0);
	send(remoteSocket, (const char*)peerInfoBuffer, (int)(stream.pos), 0);
	BFX_FREE(peerInfoBuffer);

	closesocket(remoteSocket);
	closesocket(s);
}

static const BdtPeerInfo* NetworkGetPeerInfoFrom(ExchangePeerInfo* exchange)
{
	NetworkExchangePeerInfo* networkExchange = (NetworkExchangePeerInfo*)exchange;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in remote = { 0 };
	remote.sin_family = AF_INET;
	remote.sin_port = networkExchange->port;
	inet_pton(AF_INET, networkExchange->remoteIp, &(remote.sin_addr.s_addr));

	int ret = connect(s, (const struct sockaddr*)(&remote), sizeof(struct sockaddr));
	while (ret)
	{
		DWORD dwErr = WSAGetLastError(); 
		ret = connect(s, (const struct sockaddr*)(&remote), sizeof(struct sockaddr));
	}
	uint32_t len = 0;
	int recvlen = recv(s, (char*)(&len), sizeof(uint32_t), 0);
	if (recvlen)
	{
		char* recvBuffer = BFX_MALLOC_ARRAY(uint8_t, TEST_NODE_PEERINFO_LEN);
		memset(recvBuffer, 0, sizeof(uint8_t) * TEST_NODE_PEERINFO_LEN);
		size_t total = 0;
		while (total < len)
		{
			recvlen = recv(s, (char*)(recvBuffer + total), (int)(TEST_NODE_PEERINFO_LEN - total), 0);
			total += recvlen;
		}

		BdtPeerInfo* remotePeerInfo = BdtPeerInfoCreateForTest();
		BfxBufferStream stream;
		BfxBufferStreamInit(&stream, (void*)recvBuffer, TEST_NODE_PEERINFO_LEN);

		BdtPeerInfoDecodeForTest(&stream, remotePeerInfo);

		BFX_FREE(recvBuffer);

		return remotePeerInfo;
	}
	closesocket(s);

	return NULL;
}

NetworkExchangePeerInfo* NetworkExchangePeerInfoCreate(const char* ip, const char* remoteip)
{
	NetworkExchangePeerInfo* exchange = BFX_MALLOC_OBJ(NetworkExchangePeerInfo);
	memset(exchange, 0, sizeof(NetworkExchangePeerInfo));
	exchange->base.getPeerInfoFrom = NetworkGetPeerInfoFrom;
	exchange->base.sendPeerInfoTo = NetworkSendPeerInfoTo;
	strcpy(exchange->ip, ip);
	strcpy(exchange->remoteIp, remoteip);
	exchange->port = 5000;
	
	return exchange;
}
