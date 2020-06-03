#include <crtdbg.h>  

#include "BDTCore/History/HistoryModule.c"
#define __BDT_TUNNEL_MODULE_IMPL__
#include "BDTCore/History/TunnelHistory.h"
#include "BDTCore/Global/CryptoToolKit.h"

static const BdtPeerInfo* createPeerInfo(const char* deviceid, uint16_t port, const BdtPeerInfo* snPeerInfo)
{
	BdtPeerConstInfo peerConstInfo = {
		{1,2,3,4,5},
		{'\0'},
		BDT_PEER_PUBLIC_KEY_TYPE_RSA1024,
		{
			{'\0'}
		}
	};
	BdtEndpoint endpoint = {
		.flag = BDT_ENDPOINT_PROTOCOL_UDP | BDT_ENDPOINT_IP_VERSION_4,
		.reserve = 0,
		.port = port,
		.address = {192, 168, 100, 204}
	};

	memcpy(peerConstInfo.deviceId, deviceid, sizeof(peerConstInfo.deviceId));
	memcpy(&peerConstInfo.publicKey, deviceid, sizeof(peerConstInfo.deviceId));
	BdtPeerInfoBuilder* infoBuilder = BdtPeerInfoBuilderBegin(snPeerInfo ? BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO  : 0);
	BdtPeerInfoSetConstInfo(infoBuilder, &peerConstInfo);
	BdtPeerInfoAddEndpoint(infoBuilder, &endpoint);
	if (snPeerInfo != NULL)
	{
		BdtPeerInfoAddSnInfo(infoBuilder, snPeerInfo);
	}
	return BdtPeerInfoBuilderFinish(infoBuilder, NULL);
}

static void FindHistoryCallback(const BdtHistory_PeerInfo* peerHistory, void* userData);
static void BdtStackLoadHistoryFromStorageCallback(
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount,
	void* userData
);

const BdtPeerInfo* snPeerInfo = NULL;
const BdtPeerInfo* localPeerInfo = NULL;
const BdtPeerInfo* remotePeerInfo = NULL;

struct
{
	BdtHistory_PeerUpdater updater;
	BdtHistory_KeyManager keyManager;
	BdtStorage* storage;
} Stack;

static const BdtPeerHistoryConfig historyConfig = {
	.activeTime = 3600 * 24 * 10,
	.peerCapacity = 1000
};

static const BdtAesKeyConfig aesKeyConfig = {
	.activeTime = 3600 * 24 * 10
};

static const BdtTunnelHistoryConfig tunnelHistoryConfig = {
	.respCacheSize = 3,
	.tcpLogCacheSize = 10,
	.logKeepTime = 60000, // ms
};

static void EnableMemLeakCheck()
{
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpFlag);
}

int g_successCount = 1;
bool g_isFailed = false;

typedef struct Bdt_TestCase Bdt_TestCase;


static void OnHistoryLoad(Bdt_TunnelHistory* history, void* userData)
{

}

static int HistoryTest(Bdt_TestCase* testCase)
{
	EnableMemLeakCheck();

	DeleteFileW(L"./history_test.db");

	const uint8_t snDeviceId[BDT_MAX_DEVICE_ID_LENGTH] = "sn";
	snPeerInfo = createPeerInfo(snDeviceId, 10020, NULL);
	const uint8_t localDeviceId[BDT_MAX_DEVICE_ID_LENGTH] = "local";
	localPeerInfo = createPeerInfo(localDeviceId, 2003, snPeerInfo);
	const uint8_t remoteDeviceId[BDT_MAX_DEVICE_ID_LENGTH] = "remote";
	remotePeerInfo = createPeerInfo(remoteDeviceId, 2004, NULL);

	Stack.storage = BdtCreateSqliteStorage(L"./history_test.db", 10);

	PeerUpdaterInit(&Stack.updater, localPeerInfo, false, &historyConfig);
	KeyManagerInit(&Stack.keyManager, &aesKeyConfig);

	int64_t nowSecond = BfxTimeGetNow(false) / 1000000;
	BfxUserData userData = { .userData = NULL,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BdtHistory_StorageLoadAll(
		Stack.storage,
		nowSecond - 24 * 3600,
		BdtStackLoadHistoryFromStorageCallback,
		&userData
	);

	BdtHistory_PeerUpdaterOnFoundPeerFromSuperNode(&Stack.updater, remotePeerInfo, snPeerInfo);

	size_t count = 0;
    BdtHistory_PeerUpdaterOnPackageFromRemotePeer(
		&Stack.updater, 
		BdtPeerInfoGetPeerid(remotePeerInfo),
		remotePeerInfo,
		BdtPeerInfoListEndpoint(remotePeerInfo, &count),
		BdtPeerInfoListEndpoint(localPeerInfo, &count),
		BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT,
		5);

	BdtHistory_PeerUpdaterTransferTask* statTask = BdtHistory_PeerUpdaterOnStartTransfer(&Stack.updater,
		remotePeerInfo,
		BdtPeerInfoListEndpoint(remotePeerInfo, &count),
		BdtPeerInfoListEndpoint(localPeerInfo, &count));
	assert(statTask != NULL);

	BdtHistory_PeerUpdaterOnTransferRtoTestBegin(statTask);
	Sleep(100);
	BdtHistory_PeerUpdaterOnTransferRtoTestEnd(statTask);

	for (int i = 0; i < 10; i++)
	{
		BdtHistory_PeerUpdaterOnTransferDataSend(statTask, 1024);
		BdtHistory_PeerUpdaterOnTransferDataRecv(statTask, 1024);

		Sleep(100);
		BdtHistory_PeerUpdaterOnTransferSendPause(statTask);
		BdtHistory_PeerUpdaterOnTransferRecvPause(statTask);
	}

	BdtHistory_PeerUpdaterOnTransferEnd(statTask);

	BdtHistory_PeerUpdaterFindByPeerid(&Stack.updater,
		BdtPeerInfoGetPeerid(remotePeerInfo),
		false,
		FindHistoryCallback,
		NULL);

	if (g_isFailed)
	{
		return g_successCount;
	}

	for (int i = 0; i < 1000; i++)
	{
		uint8_t aesKey[BDT_AES_KEY_LENGTH] = { 1, 2, 3, 4 };
		BfxRandomBytes(aesKey, BDT_AES_KEY_LENGTH);
		uint8_t aesKeyHash[BDT_MIX_HASH_LENGTH];
		Bdt_HashAesKey(aesKey, aesKeyHash);
		BdtHistory_KeyManagerAddKey(&Stack.keyManager, aesKey, BdtPeerInfoGetPeerid(localPeerInfo), true);

		uint8_t foundKey[BDT_AES_KEY_LENGTH];
		BdtPeerid foundPeerid;
		BdtHistory_KeyManagerFindByMixHash(&Stack.keyManager, aesKeyHash, foundKey, &foundPeerid, true, true);
		if (memcmp(foundKey, aesKey, sizeof(foundKey)) == 0 &&
			memcmp(&foundPeerid, BdtPeerInfoGetPeerid(localPeerInfo), sizeof(BdtPeerid)) == 0)
		{
			g_successCount++;
		}
		else
		{
			return g_successCount;
		}

		Sleep(10);
	}

	BfxUserData udLoad = {
		.lpfnAddRefUserData = NULL, 
		.lpfnReleaseUserData = NULL, 
		.userData = NULL
	};
	Bdt_TunnelHistory* tunnelHistory = Bdt_TunnelHistoryCreate(
		BdtPeerInfoGetPeerid(remotePeerInfo), 
		&Stack.updater, 
		&tunnelHistoryConfig, 
		OnHistoryLoad, 
		&udLoad);

	size_t endpointCount = 0;
    BdtEndpoint local = *BdtPeerInfoListEndpoint(localPeerInfo, &endpointCount);
    BdtEndpoint remote = *BdtPeerInfoListEndpoint(remotePeerInfo, &endpointCount);

    Bdt_TunnelHistoryTcpLogForLocalEndpoint tcpLog;
	size_t requireCount = 1;
	size_t logCountOld = 0;
	Bdt_TunnelHistoryGetTcpLog(tunnelHistory, &local, &tcpLog, requireCount);
    logCountOld = 0;
    for (size_t i = 0; i < tcpLog.logForRemoteVector.size; i++)
    {
        logCountOld += tcpLog.logForRemoteVector.logsForRemote[i].logVector.size;
        for (size_t j = 0; j < tcpLog.logForRemoteVector.logsForRemote[i].logVector.size; j++)
        {
            logCountOld += tcpLog.logForRemoteVector.logsForRemote[i].logVector.logs[j].times;
        }
    }

	Bdt_TunnelHistoryAddTcpLog(tunnelHistory,
		&local,
		&remote,
		BDT_TUNNEL_HISTORY_TCP_LOG_TYPE_CONNECT_FAILED);

	Bdt_TunnelHistoryGetTcpLog(tunnelHistory, &local, &tcpLog, requireCount);

	size_t logCountNew = 0;
    logCountNew = 0;
    for (size_t i = 0; i < tcpLog.logForRemoteVector.size; i++)
    {
        logCountNew += tcpLog.logForRemoteVector.logsForRemote[i].logVector.size;
        for (size_t j = 0; j < tcpLog.logForRemoteVector.logsForRemote[i].logVector.size; j++)
        {
            logCountNew += tcpLog.logForRemoteVector.logsForRemote[i].logVector.logs[j].times;
        }
    }

	if (requireCount < 1 || logCountNew != logCountOld + 1)
	{
		return g_successCount;
	}

	PeerUpdaterUninit(&Stack.updater);
	KeyManagerUninit(&Stack.keyManager);
	Stack.storage->destroy(Stack.storage);

	BdtReleasePeerInfo(snPeerInfo);
	BdtReleasePeerInfo(localPeerInfo);
	BdtReleasePeerInfo(remotePeerInfo);

	return 0;
}

static void BdtStackLoadHistoryFromStorageCallback(
	uint32_t errorCode,
	const BdtHistory_PeerInfo* historys[],
	size_t historyCount,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount,
	void* userData
)
{
	BdtHistory_PeerUpdaterOnLoadDone(&Stack.updater,
		Stack.storage,
		errorCode,
		historys,
		historyCount);

	BdtHistory_KeyManagerOnLoadDone(&Stack.keyManager,
		Stack.storage,
		errorCode,
		aesKeys,
		keyCount);
}

static void FindHistoryCallback(const BdtHistory_PeerInfo* peerHistory, void* userData)
{
	if (peerHistory == NULL)
	{
		g_isFailed = true;
		return;
	}

	if (memcmp(&peerHistory->peerid, BdtPeerInfoGetPeerid(remotePeerInfo), sizeof(BdtPeerid)) != 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (peerHistory->snVector.size != 1)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (peerHistory->snVector.snHistorys == NULL)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (memcmp(&peerHistory->snVector.snHistorys[0]->peerInfo.peerid,
		BdtPeerInfoGetPeerid(snPeerInfo), sizeof(BdtPeerid)) != 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (peerHistory->connectVector.size < 1)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	size_t count = 0;
	BdtHistory_PeerConnectInfo* connectInfo = *peerHistory->connectVector.connections;
	if (connectInfo == NULL)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (memcmp(&connectInfo->endpoint,
		BdtPeerInfoListEndpoint(remotePeerInfo, &count),
		sizeof(BdtEndpoint)) != 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (memcmp(connectInfo->localEndpoint, BdtPeerInfoListEndpoint(localPeerInfo, &count), sizeof(BdtEndpoint)) != 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->rto <= 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->lastConnectTime <= 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->flags.u32 != BDT_HISTORY_PEER_CONNECT_TYPE_TCP_DIRECT)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->sendSpeed <= 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->foundTime <= 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;

	if (connectInfo->recvSpeed <= 0)
	{
		g_isFailed = true;
		return;
	}
	g_successCount++;
}