#ifndef __STREAM_CONNECTION_IMP_INL__
#define __STREAM_CONNECTION_IMP_INL__
#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "./StreamConnection.inl"
#include "./Connection.inl"

#define STREAM_CONNECTION_FLAG_SOCKET_SENDING		(1<<7)

#define STREAM_CONNECTION_ENCRYPT_MAX_HEADER_SIZE		32
#define STREAM_CONNECTION_ENCRYPT_MAX_PADDING_SIZE		32

typedef struct StreamConnectionSendRequest
{
	const uint8_t* buffer;
	size_t length;
	BdtConnectionSendCallback callback;
	BfxUserData userData;
} StreamConnectionSendRequest;


typedef struct StreamConnectionSendRequestListItem
{
	BfxListItem base;
	StreamConnectionSendRequest request;
	size_t offset;
	int64_t postTime;
} StreamConnectionSendRequestListItem;


typedef struct StreamConnectionRecvBuffer
{
	BfxListItem base;
    int64_t recvTime;
	size_t pos;
	size_t size;
	uint8_t data[0];
} StreamConnectionRecvBuffer;

typedef struct StreamConnectionRecvRequest
{
	BfxListItem base;

	uint8_t* data;
	size_t pos;
	size_t size; //buffer
	BdtConnectionRecvCallback callback;
	BfxUserData userData;
} StreamConnectionRecvRequest;

struct StreamConnection
{
	BDT_DEFINE_TCP_INTERFACE_OWNER()
	BdtPeerid remotePeerid;
	char* name;
	BdtStreamConnectionConfig config;
	BdtConnection* owner;
	Bdt_TcpInterface* tcpInterface;
	Bdt_StackTls* tls;
	BdtSystemFramework* framework;
	
	uint32_t flags;

	int32_t sendingCallCount;
	// malloc start of record buffer
	uint8_t* recordBuffer;
	// offset with header to recordBufferHeader
	uint8_t* recordData;
	// total length of record buffer
	size_t recordBufferLength;
	// offset from recordBuffer to send from interface
	size_t recordOffset;
	// pending encrypted record to send
	size_t pendingRecordSize;

	Bdt_TimerHelper nagleTimer;

	BfxThreadLock pendingSendLock;
	// pendingSendLock protected members begin
	bool sendClosed;
	size_t recordSize;
	size_t sendRequestSize;
	BfxList sendRequests;
	// pendingSendLock protected members end

    volatile int32_t isRecvNotifying;
    bool isCompleteRecvNotifyOnly;

	BfxThreadLock recvQueueLock;
	// recvQueueLock protected members begin
	bool recvClosed;
	BfxList recvBuffersQueue;
	BfxList recvRequestQueue;
    BfxList completeRecvRequests;
	// recvQueueLock protected members end

	Bdt_TimerHelper dataNotifyTimer;
};


static void BFX_STDCALL StreamConnectionRefUserDataAddRef(void* userData)
{
	BdtAddRefConnection(((StreamConnection*)userData)->owner);
}

static void BFX_STDCALL StreamConnectionRefUserDataRelease(void* userData)
{
	BdtReleaseConnection(((StreamConnection*)userData)->owner);
}

static void StreamConnectionAsRefUserData(StreamConnection* stream, BfxUserData* outUserData)
{
	outUserData->userData = stream;
	outUserData->lpfnAddRefUserData = StreamConnectionRefUserDataAddRef;
	outUserData->lpfnReleaseUserData = StreamConnectionRefUserDataRelease;
}

static int32_t StreamConnectionAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	return BdtAddRefConnection(((StreamConnection*)owner)->owner);
}

static int32_t StreamConnectionAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	return BdtReleaseConnection(((StreamConnection*)owner)->owner);
}

static uint32_t StreamConnectionOnTcpDrain(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface);
static uint32_t StreamConnectionOnTcpData(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const uint8_t* data, size_t length);
static void StreamConnectionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error);
static void StreamConnectionNotifyCompleteRecvRequest(StreamConnection* stream, bool onlyComplete);

static void StreamConnectionInitAsTcpInterfaceOwner(StreamConnection* owner)
{
	owner->addRef = StreamConnectionAsTcpInterfaceOwnerAddRef;
	owner->release = StreamConnectionAsTcpInterfaceOwnerRelease;
	owner->onData = StreamConnectionOnTcpData;
	owner->onDrain = StreamConnectionOnTcpDrain;
	owner->onError = StreamConnectionOnTcpError;
}


static StreamConnection* StreamConnectionCreate(
	BdtSystemFramework* framework,
	Bdt_StackTls* tls,
	BdtConnection* owner,
	const BdtStreamConnectionConfig* config, 
	Bdt_TcpInterface* tcpInterface
)
{
	StreamConnection* stream = BFX_MALLOC_OBJ(StreamConnection);
	memset(stream, 0, sizeof(StreamConnection));

	//no add ref
	stream->owner = owner;

	StreamConnectionInitAsTcpInterfaceOwner(stream);

	stream->framework = framework;
	stream->config = *config;
	stream->tls = tls;
	
	BfxThreadLockInit(&stream->pendingSendLock);
	BfxListInit(&stream->sendRequests);
	stream->recordBufferLength = STREAM_CONNECTION_ENCRYPT_MAX_HEADER_SIZE + stream->config.maxRecordSize + STREAM_CONNECTION_ENCRYPT_MAX_PADDING_SIZE;
	stream->recordBuffer = (uint8_t*)BfxMalloc(stream->recordBufferLength);
	stream->recordData = stream->recordBuffer + STREAM_CONNECTION_ENCRYPT_MAX_HEADER_SIZE;

	BfxThreadLockInit(&stream->recvQueueLock);
	BfxListInit(&stream->recvBuffersQueue);
	BfxListInit(&stream->recvRequestQueue);
    BfxListInit(&stream->completeRecvRequests);
    stream->isRecvNotifying = 0;
    stream->isCompleteRecvNotifyOnly = true;

	stream->tcpInterface = tcpInterface;
	Bdt_TcpInterfaceAddRef(tcpInterface);
	Bdt_TcpInterfaceSetOwner(tcpInterface, (Bdt_TcpInterfaceOwner*)stream);

	Bdt_TimerHelperInit(&stream->nagleTimer, framework);
	Bdt_TimerHelperInit(&stream->dataNotifyTimer, framework);

	const char* prename = "StreamConnection";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetLocal(tcpInterface), localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	stream->name = name;

	return stream;
}

static void StreamConnectionDestroy(StreamConnection* stream)
{
	Bdt_TcpInterfaceRelease(stream->tcpInterface);
	BfxFree(stream->name);
	BfxFree(stream->recordBuffer);
	BfxThreadLockDestroy(&stream->pendingSendLock);
	BfxThreadLockDestroy(&stream->recvQueueLock);
}

static uint32_t StreamConnectionClose(
	StreamConnection* stream
)
{
	BLOG_DEBUG("close on %s", stream->name);

	uint32_t result = BFX_RESULT_SUCCESS;
	bool closeInterface = false;
	BfxThreadLockLock(&stream->pendingSendLock);
	do
	{
		if (stream->sendClosed)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		stream->sendClosed = true;
		if (stream->pendingRecordSize == 0
			&& stream->sendRequestSize == 0)
		{
			closeInterface = true;
		}
	} while (false);
	BfxThreadLockUnlock(&stream->pendingSendLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

	if (closeInterface)
	{
		BLOG_DEBUG("%s has no send request and pending record, close tcp interface", stream->name);
		Bdt_TcpInterfaceClose(stream->tcpInterface);
	}
	
	return BFX_RESULT_SUCCESS;
}

static void StreamConnectionCancelPendingSend(StreamConnection* stream, uint32_t result)
{
	BfxList all;
	BfxListInit(&all);
	BfxThreadLockLock(&stream->pendingSendLock);
	do
	{
		stream->sendClosed = true;
		BfxListSwap(&stream->sendRequests, &all);
		stream->sendRequestSize = 0;
		stream->recordSize = 0;
	} while (false);
	BfxThreadLockUnlock(&stream->pendingSendLock);

	StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListFirst(&all);
	while (item)
	{
		if (item->request.callback != NULL)
		{
			item->request.callback(
				result, 
				item->request.buffer, 
				0, 
				item->request.userData.userData);
			if (item->request.userData.lpfnReleaseUserData)
			{
				item->request.userData.lpfnReleaseUserData(item->request.userData.userData);
			}
		}
		void* preItem = item;
		item = (StreamConnectionSendRequestListItem*)BfxListNext(&all, (BfxListItem*)item);
		BfxFree(preItem);
	}
}


static void StreamConnectionRecvRequestDone(StreamConnectionRecvRequest* req)
{
	BLOG_CHECK(req->callback != NULL && req->size > 0);
    BdtStat_OnConnectionRecv(req->pos, false);
    req->callback(req->data, req->pos, req->userData.userData);
	if (req->userData.userData != NULL && req->userData.lpfnReleaseUserData != NULL)
	{
		req->userData.lpfnReleaseUserData(req->userData.userData);
	}
	BFX_FREE(req);
}


static void StreamConnectionCancelPendingRecv(StreamConnection* stream, uint32_t result)
{
	BfxList allReceived;
	BfxListInit(&allReceived);
	BfxThreadLockLock(&stream->recvQueueLock);
	{
		stream->recvClosed = true;
		BfxListSwap(&stream->recvBuffersQueue, &allReceived);

        if (BfxListSize(&stream->completeRecvRequests) == 0)
        {
            BfxListSwap(&stream->completeRecvRequests, &stream->recvRequestQueue);
        }
        else
        {
            while (BfxListSize(&stream->recvRequestQueue) > 0)
            {
                BfxListPushBack(&stream->completeRecvRequests, BfxListPopFront(&stream->recvRequestQueue));
            }
        }
	}
	BfxThreadLockUnlock(&stream->recvQueueLock);

    StreamConnectionNotifyCompleteRecvRequest(stream, false);

	while (BfxListSize(&allReceived) > 0)
	{
		BfxFree(BfxListPopFront(&allReceived));
	}
}

static uint32_t StreamConnectionReset(
	StreamConnection* stream
)
{
	Bdt_TcpInterfaceReset(stream->tcpInterface);
	StreamConnectionCancelPendingSend(stream, BFX_RESULT_INVALID_STATE);
	StreamConnectionCancelPendingRecv(stream, BFX_RESULT_INVALID_STATE);
	return BFX_RESULT_SUCCESS;
}

static const char* StreamConnectionGetName(StreamConnection* stream)
{
	return stream->name;
}

static void StreamConnectionCheckPendingSend(StreamConnection* stream);
static uint32_t StreamConnectionSendRecord(StreamConnection* stream, bool countCall);

static void BFX_STDCALL StreamConnectionOnNagleTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	StreamConnection* stream = (StreamConnection*)userData;
	Bdt_TimerHelperStop(&stream->nagleTimer);
	StreamConnectionCheckPendingSend(stream);
}

static uint32_t StreamConnectionOnTcpDrain(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	StreamConnection* stream = (StreamConnection*)owner;
	if (stream->tcpInterface == tcpInterface)
	{
		BLOG_ERROR("%s got tcp drain event, checking pending send", stream->name);
		StreamConnectionSendRecord(stream, true);
	}
	else 
	{
		return BFX_RESULT_FAILED;
	}
	return BFX_RESULT_SUCCESS;
}

static void StreamConnectionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	StreamConnection* stream = (StreamConnection*)owner;
	StreamConnectionReset(stream);

	Bdt_TunnelContainer* container = Bdt_ConnectionGetTunnelContainer(stream->owner);
	Bdt_TcpTunnel* tunnel = (Bdt_TcpTunnel*)BdtTunnel_GetTunnelByEndpoint(
		container, 
		Bdt_TcpInterfaceGetLocal(stream->tcpInterface), 
		Bdt_TcpInterfaceGetRemote(stream->tcpInterface));
	if (tunnel)
	{
		Bdt_TcpTunnelMarkDead(tunnel);
	}
	Bdt_TunnelContainerRelease(container);

	//todo: fire error to connection
}

static void BFX_STDCALL StreamConnectionOnSyncSent(void* userData)
{
	StreamConnection* stream = (StreamConnection*)userData;
	StreamConnectionCheckPendingSend(stream);
}


static uint32_t StreamConnectionSendRecord(StreamConnection* stream, bool countCall)
{
	if (countCall)
	{
		if (1 != BfxAtomicInc32(&stream->sendingCallCount))
		{
			BLOG_DEBUG("%s ignore send record for reenter", stream->name);
			return BFX_RESULT_PENDING;
		}
	}
	else
	{
		if (0 != BfxAtomicCompareAndSwap32(&stream->sendingCallCount, 0, 1))
		{
			BLOG_DEBUG("%s count send record for reenter", stream->name);
			return BFX_RESULT_PENDING;
		}
	}

	size_t pending = 0;
	do
	{
		if (stream->pendingRecordSize)
		{
			size_t sent;
			uint32_t sr = Bdt_TcpInterfaceSend(
				stream->tcpInterface,
				stream->recordBuffer + stream->recordOffset,
				stream->pendingRecordSize,
				&sent);
			BLOG_DEBUG("stream(%p) %s send record %zu/%zu to tcp interface, pending %zu", stream, stream->name, sent, stream->pendingRecordSize, pending);
			stream->recordOffset += sent;
			stream->pendingRecordSize -= sent;
			pending = stream->pendingRecordSize;
		}
		else
		{
			BLOG_DEBUG("stream(%p) %s ignore send to interface for no pending record", stream, stream->name);
		}
		if (0 == BfxAtomicDec32(&stream->sendingCallCount))
		{
			break;
		}
	} while (true);
	

	if (!pending)
	{
		bool closeInterface = false;
		BfxThreadLockLock(&stream->pendingSendLock);
		do
		{
			stream->recordSize = 0;
			if (!stream->sendRequestSize
				&& stream->sendClosed)
			{
				closeInterface = stream->sendClosed;
			}
		} while (false);
		
		BfxThreadLockUnlock(&stream->pendingSendLock);

		if (closeInterface)
		{
			BLOG_DEBUG("%s all pending record sent, and send closed, close tcp interface", stream->name);
			Bdt_TcpInterfaceClose(stream->tcpInterface);
		}
		else
		{
			BfxUserData userData;
			StreamConnectionAsRefUserData(stream, &userData);
			BLOG_DEBUG("%s all pending record sent, call next check pending send immediately", stream->name);
			BdtImmediate(stream->framework, StreamConnectionOnSyncSent, &userData);
		}
	}

	return BFX_RESULT_SUCCESS;
}


static void StreamConnectionCheckPendingSend(StreamConnection* stream)
{
	BLOG_DEBUG("%s check pending send", stream->name);

	size_t toRecordSize = 0;
	BfxList toRecord;
	BfxListInit(&toRecord);
	int64_t now = BfxTimeGetNow(false);
    int64_t delay = 0;

	BfxThreadLockLock(&stream->pendingSendLock);
	{
		if (stream->recordSize)
		{
			// do nothing
		}
		else
		{
			if (stream->sendRequestSize >= stream->config.minRecordSize
				|| stream->sendClosed)
			{
				if (stream->sendRequestSize < stream->config.maxRecordSize)
				{
					toRecordSize = stream->sendRequestSize;
					BfxListSwap(&toRecord, &stream->sendRequests);
				}
				else
				{
					toRecordSize = stream->config.maxRecordSize;
					size_t recordSize = 0;
					StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListFirst(&stream->sendRequests);
					while (item)
					{
						if ((recordSize + item->request.length - item->offset) > toRecordSize)
						{
                            StreamConnectionSendRequestListItem* partItem = BFX_MALLOC_OBJ(StreamConnectionSendRequestListItem);
							partItem->request.buffer = item->request.buffer;
							partItem->request.callback = NULL;
							partItem->offset = item->offset;
							item->offset += toRecordSize - recordSize;
							partItem->request.length = item->offset;
							recordSize = toRecordSize;
							BfxListPushBack(&toRecord, (BfxListItem*)partItem);
							break;
						}
						recordSize += (item->request.length - item->offset);
						BfxListPushBack(&toRecord, BfxListPopFront(&stream->sendRequests));
						item = (StreamConnectionSendRequestListItem*)BfxListFirst(&stream->sendRequests);
					}
				}
			}
			else
			{
				if (stream->sendRequestSize > 0)
				{
                    int64_t postTime = ((StreamConnectionSendRequestListItem*)BfxListFirst(&stream->sendRequests))->postTime;
                    delay = (((int64_t)stream->config.maxNagleDelay) * 1000 / 2 - (now - postTime)) / 1000;
                    if (delay <= 0 || postTime > now)
                    {
                        delay = 0;
                        toRecordSize = stream->sendRequestSize;
                        BfxListSwap(&toRecord, &stream->sendRequests);
                    }
				}
				else
				{
					// do nothing
				}
			}
			stream->sendRequestSize -= toRecordSize;
			stream->recordSize = toRecordSize;
		}
    }
	BfxThreadLockUnlock(&stream->pendingSendLock);

	size_t recordSize = 0;
	if (toRecordSize)
	{
		BLOG_DEBUG("%s encode record %zu", stream->name, toRecordSize);

		
		for (StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListFirst(&toRecord);
            item != NULL;
            item = (StreamConnectionSendRequestListItem*)BfxListNext(&toRecord, (BfxListItem*)item))
		{
            BLOG_HEX(item->request.buffer, 10, "stream(%p) record to send, size:%d", stream, item->request.length - item->offset);
			memcpy(stream->recordData + recordSize, item->request.buffer + item->offset, item->request.length - item->offset);
			recordSize += (item->request.length - item->offset);
		}

        assert(recordSize == toRecordSize);

		size_t headerSize = 0;
		int result = Bdt_TcpInterfaceBoxCryptoData(
			stream->tcpInterface,
			stream->recordData,
			toRecordSize,
			stream->recordBufferLength,
			&recordSize,
			&headerSize);
		stream->recordOffset = STREAM_CONNECTION_ENCRYPT_MAX_HEADER_SIZE - headerSize;
		stream->pendingRecordSize = recordSize + headerSize;
		assert(!result);

        BLOG_INFO("stream(%p) crypto size: %d=>%d, headersize:%d", stream, (int)toRecordSize, (int)recordSize, (int)headerSize);

        // 放入缓冲区就callback，否则callback通知顺序和send投递顺序不一致；
        // 因为sendRecord操作可能会触发其他线程执行record并callback；
        // 另外的解决办法就是严格按照record顺序排队执行callback
        for (StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListPopFront(&toRecord);
            item != NULL;
            item = (StreamConnectionSendRequestListItem*)BfxListPopFront(&toRecord))
        {
            if (item->request.callback != NULL)
            {
                BLOG_HEX(item->request.buffer, 10, "stream(%p) sent callback, size:%d", stream, item->request.length - item->offset);

                item->request.callback(BFX_RESULT_SUCCESS, item->request.buffer, item->request.length, item->request.userData.userData);
                if (item->request.userData.lpfnReleaseUserData)
                {
                    item->request.userData.lpfnReleaseUserData(item->request.userData.userData);
                }
            }
            BfxFree(item);
        }

		StreamConnectionSendRecord(stream, false);
	}
    else
    {
        if (delay > 0)
        {
            BfxUserData userData;
            StreamConnectionAsRefUserData(stream, &userData);
            BLOG_DEBUG("%s only little data remain, send request, create nagle timer, delay:%d.", StreamConnectionGetName(stream), delay);
            Bdt_TimerHelperStart(&stream->nagleTimer, StreamConnectionOnNagleTimer, &userData, (int32_t)delay);
        }
    }
}

static uint32_t StreamConnectionSend(
	StreamConnection* stream,
	const uint8_t* buffer,
	size_t length,
	BdtConnectionSendCallback callback,
	const BfxUserData* userData
)
{
	BLOG_DEBUG("send on %s, length:%zu", stream->name, length);

	StreamConnectionSendRequestListItem* requestItem = BFX_MALLOC_OBJ(StreamConnectionSendRequestListItem);
	memset(requestItem, 0, sizeof(StreamConnectionSendRequestListItem));
	requestItem->request.buffer = buffer;
	requestItem->request.length = length;
	requestItem->request.callback = callback;
	requestItem->request.userData = *userData;
	requestItem->postTime = BfxTimeGetNow(false);

	if (requestItem->request.userData.lpfnAddRefUserData)
	{
		requestItem->request.userData.lpfnAddRefUserData(requestItem->request.userData.userData);
	}

	StreamConnectionSendRequest* request = &requestItem->request;

	bool pending = false;
	bool nagle = false;
	size_t toRecordSize = 0;
	BfxList toRecord;
	BfxListInit(&toRecord);

	bool sendClosed = false;
	BfxThreadLockLock(&stream->pendingSendLock);
	do 
	{
		if (stream->sendClosed)
		{
			sendClosed = stream->sendClosed;
			break;
		}
		if (stream->recordSize)
		{
			BfxListPushBack(&stream->sendRequests, (BfxListItem*)requestItem);
			stream->sendRequestSize += request->length;
			pending = true;
		}
		else
		{
			if (stream->sendRequestSize < stream->config.minRecordSize)
			{
				size_t sendRequestSize = stream->sendRequestSize + request->length;
				if (sendRequestSize >= stream->config.minRecordSize)
				{
					BfxListSwap(&toRecord, &stream->sendRequests);
					if (sendRequestSize < stream->config.maxRecordSize)
					{
                        stream->sendRequestSize = 0;
						toRecordSize = sendRequestSize;
					}
					else
					{
						toRecordSize = stream->config.maxRecordSize;
						requestItem->offset += toRecordSize - stream->sendRequestSize;
						stream->sendRequestSize = (request->length - requestItem->offset);
						BfxListPushBack(&stream->sendRequests, (BfxListItem*)requestItem);
						pending = true;
					}
					stream->recordSize = toRecordSize;
				}
				else
				{
					BfxListPushBack(&stream->sendRequests, (BfxListItem*)requestItem);
					stream->sendRequestSize += request->length;
					pending = true;
					// wait nagle timer
					nagle = true;
				}
			}
			else
			{
				BfxListPushBack(&stream->sendRequests, (BfxListItem*)requestItem);
				stream->sendRequestSize += request->length;
				pending = true;
				// there wiil be a CheckPendingSend on air
			}
		}
	} while (false);
	BfxThreadLockUnlock(&stream->pendingSendLock);

	if (sendClosed)
	{
		BfxFree(requestItem);
		return BFX_RESULT_INVALID_STATE;
	}
	
	size_t recordSize = 0;
	if (toRecordSize)
	{
		for (StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListFirst(&toRecord);
            item != NULL;
            item = (StreamConnectionSendRequestListItem*)BfxListNext(&toRecord, (BfxListItem*)item))
		{
			memcpy(stream->recordData + recordSize, item->request.buffer + item->offset, item->request.length - item->offset);
			recordSize += (item->request.length - item->offset);
            BLOG_HEX(item->request.buffer, 10, "stream(%p) record to send, size:%d", stream, item->request.length - item->offset);
        }
		assert(recordSize < toRecordSize);
        BLOG_HEX(request->buffer, 10, "stream(%p) record to send, size:%d", stream, toRecordSize - recordSize);
        memcpy(stream->recordData + recordSize, request->buffer, toRecordSize - recordSize);
		recordSize = toRecordSize;

		size_t headerSize = 0;
		BLOG_DEBUG("%s post send request(size:%d), push to interface.", StreamConnectionGetName(stream), stream->recordBufferLength);
		int result = Bdt_TcpInterfaceBoxCryptoData(
			stream->tcpInterface,
			stream->recordData,
			toRecordSize,
			stream->recordBufferLength,
			&recordSize,
			&headerSize);
		stream->recordOffset = STREAM_CONNECTION_ENCRYPT_MAX_HEADER_SIZE - headerSize;
		stream->pendingRecordSize = recordSize + headerSize;
		assert(!result);

        BLOG_INFO("stream(%p) crypto size: %d=>%d, headersize:%d", stream, (int)toRecordSize, (int)recordSize, (int)headerSize);

        // 放入缓冲区就callback，否则callback通知顺序和send投递顺序不一致；
        // 因为sendRecord操作可能会触发其他线程执行record并callback；
        // 另外的解决办法就是严格按照record顺序排队执行callback
        for (StreamConnectionSendRequestListItem* item = (StreamConnectionSendRequestListItem*)BfxListPopFront(&toRecord);
            item != NULL;
            item = (StreamConnectionSendRequestListItem*)BfxListPopFront(&toRecord))
        {
            BLOG_HEX(item->request.buffer, 10, "stream(%p) sent callback, size:%d", stream, item->request.length);
            item->request.callback(BFX_RESULT_SUCCESS, item->request.buffer, item->request.length, item->request.userData.userData);
            if (item->request.userData.lpfnReleaseUserData)
            {
                item->request.userData.lpfnReleaseUserData(item->request.userData.userData);
            }
            BfxFree(item);
        }

        if (!pending)
        {
            // tofix: should always async call callback
            BLOG_HEX(request->buffer, 10, "stream(%p) sent callback, size:%d", stream, request->length);
            requestItem->request.callback(
                BFX_RESULT_SUCCESS,
                requestItem->request.buffer,
                requestItem->request.length,
                requestItem->request.userData.userData);
            if (requestItem->request.userData.lpfnReleaseUserData)
            {
                requestItem->request.userData.lpfnReleaseUserData(requestItem->request.userData.userData);
            }
            BfxFree(requestItem);
        }

		StreamConnectionSendRecord(stream, false);
	}


	if (nagle)
	{
		BLOG_CHECK(pending);

		int32_t delay = (int32_t)((stream->config.maxNagleDelay * 1000 - (BfxTimeGetNow(false) - requestItem->postTime)) / 1000);
        if (delay < 0)
        {
            delay = 0;
        }
        else if (delay > stream->config.maxNagleDelay)
        {
            delay = (int32_t)stream->config.maxNagleDelay;
        }
		BfxUserData userData;
		StreamConnectionAsRefUserData(stream, &userData);
		BLOG_DEBUG("%s post send request, create nagle timer, delay:%d.", StreamConnectionGetName(stream), delay);
		Bdt_TimerHelperStart(&stream->nagleTimer, StreamConnectionOnNagleTimer, &userData, delay);
	}

	if (pending)
	{
		BLOG_DEBUG("send on %s, length:%zu pending", stream->name, length);
		return BFX_RESULT_PENDING;
	}
	else
	{
		BLOG_DEBUG("send on %s, length:%zu directly success", stream->name, length);
		return BFX_RESULT_SUCCESS;
	}
}

static StreamConnectionRecvRequest* StreamConnectionMakeRecvRequest(
	uint8_t* data,
	size_t len,
	BdtConnectionRecvCallback callback,
	const BfxUserData* userData
)
{
	StreamConnectionRecvRequest* req = BFX_MALLOC_OBJ(StreamConnectionRecvRequest);

	req->data = data;
	req->pos = 0;
	req->size = len;
	req->callback = callback;

	if (userData != NULL)
	{
		req->userData = *userData;
		if (userData->lpfnAddRefUserData != NULL && userData->userData != NULL)
		{
			userData->lpfnAddRefUserData(userData->userData);
		}
	}
	else
	{
		req->userData.lpfnAddRefUserData = NULL;
		req->userData.lpfnReleaseUserData = NULL;
		req->userData.userData = NULL;
	}

	return req;
}

static void StreamConnectionStopDataNotifyTimer(StreamConnection* stream)
{
	Bdt_TimerHelperStop(&stream->dataNotifyTimer);
}

static void BFX_STDCALL StreamConnectionOnDataNotifyTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	StreamConnection* stream = (StreamConnection*)userData;
	StreamConnectionStopDataNotifyTimer(stream);

    StreamConnectionNotifyCompleteRecvRequest(stream, false);
}

static void StreamConnectionNotifyCompleteRecvRequest(StreamConnection* stream, bool onlyComplete)
{
    if (!onlyComplete)
    {
        stream->isCompleteRecvNotifyOnly = onlyComplete;
    }

    if (BfxAtomicCompareAndSwap32(&stream->isRecvNotifying, 0, 1) == 1)
    {
        return;
    }

    BfxList toCallbackQueue;
    BfxListInit(&toCallbackQueue);

    while (true)
    {
        assert(BfxListIsEmpty(&toCallbackQueue));

        BfxThreadLockLock(&stream->recvQueueLock);
        {
            if (BfxListSize(&stream->completeRecvRequests) > 0)
            {
                BfxListSwap(&toCallbackQueue, &stream->completeRecvRequests);
            }
            if (!stream->isCompleteRecvNotifyOnly)
            {
                stream->isCompleteRecvNotifyOnly = true;

                StreamConnectionRecvRequest* req = (StreamConnectionRecvRequest*)BfxListFirst(&stream->recvRequestQueue);
                if (req != NULL && req->pos > 0)
                {
                    BfxListPopFront(&stream->recvRequestQueue);
                    BfxListPushBack(&toCallbackQueue, (BfxListItem*)req);
                }
            }

            if (BfxListSize(&toCallbackQueue) == 0)
            {
                BfxAtomicExchange32(&stream->isRecvNotifying, 0);
            }
        }
        BfxThreadLockUnlock(&stream->recvQueueLock);

        if (BfxListSize(&toCallbackQueue) == 0)
        {
            break;
        }

        while (BfxListSize(&toCallbackQueue) > 0)
        {
            StreamConnectionRecvRequest* req = (StreamConnectionRecvRequest*)BfxListPopFront(&toCallbackQueue);

            BLOG_HEX(req->data, req->pos > 15 ? 15: req->pos,
                "stream(%p) recv callback, size:%d, name:%s",
                stream, (int)req->pos, stream->name);

            StreamConnectionRecvRequestDone(req);
        }
    }
}

static void StreamConnectionStartDataNotifyTimer(StreamConnection* stream, int32_t timeout)
{
	BfxUserData userData;
	StreamConnectionAsRefUserData(stream, &userData);
	Bdt_TimerHelperStart(&stream->dataNotifyTimer,
		StreamConnectionOnDataNotifyTimer,
		&userData,
		timeout);
}

static uint32_t StreamConnectionRecv(
	StreamConnection* stream,
	uint8_t* data,
	size_t len,
	BdtConnectionRecvCallback callback,
	const BfxUserData* userData
)
{
	BLOG_CHECK(stream != NULL);
	BLOG_CHECK(data != NULL && len > 0);
	BLOG_CHECK(callback != NULL);

	BLOG_INFO("stream(0x%p):%s",
		stream,
		stream->name);

	bool resume = false;
	bool pending = false;
	size_t writePos = 0;
    int64_t firstRecvTime = 0;
    int32_t recvLeftTime = 0;

	BfxList completeBufferQueue;
	BfxListInit(&completeBufferQueue);

    StreamConnectionRecvRequest* req = StreamConnectionMakeRecvRequest(data, len, callback, userData);

	BfxThreadLockLock(&stream->recvQueueLock);
	do {
		while (writePos < len 
			&& !BfxListIsEmpty(&stream->recvBuffersQueue))
		{
			StreamConnectionRecvBuffer* recvBuffer = (StreamConnectionRecvBuffer*)BfxListFirst(&stream->recvBuffersQueue);
			size_t copySize = min(len - writePos, recvBuffer->size - recvBuffer->pos);
			memcpy(data + writePos, recvBuffer->data + recvBuffer->pos, copySize);
			writePos += copySize;
			recvBuffer->pos += copySize;
            if (firstRecvTime == 0)
            {
                firstRecvTime = recvBuffer->recvTime;
                recvLeftTime = (int32_t)(BfxTimeGetNow(false) - firstRecvTime - (int64_t)stream->config.recvTimeout);
                if (recvLeftTime > (int64_t)stream->config.recvTimeout)
                {
                    recvLeftTime = (int64_t)stream->config.recvTimeout;
                }
            }
			if (recvBuffer->pos == recvBuffer->size)
			{
				BfxListPopFront(&stream->recvBuffersQueue);
				BfxListPushBack(&completeBufferQueue, &recvBuffer->base);
			}
			else
			{
				break;
			}
		}

        req->pos = writePos;

		if (writePos == len
			|| stream->recvClosed
            || (writePos > 0 && recvLeftTime <= 2000)) // 剩余时间不足2ms
		{
            assert(req->pos > 0 || stream->recvClosed);
            BfxListPushBack(&stream->completeRecvRequests, (BfxListItem*)req);
			break;
		}

		pending = true;
		BfxListPushBack(&stream->recvRequestQueue, &req->base);
		if (BfxListSize(&stream->recvRequestQueue) == 1)
		{
			resume = true;
		}
	} while (false);
	BfxThreadLockUnlock(&stream->recvQueueLock);

	if (resume)
	{
		BLOG_INFO("resume, stream(0x%p):%s", stream, stream->name);
		Bdt_TcpInterfaceResume(stream->tcpInterface);
	}

	while (!BfxListIsEmpty(&completeBufferQueue))
	{
		StreamConnectionRecvBuffer* recvBuffer = (StreamConnectionRecvBuffer*)BfxListPopFront(&completeBufferQueue);
		BFX_FREE(recvBuffer);
	}

	if (pending)
	{
		if (writePos > 0)
		{
			StreamConnectionStartDataNotifyTimer(stream, recvLeftTime);
		}
	}
	else
	{
        StreamConnectionNotifyCompleteRecvRequest(stream, true);
	}
	return BFX_RESULT_SUCCESS;
}

// should not reenter
static uint32_t StreamConnectionOnTcpData(
	Bdt_TcpInterfaceOwner* owner, 
	Bdt_TcpInterface* tcpInterface, 
	const uint8_t* data, 
	size_t length)
{
	StreamConnection* stream = (StreamConnection*)owner;

	BLOG_HEX(data, length < 15? length : 15, "length=%d,stream(0x%p):%s", (int)length, stream, stream->name);

	if (length == 0)
	{
		BLOG_INFO("%s got zero data from tcp interface, close recv", stream->name);
		BfxThreadLockLock(&stream->recvQueueLock);
		{
			stream->recvClosed = true;
            if (BfxListSize(&stream->completeRecvRequests) == 0)
            {
			    BfxListSwap(&stream->recvRequestQueue, &stream->completeRecvRequests);
            }
            else
            {
                while (BfxListSize(&stream->recvRequestQueue) > 0)
                {
                    BfxListPushBack(&stream->completeRecvRequests, BfxListPopFront(&stream->recvRequestQueue));
                }
            }
		}
		BfxThreadLockUnlock(&stream->recvQueueLock);

        StreamConnectionNotifyCompleteRecvRequest(stream, false);

		return BFX_RESULT_SUCCESS;
	}

	BLOG_CHECK(length > 0);

	bool pause = false;
	bool delayNotify = false;
	bool cancelDelayNotify = false;
	bool recvClosed = false;

	BfxThreadLockLock(&stream->recvQueueLock);
	do {
		recvClosed = stream->recvClosed;
		if (recvClosed)
		{
			break;
		}

		while (length > 0 && !BfxListIsEmpty(&stream->recvRequestQueue))
		{
            assert(BfxListIsEmpty(&stream->recvBuffersQueue));

			StreamConnectionRecvRequest* req = (StreamConnectionRecvRequest*)BfxListFirst(&stream->recvRequestQueue);
			size_t copySize = min(length, req->size - req->pos);
			memcpy(req->data + req->pos, data, copySize);
			data += copySize;
			length -= copySize;
			req->pos += copySize;
			if (req->pos == req->size)
			{
				cancelDelayNotify = true;
				BfxListPopFront(&stream->recvRequestQueue);
                assert(req->pos > 0);
				BfxListPushBack(&stream->completeRecvRequests, &req->base);
			}
			else
			{
				delayNotify = true;
				break;
			}
		}

		if (length > 0)
		{
            assert(BfxListIsEmpty(&stream->recvRequestQueue));

			StreamConnectionRecvBuffer* recvBuffer = (StreamConnectionRecvBuffer*)BFX_MALLOC(sizeof(StreamConnectionRecvBuffer) + length);
			memcpy(recvBuffer->data, data, length);
			recvBuffer->size = length;
			recvBuffer->pos = 0;
            recvBuffer->recvTime = BfxTimeGetNow(false);
			BfxListPushBack(&stream->recvBuffersQueue, &recvBuffer->base);

			if (BfxListSize(&stream->recvBuffersQueue) == 1)
			{
				pause = true;
			}
		}
	} while (false);
	BfxThreadLockUnlock(&stream->recvQueueLock);

	if (recvClosed)
	{
		BLOG_DEBUG("%s ignore tcp data from tcp interface for recv closed", stream->name);
		// do nothing
		return BFX_RESULT_SUCCESS;
	}

	if (pause)
	{
		BLOG_INFO("pause, stream(0x%p):%s", stream, stream->name);
		Bdt_TcpInterfacePause(stream->tcpInterface);
		if (!BfxListIsEmpty(&stream->recvRequestQueue))
		{
			BLOG_INFO("resume, stream(0x%p):%s", stream, stream->name);
			Bdt_TcpInterfaceResume(stream->tcpInterface);
		}
	}

    StreamConnectionNotifyCompleteRecvRequest(stream, true);

	{
		if (cancelDelayNotify && delayNotify)
		{
			StreamConnectionStopDataNotifyTimer(stream);
		}
		if (delayNotify)
		{
			StreamConnectionStartDataNotifyTimer(stream, (int32_t)stream->config.recvTimeout);
		}
	}
	return BFX_RESULT_SUCCESS;
}

#endif //__TCP_STREAM_IMP_INL__