
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in Module.c of history module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include "./Storage.h"
#include "./TunnelHistory.h"

typedef BdtHistory_PeerConnectVector history_connect;
BFX_VECTOR_DEFINE_FUNCTIONS(history_connect, BdtHistory_PeerConnectInfo*, connections, size, _allocsize)
typedef BdtHistory_PeerSuperNodeInfoVector super_node_info;
BFX_VECTOR_DEFINE_FUNCTIONS(super_node_info, BdtHistory_SuperNodeInfo*, snHistorys, size, _allocsize)

void BdtHistory_StorageEvent(
	BdtStack* stack,
	BdtStorage* storage,
	uint32_t eventId,
	void* eventParam,
	const void* userData
)
{

}

// ���ؽڵ���ʷͨ����Ϣ
uint32_t BdtHistory_StorageLoadAll(
	BdtStorage* storage,
	int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��(UTC, S)
	BDT_HISTORY_STORAGE_LOAD_ALL_CALLBACK callback,
	const BfxUserData* userData
)
{
	if (storage == NULL)
	{
		callback(BFX_RESULT_NOT_SUPPORT,
			NULL,
			0,
			NULL,
			0,
			userData == NULL ? NULL : userData->userData);

		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->loadAll(storage,
		historyValidTime,
		callback,
		userData);
}

// ����һ���ڵ���Ϣ
uint32_t BdtHistory_StorageAddPeer(
	BdtStorage* storage,
	const BdtHistory_PeerInfo* history
)
{
	BLOG_CHECK(history != NULL);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->addPeerHistory(storage, history);
}

// ���ýڵ��constInfo����������½ڵ���Ϣ����ʱû��ConstInfo
uint32_t BdtHistory_StorageSetConstInfo(
    BdtStorage* storage,
    const BdtHistory_PeerInfo* history
)
{
    BLOG_CHECK(history != NULL);

    if (storage == NULL)
    {
        return BFX_RESULT_NOT_SUPPORT;
    }

    return storage->setPeerConstInfo(storage, history);
}

// ����ĳ�ڵ��һ��������Ϣ
uint32_t BdtHistory_StorageUpdateConnect(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const BdtHistory_PeerConnectInfo* connectInfo,
	bool isNew
)
{
	BLOG_CHECK(peerid != NULL);
	BLOG_CHECK(connectInfo != NULL);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->updateConnectHistory(storage,
		peerid,
		connectInfo,
		isNew);
}

// ������ĳSuperNode�ϲ�ѯ��ĳ�ڵ��ʱ��
uint32_t BdtHistory_StorageUpdateSuperNode(
	BdtStorage* storage,
	const BdtPeerid* clientPeerid,
	const BdtPeerid* superNodePeerid,
	int64_t responseTime,
	bool isNew
)
{
	BLOG_CHECK(clientPeerid != NULL);
	BLOG_CHECK(superNodePeerid != NULL);
	BLOG_CHECK(responseTime > 0);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->updateSuperNodeHistory(storage,
		clientPeerid,
		superNodePeerid,
		responseTime,
		isNew);
}

// ����һ���ڵ�ͨ�ŵ�AES��Կ(����ʱ��/��ʱʱ��/��ǵ�)
uint32_t BdtHistory_StorageUpdateAesKey(
	BdtStorage* storage,
	const BdtPeerid* peerid,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	int64_t expireTime,
	uint32_t flags,
	int64_t lastAccessTime,
	bool isNew
)
{
	BLOG_CHECK(peerid != NULL);
	BLOG_CHECK(aesKey != NULL);
	BLOG_CHECK(expireTime > 0);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->updateAesKey(storage,
		peerid,
		aesKey,
		expireTime,
		flags,
		lastAccessTime,
		isNew);
}

// �����ض��ڵ����ʷ��¼
uint32_t BdtHistory_StorageLoadPeerHistoryByPeerid(
    BdtStorage* storage,
    const BdtPeerid* peerid,
    int64_t historyValidTime, // ��ʷ��¼���籣��ʱ��
    BDT_HISTORY_STORAGE_LOAD_PEER_HISTORY_BY_PEERID_CALLBACK callback,
    const BfxUserData* userData
)
{
    BLOG_CHECK(peerid != NULL);

    if (storage == NULL)
    {
        callback(BFX_RESULT_NOT_SUPPORT,
            NULL,
            NULL,
            0,
            userData == NULL ? NULL : userData->userData);

        return BFX_RESULT_NOT_SUPPORT;
    }

    return storage->loadPeerHistoryByPeerid(storage,
        peerid,
        historyValidTime,
        callback,
        userData);
}

// �����ض��ڵ��TCP������־
uint32_t BdtHistory_LoadTunnelHistoryTcpLog(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	uint32_t invalidTime,
	BDT_HISTORY_STORAGE_LOAD_TUNNEL_HISTORY_TCP_LOG_CALLBACK callback,
	const BfxUserData* userData
)
{
	BLOG_CHECK(remotePeerid != NULL);
	BLOG_CHECK(callback != NULL);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->loadTunnelHistoryTcpLog(storage,
		remotePeerid,
		invalidTime,
		callback,
		userData);
}

// ����/׷��TCP������־
uint32_t BdtHistory_UpdateTunnelHistoryTcpLog(
	BdtStorage* storage,
	const BdtPeerid* remotePeerid,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	Bdt_TunnelHistoryTcpLogType type,
	uint32_t times,
	int64_t timestamp,
	int64_t oldTimestamp,
	bool isNew
)
{
	BLOG_CHECK(remotePeerid != NULL);
	BLOG_CHECK(localEndpoint != NULL);
	BLOG_CHECK(remoteEndpoint != NULL);

	if (storage == NULL)
	{
		return BFX_RESULT_NOT_SUPPORT;
	}

	return storage->updateTunnelHistoryTcpLog(storage,
		remotePeerid,
		localEndpoint,
		remoteEndpoint,
		type,
		times,
		timestamp,
		oldTimestamp,
		isNew);
}

// �رմ洢�ļ�
void BdtHistory_StorageDestroy(BdtStorage* storage)
{
	if (storage != NULL)
	{
		storage->destroy(storage);
	}
}
