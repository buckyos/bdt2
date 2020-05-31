#include "./bdtCoreTest.h"

static uv_sem_t sig;


static BdtStack* lstack = NULL;
static BdtStack* rstack = NULL;
static const BdtPeerInfo* remotePeer = NULL;

static void OnStackEvent(BdtStack* stack, uint32_t eventId, const void* eventParams, void* userData)
{

}

static void BFX_STDCALL OnFirstAnswer(const uint8_t* dataArray, size_t dataCount, void* userData)
{
	char recvStr[100];
	memcpy(recvStr, dataArray, dataCount);
	recvStr[dataCount] = '\0';
	printf("test stream connection: got first answer %s\n", recvStr);
}


static void OnSecondConnectionEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	printf("second tcp connection established \r\n");
}




static void BFX_STDCALL OnSend(uint32_t result, const uint8_t* buffer, size_t length, void* userData)
{
	printf("send %zu data stream callback with result %u\r\n", length, result);
}

static void OnFirstConnectionConnectedEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	printf("first tcp connection established \r\n");
	/*{
		char* data = BFX_MALLOC_ARRAY(char, 100);
		memset(data, 0, 100);
		const char* datastr = "this is first data stream";
		memcpy(data, datastr, strlen(datastr));
		BfxUserData udSend =
		{
			.lpfnAddRefUserData = NULL,
			.lpfnReleaseUserData = NULL,
			.userData = NULL
		};
		result = BdtConnectionSend(connection, data, 100, OnSend, &udSend);
		printf("send first data stream with result %u", result);
	}*/

	{
		size_t len = 5 * 1024;
		uint8_t* data = BFX_MALLOC_ARRAY(uint8_t, len);
		for (size_t ix = 0; ix < len; ++ix)
		{
			data[ix] = (uint8_t)ix;
		}
		BfxUserData udSend =
		{
			.lpfnAddRefUserData = NULL,
			.lpfnReleaseUserData = NULL,
			.userData = NULL
		};
		//result = BdtConnectionSend(connection, data, len, OnSend, &udSend);
		printf("send %zu data stream with result %u", len, result);


		{

			BfxUserData recvUserData = {
			.userData = NULL,
			.lpfnAddRefUserData = NULL,
			.lpfnReleaseUserData = NULL
			};

			BdtConnection* connection2 = NULL;

			BdtBuildTunnelParams params;
			memset(&params, 0, sizeof(BdtBuildTunnelParams));
			params.remoteConstInfo = BdtPeerInfoGetConstInfo(remotePeer);
			const char* fq = "firstQuestion";
			size_t fqLen = strlen(fq);

			BdtCreateConnection(
				lstack,
				BdtPeerInfoGetPeerid(remotePeer),
				true,
				&connection2
			);

			BdtConnectionConnect(
				connection2,
				0,
				&params,
				NULL,
				0,
				OnSecondConnectionEvent,
				&recvUserData, 
				OnFirstAnswer,
				&recvUserData
			);
		}
	}

}


typedef struct RecvUserData
{
	BdtConnection* connection;
	BFX_BUFFER_HANDLE recvBuffer;
	size_t totalRecv;
	char name[10];
} RecvUserData;

static RecvUserData g_recvUserData1 = { .name = "con1" };
static RecvUserData g_recvUserData2 = { .name = "con2" };

static void OnRecv(
	uint8_t* buffer,
	size_t recv,
	void* userData)
{
	RecvUserData* ud = (RecvUserData*)userData;


	for (size_t ix = 0; ix < recv; ++ix)
	{
		if (buffer[ix] != (uint8_t)(ix + ud->totalRecv))
		{
			BLOG_ERROR("got error data on index: %zu, %u expected, but %u got\n", ix + ud->totalRecv, (uint8_t)(ix + ud->totalRecv), buffer[ix]);
		}
	}
	ud->totalRecv += recv;
	printf("connection recv len: %zu total: %zu\n", recv, ud->totalRecv);
	size_t bufSize = 0;
	BfxUserData recvUserData = {
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL,
		.userData = userData
	};
	BdtConnectionRecv(ud->connection, BfxBufferGetData(ud->recvBuffer, &bufSize), bufSize, OnRecv, &recvUserData);
}




static void OnFirstConnectionAcceptedEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	printf("first tcp connection passive established \r\n");

	BFX_BUFFER_HANDLE buf = BfxCreateBuffer(100);
	g_recvUserData1.connection = connection;
	g_recvUserData1.recvBuffer = buf;
	g_recvUserData1.totalRecv = 0;

	size_t bufSize = 0;
	BfxUserData recvUserData = {
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL,
		.userData = &g_recvUserData1
	};

	BdtConnectionRecv(connection, BfxBufferGetData(g_recvUserData1.recvBuffer, &bufSize), bufSize, OnRecv, &recvUserData);
}



static void OnFirstConnectionPreAcceptEvent(uint32_t result,
	const uint8_t* buffer,
	size_t recv,
	BdtConnection* connection,
	void* userData)
{
	char szTemp[1024] = { 0 };
	memcpy(szTemp, buffer, recv);
	printf("test stream connection: got first question %s\n", szTemp);
	BdtAddRefConnection(connection);
	const char* fr = "first response";
	size_t toSendLen = strlen(fr);
	uint8_t* toSend = BfxMalloc(toSendLen);
	memcpy(toSend, fr, toSendLen);
	BfxUserData sendUserData = {
		.userData = NULL,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BfxUserData connectUserData = {
		.userData = NULL,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BdtConfirmConnection(connection, toSend, toSendLen, OnFirstConnectionAcceptedEvent, &connectUserData);
}



static void OnSecondConnectionAcceptedEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	printf("second tcp connection passive established \r\n");
}


static void OnSecondConnectionPreAcceptEvent(uint32_t result,
	const uint8_t* buffer,
	size_t recv,
	BdtConnection* connection,
	void* userData)
{
	char szTemp[1024] = { 0 };
	memcpy(szTemp, buffer, recv);
	printf("test stream connection: got first question %s\n", szTemp);
	BdtAddRefConnection(connection);
	const char* fr = "first response";
	size_t toSendLen = strlen(fr);
	uint8_t* toSend = BfxMalloc(toSendLen);
	memcpy(toSend, fr, toSendLen);
	BfxUserData sendUserData = {
		.userData = NULL,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BfxUserData connectUserData = {
		.userData = NULL,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BdtConfirmConnection(connection, toSend, toSendLen, OnSecondConnectionAcceptedEvent, &connectUserData);
}


void testPackageConn()
{
	const char* localEp[] = { "v4udp127.0.0.1:10000", NULL };
	const BdtPeerInfo* localPeer = testCreatePeerInfo("local", localEp);
	const char* remoteEp[] = { "v4udp127.0.0.1:10001", NULL };
	remotePeer = testCreatePeerInfo("remote", remoteEp);

	BfxUserData userData = {
		.userData = NULL,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	lstack = testCreateBdtStack(userData, localPeer, remotePeer, OnStackEvent);
	rstack = testCreateBdtStack(userData, remotePeer, localPeer, OnStackEvent);


	BdtListenConnection(rstack, 0, NULL);

	uv_sem_init(&sig, 0);

	BdtConnection* connection1 = NULL;
	BdtBuildTunnelParams params;
	memset(&params, 0, sizeof(BdtBuildTunnelParams));
	params.remoteConstInfo = BdtPeerInfoGetConstInfo(remotePeer);
	const char* fq = "firstQuestion";
	size_t fqLen = strlen(fq);


	BdtCreateConnection(
		lstack,
		BdtPeerInfoGetPeerid(remotePeer),
		true,
		&connection1
	);

	BdtConnectionConnect(
		connection1,
		0,
		&params,
		fq,
		fqLen,
		OnFirstConnectionConnectedEvent,
		&userData, 
		OnFirstAnswer,
		&userData
	);

	BfxUserData recvUserData = {
		.userData = &g_recvUserData1,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	{
		size_t bufferLen = 100;
		uint8_t* buffer = malloc(bufferLen);
		memset(buffer, 0, bufferLen);
		BdtAcceptConnection(rstack, 0, OnFirstConnectionPreAcceptEvent, &userData);
	}

	{
		size_t bufferLen = 100;
		uint8_t* buffer = malloc(bufferLen);
		memset(buffer, 0, bufferLen);
		BdtAcceptConnection(rstack, 0, OnSecondConnectionPreAcceptEvent, &userData);
	}


	uv_sem_wait(&sig);
}