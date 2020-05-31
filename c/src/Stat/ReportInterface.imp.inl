#ifndef __BSTAT_REPORT_INTERFACE_IMP_INL__
#define __BSTAT_REPORT_INTERFACE_IMP_INL__
#ifndef __BSTAT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of Stat module"
#endif //__BSTAT_REPORT_INTERFACE_IMP_INL__

#include <libuv-1.29.1/include/uv.h>
#include <stdlib.h>

#define PROTOCOL_VERSION (0x00000101)
#define PROTOCOL_MAGIC  (0x20200406)

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 80

typedef struct SendBuffer
{
    BfxListItem base;
    int32_t length;
    uint8_t data[1];
} SendBuffer;

typedef enum REPORT_CONNECTION_STATUS
{
    REPORT_CONNECTION_STATUS_NONE = 0,
    REPORT_CONNECTION_STATUS_CONNECTING = 1,
    REPORT_CONNECTION_STATUS_CONNECTED = 2,
    REPORT_CONNECTION_STATUS_CLOSING = 3
} REPORT_CONNECTION_STATUS;

typedef struct TCP_SOCKET
{
    uv_tcp_t socket;
    volatile int32_t ref;
} TCP_SOCKET;

typedef struct WriteHandle
{
    uv_write_t req;
    int32_t length;
    ReportInterface* reporter;
    TCP_SOCKET* socket;
} WriteHandle;

typedef struct ReportInterface
{
    BuckyStat* stat;
    volatile int32_t isWillExit;

    uv_thread_t thread;
    uv_loop_t loop;

    volatile int32_t ref;

    // begin sendQueueLock
    BfxThreadLock sendQueueLock;
    REPORT_CONNECTION_STATUS connectionStatus;
    TCP_SOCKET* socket;
    volatile int32_t pendingDataSize;
    BfxList sendQueue;
    BfxList waitRespQueue;
    // end of sendQueueLock

    uint32_t cacheRespLength;
    ProtocolResponce pendingResp;

    uint16_t peeridLength;
    uint16_t productIdLength;
    uint16_t productVersionLength;
    uint16_t userProductIdLength;
    uint16_t userProductVersionLength;
    const uint8_t* peerid;
    const char* productId;
    const char* productVersion;
    const char* userProductId;
    const char* userProductVersion;
    uint16_t sourceLength;
    uint8_t sourceData[1];
} ReportInterface;

static int32_t _ReportInterfaceAddRef(ReportInterface* reporter);
static int32_t _ReportInterfaceRelease(ReportInterface* reporter);
static void _ReportInterfaceCloseSocket(ReportInterface* reporter);
static void _TcpSocketCloseCallback(uv_handle_t* handle);
static void _ReportInterfaceConnectToServer(ReportInterface* reporter, const char* host, uint16_t port);
static TCP_SOCKET* _NewTcpSocket(ReportInterface* reporter);
static void _TcpSocketAddRef(TCP_SOCKET* socket);
static void _TcpSocketRelease(TCP_SOCKET* socket);
static void _TcpSendCallback(uv_write_t* req, int status);
static void _ReportInterfaceOnData(ReportInterface* reporter, const uint8_t* data, uint32_t len);

static void _ioThread(void* data)
{
    ReportInterface* reporter = (ReportInterface*)data;
    int uvRet = uv_run(&reporter->loop, UV_RUN_DEFAULT);
    _ReportInterfaceRelease(reporter);
}

static ReportInterface* ReportInterfaceCreate(
    uint16_t peeridLength,
    const uint8_t* peerid,
    const char* productId,
    const char* productVersion,
    const char* userProductId,
    const char* userProductVersion,
    BuckyStat* stat
)
{
    uint16_t productIdLength = productId == NULL? 0 : (uint16_t)strlen(productId);
    uint16_t productVersionLength = productVersion == NULL ? 0 : (uint16_t)strlen(productVersion);
    uint16_t userProductIdLength = userProductId == NULL ? 0 : (uint16_t)strlen(userProductId);
    uint16_t userProductVersionLength = userProductVersion == NULL ? 0 : (uint16_t)strlen(userProductVersion);
    uint16_t sourceLength = sizeof(peeridLength) + peeridLength +
        sizeof(productIdLength) + productIdLength +
        sizeof(productVersionLength) + productVersionLength +
        sizeof(userProductIdLength) + userProductIdLength +
        sizeof(userProductVersionLength) + userProductVersionLength;

    ReportInterface* reporter = BfxMalloc(sizeof(ReportInterface) + 
        sourceLength
        - sizeof(((ReportInterface*)NULL)->sourceData)
    );

    reporter->stat = stat;
    BStatAddRef(stat);

    reporter->ref = 2;

    BfxThreadLockInit(&reporter->sendQueueLock);
    BfxListInit(&reporter->sendQueue);
    BfxListInit(&reporter->waitRespQueue);
    reporter->connectionStatus = REPORT_CONNECTION_STATUS_NONE;
    reporter->pendingDataSize = 0;
    reporter->socket = NULL;
    reporter->cacheRespLength = 0;

    int offset = 0;

#define FILL_SOURCE_FIELD(fieldName)    \
    reporter->fieldName # Length = fieldName # Length;  \
    *(uint16_t*)(reporter->sourceData + offset) = fieldName # Length; \
    offset += sizeof(fieldName # Length); \
    memcpy(reporter->sourceData + offset, fieldName # , fieldName # Length);    \
    offset += fieldName # Length;

    FILL_SOURCE_FIELD(peerid);
    FILL_SOURCE_FIELD(productId);
    FILL_SOURCE_FIELD(productVersion);
    FILL_SOURCE_FIELD(userProductId);
    FILL_SOURCE_FIELD(userProductVersion);

    reporter->sourceLength = sourceLength;

    reporter->loop.data = reporter;
    int uvRet = uv_loop_init(&reporter->loop);
    assert(uvRet == 0);

    reporter->isWillExit = 0;

    uvRet = uv_thread_create(&reporter->thread, _ioThread, reporter);
    assert(uvRet == 0);

    return reporter;
}

static int32_t ReportInterfaceDestroy(ReportInterface* reporter)
{
    int32_t isExited = BfxAtomicExchange32(&reporter->isWillExit, 1);
    if (isExited == 1)
    {
        return;
    }

    _ReportInterfaceCloseSocket(reporter);
    _ReportInterfaceRelease(reporter);
}

static int32_t _ReportInterfaceAddRef(ReportInterface* reporter)
{
    BfxAtomicInc32(&reporter->ref);
}

static int32_t _ReportInterfaceRelease(ReportInterface* reporter)
{
    int32_t ref = BfxAtomicDec32(&reporter->ref);
    if (ref == 1)
    {
        assert(reporter->isWillExit);
        uv_stop(reporter->loop);
    }
    else if (ref == 0)
    {
        for (BfxListItem* item = BfxListPopFront(&reporter->sendQueue);
            item != NULL;
            item = BfxListPopFront(&reporter->sendQueue)
            )
        {
            BfxFree(item);
        }

        BfxThreadLockDestroy(&reporter->socketLock);
        BfxThreadLockDestroy(&reporter->sendQueueLock);
        uv_thread_join(&reporter->thread);
        uv_loop_close(&reporter->loop);
        BStatRelease(reporter->stat);
        BfxFree(reporter);
    }
    else if (ref < 0)
    {
        assert(false);
    }

    return ref;
}

static uint32_t ReportInterfaceSend(
    ReportInterface* reporter,
    uint32_t cmdType,
    uint32_t seq,
    uint64_t timestamp,
    const uint8_t* body,
    uint32_t bodyLength
)
{
    uint32_t sendBufferSize = sizeof(ProtocolHeader) +
        reporter->sourceLength +
        sizeof(uint32_t) + bodyLength;
    SendBuffer* sendBuffer = (SendBuffer*)BfxMalloc(sizeof(SendBuffer) + sendBufferSize - sizeof(((SendBuffer*)NULL)->data));
    sendBuffer->dataLength = sendBufferSize;

    ProtocolHeader* header = (ProtocolHeader*)sendBuffer->data;
    header->protocolVersion = PROTOCOL_VERSION;
    header->length = sendBufferSize;
    header->flags = 0;
    header->magic = PROTOCOL_MAGIC;
    header->cmdType = cmdType;
    header->seq = seq;
    header->timestamp = timestamp;

    int offset = sizeof(ProtocolHeader);
    memcpy(sendBuffer->data + offset, reporter->sourceData, reporter->sourceLength);
    offset += reporter->sourceLength;
    *(uint32_t*)(sendBuffer->data + offset) = bodyLength;
    offset += sizeof(uint32_t);
    memcpy(sendBuffer->data + offset, body, bodyLength);

    REPORT_CONNECTION_STATUS oldConnectionStatus = reporter->connectionStatus;
    TCP_SOCKET* socket = NULL;

    bool needReconnect = false;
    bool pending = true;

    BfxThreadLockLock(&reporter->sendQueueLock);
    {
        BfxAtomicAdd32(&reporter->pendingDataSize, sendBuffer->length);

        oldConnectionStatus = reporter->connectionStatus;
        if (oldConnectionStatus == REPORT_CONNECTION_STATUS_NONE)
        {
            reporter->connectionStatus = REPORT_CONNECTION_STATUS_CONNECTIONG;
            assert(reporter->socket == NULL);
        }

        SendBuffer* firstWaitBuffer = (SendBuffer*)BfxListFirst(&reporter->waitRespQueue);
        ProtocolHeader* firstWaitHeader = (ProtocolHeader*)firstWaitBuffer->data;
        if (oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTED &&
            (firstWaitBuffer == NULL || (timestamp > firstWaitHeader->timestamp && timestamp - firstWaitHeader->timestamp < 5000000)))
        {
            if (reporter->pendingDataSize < 16 * 1024)
            {
                pending = false;
            }

            BfxListPushBack(&reporter->waitRespQueue, (BfxListItem*)sendBuffer);
            socket = reporter->socket;
            _TcpSocketAddRef(socket);
        }
        else
        {
            needReconnect = true;
            BfxListPushBack(&reporter->sendQueue, (BfxListItem*)sendBuffer);
        }
    }
    BfxThreadLockUnlock(&reporter->sendQueueLock);

    if (oldConnectionStatus == REPORT_CONNECTION_STATUS_NONE)
    {
        // connect
        _ReportInterfaceConnectToServer(reporter, SERVER_HOST, SERVER_PORT);
    }

    if (oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTED)
    {
        if (!needReconnect)
        {
            // send
            WriteHandle* handle = BFX_MALLOC_OBJ(WriteHandle);
            handle->socket = socket;
            handle->length = 0;
            uv_buf_t uvBuffer = {
                .base = sendBuffer->data,
                .len = sendBuffer->length
            }
            int uvRet = uv_write((uv_write_t*)handle, (uv_stream_t*)socket, &uvBuffer, 1, _TcpSendCallback);
            if (uvRet != 0)
            {
                _ReportInterfaceCloseSocket(reporter);
                _TcpSocketRelease(socket);
                BFX_FREE(handle);
            }
        }
        else
        {
            _ReportInterfaceCloseSocket(reporter);
        }
    }

    if (pending)
    {
        return BFX_RESULT_PENDING;
    }
    return BFX_RESULT_SUCCESS;
}

static TCP_SOCKET* _NewTcpSocket(ReportInterface* reporter)
{
    TCP_SOCKET* socket = BFX_MALLOC_OBJ(TCP_SOCKET);
    uv_tcp_init(&reporter->loop, socket);
    socket->ref = 1;
    socket->socket.data = reporter;
    _ReportInterfaceAddRef(reporter);
    return socket;
}

static void _TcpSocketAddRef(TCP_SOCKET* socket)
{
    BfxAtomicInc32(&socket->ref);
}

static void _TcpSocketRelease(TCP_SOCKET* socket)
{
    if (BfxAtomicDec32(&socket->ref) == 0)
    {
        ReportInterface* reporter = (ReportInterface*)socket->socket.data;
        _ReportInterfaceRelease(reporter);
        BFX_FREE(socket);
    }
}

static void _TcpReadAllocBuffer(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    buf->base = (char*)BfxMalloc(suggestedSize);
    buf->len = (ULONG)suggestedSize;
}

static void _TcpOnRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {

    TCP_SOCKET* socket = (TCP_SOCKET*)client->data;
    assert(socket);
    ReportInterface* reporter = (ReportInterface*)socket->socket.data;

    if (nread > 0) {
        _ReportInterfaceOnData(reporter, buf->base, buf->len);
    }
    else if (nread < 0) {
        _ReportInterfaceCloseSocket(reporter);
        _TcpSocketRelease(socket); // read操作持有了一个计数
    }
    else {
        // 不做处理
        // nread might be 0, which does not indicate an error or EOF. This is equivalent to EAGAIN or EWOULDBLOCK under read(2).
    }

    if (buf->base) {
        BfxFree(buf->base);
    }
}

static void _TcpSendCallback(uv_write_t* req, int status)
{
    WriteHandle* handle = (WriteHandle*)req;
    TCP_SOCKET* socket = handle->socket;
    ReportInterface* reporter = (ReportInterface*)socket->socket.data;

    if (status != 0)
    {
        _ReportInterfaceCloseSocket(reporter);
    }
    else
    {
        BfxAtomicAnd32(&reporter->pendingDataSize, -handle->length);
    }

    _TcpSocketRelease(socket);
    BFX_FREE(handle);
}

static void _TcpConnectCallback(uv_connect_t* req, int status) {
    TCP_SOCKET* socket = (TCP_SOCKET*)req->handle;
    ReportInterface* reporter = (ReportInterface*)socket->socket.data;
    assert(socket);

    BFX_FREE(req);

    bool succ = false;

    if (status == 0)
    {
        _TcpSocketAddRef(socket); // for read
        if (uv_read_start((uv_stream_t*)&socket->socket, _TcpReadAllocBuffer, _TcpOnRead) != 0)
        {
            _TcpSocketRelease(socket);
        }
        else
        {
            succ = true;
        }
    }
    
    REPORT_CONNECTION_STATUS oldConnectionStatus = REPORT_CONNECTION_STATUS_NONE;

    if (!succ)
    {
        BfxThreadLockLock(&reporter->sendQueueLock);
        assert(reporter->socket == NULL);
        oldConnectionStatus = REPORT_CONNECTION_STATUS_CLOSING;
        reporter->connectionStatus = REPORT_CONNECTION_STATUS_CLOSING;
        _TcpSocketAddRef(socket); // for close
        BfxThreadLockUnlock(&reporter->sendQueueLock);
    }
    else
    {
        // send all
        bool isFirst = true;

        while (true)
        {
            uv_buf_t* sendBuffers = NULL;
            uv_buf_t sendBuffersInStack[128];
            uint32_t bufferCount = 0;
            WriteHandle* handle = NULL;

            BfxThreadLockLock(&reporter->sendQueueLock);
            do
            {
                _TcpSocketAddRef(socket); // 转交所有权/write/close,三个分支都需要计数

                oldConnectionStatus = reporter->connectionStatus;
                if (oldConnectionStatus == REPORT_CONNECTION_STATUS_CLOSING)
                {
                    break;
                }
                assert(oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTING);

                if (isFirst)
                {
                    BfxAtomicExchange32(&reporter->pendingDataSize, 0);
                    bufferCount = (uint32_t)BfxListSize(&reporter->waitRespQueue);
                }
                bufferCount += (uint32_t)BfxListSize(reporter->sendQueue);
                if (bufferCount <= BFX_ARRAYSIZE(sendBuffersInStack))
                {
                    sendBuffers = sendBuffersInStack;
                }
                else
                {
                    sendBuffers = BFX_MALLOC_ARRAY(uv_buf_t, bufferCount);
                }

                if (bufferCount == 0)
                {
                    assert(oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTING);
                    reporter->socket = socket; // 转交所有权
                    reporter->connectionStatus = REPORT_CONNECTION_STATUS_CONNECTED;
                    break;
                }
                else
                {
                    handle = BFX_MALLOC_OBJ(WriteHandle);
                    handle->socket = socket; // for write
                    handle->length = 0;

                    int pos = 0;
                    if (isFirst)
                    {
                        // 等待响应的包，对端可能收到，也可能没收到，所以可能有重发数据
                        for (BfxListItem* item = BfxListFirst(&reporter->waitRespQueue);
                            item != NULL;
                            item = BfxListNext(&reporter->waitRespQueue, item))
                        {
                            SendBuffer* sendBuffer = (SendBuffer*)item;
                            sendBuffers[pos].base = (char*)sendBuffer->data;
                            sendBuffers[pos++].len = sendBuffer->length;
                            handle->length += sendBuffer->length;
                            BfxAtomicAdd32(&reporter->pendingDataSize, sendBuffer->length);
                        }
                    }

                    for (BfxListItem* item = BfxListPopFront(&reporter->sendQueue);
                        item != NULL;
                        item = BfxListPopFront(&reporter->sendQueue))
                    {
                        BfxListPushBack(&reporter->waitRespQueue, item);
                        SendBuffer* sendBuffer = (SendBuffer*)item;
                        sendBuffers[pos].base = (char*)sendBuffer->data;
                        sendBuffers[pos++].len = sendBuffer->length;
                        handle->length += sendBuffer->length;
                        BfxAtomicAdd32(&reporter->pendingDataSize, sendBuffer->length);
                    }
                    isFirst = false;
                }
            } while (false);
            BfxThreadLockUnlock(&reporter->sendQueueLock);


            if (oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTING)
            {
                assert(oldConnectionStatus == REPORT_CONNECTION_STATUS_CONNECTING);
                int uvRet = uv_write((uv_write_t*)handle, (uv_stream_t*)socket, sendBuffers, bufferCount, _TcpSendCallback);
                if (uvRet != 0)
                {
                    uv_close((uv_handle_t*)socket, _TcpSocketCloseCallback);
                    BFX_FREE(handle);
                }

                if (sendBuffers != sendBuffersInStack)
                {
                    BFX_FREE(sendBuffers);
                }
            }
        }
    }

    if (oldConnectionStatus == REPORT_CONNECTION_STATUS_CLOSING)
    {
        uv_close((uv_handle_t*)socket, _TcpSocketCloseCallback);
    }

    _TcpSocketRelease(socket);
}

static void _ReportInterfaceConnectToServer(ReportInterface* reporter, const char* host, uint16_t port)
{
    assert(reporter->socket == NULL);
    TCP_SOCKET* socket = _NewTcpSocket(reporter);
    uv_connect_t* connect = BFX_MALLOC_OBJ(uv_connect_t);
    ret = uv_tcp_connect(connect, &socket->socket, (const struct sockaddr*) & dest, _TcpOnConnect);
    if (ret != 0)
    {
        BfxThreadLockLock(&reporter->sendQueueLock);
        assert(reporter->socket == NULL);
        reporter->connectionStatus = REPORT_CONNECTION_STATUS_CLOSING;
        BfxThreadLockUnlock(&reporter->sendQueueLock);

        uv_close((uv_handle_t*)&socket->socket, _TcpSocketCloseCallback);

        BFX_FREE(connect);
    }
}

static void _ReportInterfaceOnData(ReportInterface* reporter, const uint8_t* data, uint32_t len)
{
    SendBuffer* nextWaitResp = (SendBuffer*)BfxListFirst(&reporter->waitRespQueue);

    if (reporter->cacheRespLength > 0)
    {
        if (reporter->cacheRespLength + len >= sizeof(ProtocolResponce))
        {
            uint32_t copySize = sizeof(ProtocolResponce) - reporter->cacheRespLength;
            memcpy(&reporter->pendingResp + reporter->cacheRespLength, data, copySize);
            StatOnResponce(reporter->stat, &reporter->pendingResp);

            assert(((ProtocolRequest*)nextWaitResp->data)->header.timestamp == reporter->pendingResp.header.timestamp);
            assert(((ProtocolRequest*)nextWaitResp->data)->header.seq == reporter->pendingResp.header.seq);
            nextWaitResp = (SendBuffer*)BfxListNext(&reporter->waitRespQueue, nextWaitResp);

            reporter->cacheRespLength = 0;
            data += copySize;
            len -= copySize;
        }
    }

    while (len >= sizeof(ProtocolResponce))
    {
        StatOnResponce(reporter->stat, (ProtocolResponce*)data);

        assert(((ProtocolRequest*)nextWaitResp->data)->header.timestamp == ((ProtocolResponce*)data)->header.timestamp);
        assert(((ProtocolRequest*)nextWaitResp->data)->header.seq == ((ProtocolResponce*)data)->header.seq);
        nextWaitResp = (SendBuffer*)BfxListNext(&reporter->waitRespQueue, nextWaitResp);

        data += sizeof(ProtocolResponce);
        len -= sizeof(ProtocolResponce);
    }

    BfxList responcedReqList;
    BfxListInit(&responcedReqList);

    BfxThreadLockLock(&reporter->sendQueueLock);
    {
        for (ProtocolRequest* responced = (ProtocolRequest*)BfxListFirst(&reporter->waitRespQueue);
            responced != nextWaitResp;
            responced = (ProtocolRequest*)BfxListFirst(&reporter->waitRespQueue)
            )
        {
            BfxListPushBack(&responcedReqList, (BfxListItem*)responced);
            BfxListPopFront(&reporter->waitRespQueue);
        }
    }
    BfxThreadLockUnlock(&reporter->sendQueueLock);

    for (BfxListItem* item = BfxListPopFront(&responcedReqList);
        item != NULL;
        item = BfxListPopFront(responcedReqList)
        )
    {
        BfxFree(item);
    }

    if (len > 0)
    {
        memcpy(&reporter->pendingResp, data, len);
    }
}

static void _TcpSocketCloseCallback(uv_handle_t* handle) {
    TCP_SOCKET* socket = (TCP_SOCKET*)handle;
    ReportInterface* reporter = (ReportInterface*)socket->socket.data;
    TCP_SOCKET workingSocket = NULL;

    BfxThreadLockLock(&reporter->sendQueueLock);
    assert(reporter->connectionStatus == REPORT_CONNECTION_STATUS_CLOSING);
    workingSocket = reporter->socket;
    assert(workingSocket == socket || workingSocket == NULL);
    reporter->socket = NULL;
    reporter->connectionStatus = REPORT_CONNECTION_STATUS_NONE;
    BfxThreadLockUnlock(&reporter->sendQueueLock);

    if (workingSocket != NULL)
    {
        _TcpSocketRelease(workingSocket);
    }
    _TcpSocketRelease(socket);
}

static void _ReportInterfaceCloseSocket(ReportInterface* reporter)
{
    TCP_SOCKET* tcpSocket;
    REPORT_CONNECTION_STATUS oldConnectionStatus = REPORT_CONNECTION_STATUS_NONE;

    BfxThreadLockLock(&reporter->sendQueueLock);
    oldConnectionStatus = reporter->connectionStatus;
    if (oldConnectionStatus != REPORT_CONNECTION_STATUS_NONE && oldConnectionStatus != REPORT_CONNECTION_STATUS_CLOSING)
    {
        tcpSocket = reporter->socket;
        reporter->connectionStatus = REPORT_CONNECTION_STATUS_CLOSING;
        if (tcpSocket != NULL)
        {
            _TcpSocketAddRef(tcpSocket);
        }
    }
    BfxThreadLockUnlock(&reporter->sendQueueLock);

    if (oldConnectionStatus != REPORT_CONNECTION_STATUS_NONE &&
        oldConnectionStatus != REPORT_CONNECTION_STATUS_CLOSING &&
        tcpSocket != NULL)
    {
        uv_close((uv_handle_t*)tcpSocket, _TcpSocketCloseCallback);
    }
}