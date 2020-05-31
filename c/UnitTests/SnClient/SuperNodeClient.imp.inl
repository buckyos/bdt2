#include <crtdbg.h>  

#include <BDTCore/Stack.h>
#include <BDTCore/SnClient/SnClientModule.c>
#include <BuckyFramework/framework.h>

static void BFX_STDCALL BdtStackEventCallback(BdtStack* stack, uint32_t eventID, const void* eventParam, void* owner);
static void BdtCallTestCallRespCallback(uint32_t errorCode, BdtSnClient_CallSession* session, const BdtPeerInfo* remotePeerInfo, void* owner);

static BdtStack* preparePeer(BdtPeerConstInfo* constInfo,
	BdtEndpoint* localEndpoint,
	const BdtPeerInfo* snPeerinfo,
	BdtStorage* storage)
{
	BdtStack* stack = NULL;
	BfxUserData ud = { NULL, NULL, NULL };

	BdtPeerSecret secret;
	size_t publicLen = 1024;
	size_t secretLength = 1024;
	int ret = Bdt_RsaGenerateToBuffer(1024, constInfo->publicKey.rsa1024, &publicLen, secret.secret.rsa1024, &secretLength);

	constInfo->publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;
	secret.secretLength = (uint16_t)secretLength;

	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET|BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
	BdtPeerInfoSetConstInfo(builder, constInfo);
	BdtPeerInfoAddEndpoint(builder, localEndpoint);
	const BdtPeerInfo* localPeer = BdtPeerInfoBuilderFinish(builder, &secret);

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

	BdtSystemFramework* framework = createBuckyFramework(&events, &options);
	BdtCreateStack(framework,
		localPeer,
		NULL, 0,
		storage,
		BdtStackEventCallback,
		&ud,
		&stack);

	BdtSnClient_UpdateSNList(BdtStackGetSnClient(stack), &snPeerinfo, 1);
	BdtReleasePeerInfo(localPeer);
	return stack;
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

static void EnableMemLeakCheck()
{
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpFlag);
}

typedef struct Bdt_TestCase Bdt_TestCase;

static int SuperNodeClientUnitTest(Bdt_TestCase* testCase)
{
	EnableMemLeakCheck();
	// _CrtSetBreakAlloc(84869); pk
	// _CrtSetBreakAlloc(6786); pk
	// _CrtSetBreakAlloc(11561);

	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

	BdtPeerConstInfo localPeerConstInfo1 = {
		{1,2,3,4,5},
		{'c', 'l', 'i', 'e', 'n', 't', '1', '\0'},
		BDT_PEER_PUBLIC_KEY_TYPE_RSA1024,
		{
			{'\0'}
		}
	};
	BdtEndpoint localEndpoint1 = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 2002,
		.address = {192, 168, 100, 204}
	};

	BdtEndpoint localEndpointV61 = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_6,
		.reserve = 0,
		.port = 2002,
        .addressV6 = {0xfd, 0x92, 0xae, 0xf3, 0xbc, 0xcd, 0x00, 0x00, 0xe8, 0x97, 0x68, 0x67, 0x8f, 0xa1, 0x16, 0x77}
    };

	BdtPeerConstInfo localPeerConstInfo2 = {
		{1,2,3,4,5},
		{'c', 'l', 'i', 'e', 'n', 't', '2', '\0'},
		BDT_PEER_PUBLIC_KEY_TYPE_RSA1024,
		{
			{'\0'}
		}
	};
	BdtEndpoint localEndpoint2 = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = 2003,
		.address = {192, 168, 100, 204}
	};

    BdtEndpoint localEndpointV62 = {
        .flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_6,
        .reserve = 0,
        .port = 2003,
        .addressV6 = {0xfd, 0x92, 0xae, 0xf3, 0xbc, 0xcd, 0x00, 0x00, 0xe8, 0x97, 0x68, 0x67, 0x8f, 0xa1, 0x16, 0x77}
    };

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
		.address = {192, 168, 100, 204}
	};

    BdtEndpoint snwwwEndpointUdpV4 = {
    .flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_4,
    .reserve = 0,
    .port = 10020,
    .address = {106, 75, 175, 123}
    };

	BdtEndpoint snEndpointUdpV6 = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_6,
		.reserve = 0,
		.port = 10020,
        .addressV6 = {0xfd, 0x92, 0xae, 0xf3, 0xbc, 0xcd, 0x00, 0x00, 0xe8, 0x97, 0x68, 0x67, 0x8f, 0xa1, 0x16, 0x77}
        // .addressV6 = {0xfd, 0x92, 0xae, 0xf3, 0xbc, 0xcd, 0x00, 0x00, 0x09, 0x16, 0xad, 0x2e, 0xbc, 0x1b, 0x6f, 0xac}
    };
	//BdtEndpoint snEndpointTcp = {
	//	.flag = BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_IP_VERSION_4,
	//	.reserve = 0,
	//	.port = 10030,
	//	.address = {192, 168, 100, 204}
 //   };
	size_t publicKeyLength = 0;
	HexToBinary("30819f300d06092a864886f70d010101050003818d0030818902818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a50203010001",
		snPeerConstInfo.publicKey.rsa1024, &publicKeyLength);

	BdtPeerInfoBuilder* infoBuilder = BdtPeerInfoBuilderBegin(0);
	BdtPeerInfoSetConstInfo(infoBuilder, &snPeerConstInfo);
	BdtPeerInfoAddEndpoint(infoBuilder, &snwwwEndpointUdpV4);
	// BdtPeerInfoAddEndpoint(infoBuilder, &snEndpointTcp);
	const BdtPeerInfo* snPeerinfo = BdtPeerInfoBuilderFinish(infoBuilder, NULL);

	BdtStorage* storage = BdtCreateSqliteStorage(L"./history.db", 10);

	BdtStack* stack1 = preparePeer(&localPeerConstInfo1, &localEndpoint1, snPeerinfo, storage);
	BdtStack* stack2 = preparePeer(&localPeerConstInfo2, &localEndpoint2, snPeerinfo, NULL);

	/*
	BFX_BUFFER_HANDLE payload = BfxCreateBuffer(500);
	size_t payloadSize = 100;
	memset(BfxBufferGetData(payload, &payloadSize), 1, payloadSize);
	/*/
	BFX_BUFFER_HANDLE payload = NULL;
	//*/

	for (int i = 0; i < 10; i++)
	{
		PingManagerOnTimer(BdtStackGetSnClient(stack1)->pingMgr);
		PingManagerOnTimer(BdtStackGetSnClient(stack2)->pingMgr);
		Sleep(1000);

        Bdt_PackageWithRef* callPkg = NULL;

		//*
		BdtSnClient_CallSession* callSession = NULL;
		BdtSnClient_CreateCallSession(
			BdtStackGetSnClient(stack1),
            BdtPeerFinderGetLocalPeer(BdtStackGetPeerFinder(stack1)),
			BdtStackGetLocalPeerid(stack2), 
			NULL,
			0, 
			snPeerinfo,
			false,
			true,
			BdtCallTestCallRespCallback,
			NULL,
			&callSession,
            &callPkg);

		BdtSnClient_CallSessionStart(callSession, payload);

        Bdt_PackageRelease(callPkg);
		//*/
	}

	Sleep(10000);

	BdtCloseStack(stack1);
	BdtCloseStack(stack2);

	BdtReleasePeerInfo(snPeerinfo);

	WSACleanup();

	return 0;
}

static void BFX_STDCALL BdtStackEventCallback(BdtStack* stack, uint32_t eventID, const void* eventParam, void* owner)
{

}

static void BdtCallTestCallRespCallback(uint32_t errorCode, BdtSnClient_CallSession* session, const BdtPeerInfo* remotePeerInfo, void* owner)
{
	printf("call responsed, errorCode:%u", errorCode);
	if (remotePeerInfo == NULL)
	{
		printf("call responsed, and no peer return.");
		BdtSnClient_CallSessionRelease(session);
		return;
	}

	const BdtPeerConstInfo* constInfo = BdtPeerInfoGetConstInfo(remotePeerInfo);
	const BdtPeerid* peerid = BdtPeerInfoGetPeerid(remotePeerInfo);
	printf("call responsed, remote:peerid=0x");
	const uint32_t* pidU32 = (const uint32_t*)peerid->pid;
	for (int i = 0; i < sizeof(peerid->pid) / 4; i++)
	{
		printf("%x", pidU32[i]);
	}

	printf("\nconstInfo:\n");
	printf("\tareaCode:\n");
	printf("\tcontinent=%u\n", (uint32_t)constInfo->areaCode.continent);
	printf("\tcountry=%u\n", (uint32_t)constInfo->areaCode.country);
	printf("\tcarrier=%u\n", (uint32_t)constInfo->areaCode.carrier);
	printf("\tcity=%u\n", (uint32_t)constInfo->areaCode.city);
	printf("\tinner=%u\n", (uint32_t)constInfo->areaCode.inner);
	
	printf("\tdeviceId=0x");
	const uint32_t* didU32 = (const uint32_t*)constInfo->deviceId;
	for (int i = 0; i < sizeof(constInfo->deviceId) / 4; i++)
	{
		printf("%x", didU32[i]);
	}

	printf("\n\tpublicKeyType=%d\n", (uint32_t)constInfo->publicKeyType);
	printf("\tpublicKey=0x");
	const uint32_t* publickKeyU32 = (const uint32_t*)(&constInfo->publicKey);
	uint32_t keyLength = BdtGetPublicKeyLength(constInfo->publicKeyType);
	for (uint32_t i = 0; i < keyLength / 4; i++)
	{
		printf("%x", publickKeyU32[i]);
	}

	printf("\n\nendpointList:\n");
	size_t epcount = 0;
	const BdtEndpoint* eplist =  BdtPeerInfoListEndpoint(remotePeerInfo, &epcount);
	char epString[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = "";
	for (int i = 0; i < epcount; i++)
	{
		BdtEndpointToString(eplist + i, epString);
		printf("%s\n", epString);
	}

	BdtSnClient_CallSessionRelease(session);
}