#include "./lpc_client.h"
#include <BuckyFramework/framework.h>
#include <libuv-1.29.1/include/uv.h>

#if defined(BFX_OS_WIN)
#define MySleep(x) Sleep((x)*1000)
#else
#include <unistd.h>
#define MySleep(x) sleep(x)
#endif

#define  BDT_LPC_COMMAND_NAME 64

#define CMD_LENGTH_BYTES sizeof(uint16_t)

typedef struct LpcCommandHandlerMap
{
	struct LpcCommandHandlerMap* left;
	struct LpcCommandHandlerMap* right;
	uint8_t color;
	char name[BDT_LPC_COMMAND_NAME];
	LpcCommandHandler handler;
}LpcCommandHandlerMap, handler_map;

static int command_handler_map_compare(const handler_map* left, const handler_map* right)
{
	return strcmp(left->name, right->name);
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(handler_map, left, right, color, command_handler_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(handler_map, left, right, color, command_handler_map_compare)


struct LpcAsyncData;
typedef void (*LpcLoopCommandHandleType)(struct LpcAsyncData* c, uv_loop_t* loop);

typedef struct LpcAsyncData
{
	struct LpcAsyncData* next;
	struct LpcAsyncData* prev;
	LpcLoopCommandHandleType handle;
}LpcAsyncData, async_data;

#define ASYNC_DATA_COMPARATOR(e1, e2) (-1)
SGLIB_DEFINE_DL_LIST_PROTOTYPES(async_data, ASYNC_DATA_COMPARATOR, prev, next);
SGLIB_DEFINE_DL_LIST_FUNCTIONS(async_data, ASYNC_DATA_COMPARATOR, prev, next);

#define  TCP_RECV_CACHE_LEN 65535
struct LpcClient
{
	BfxThreadLock handlerMapLock;
	LpcCommandHandlerMap* handlerMap;

	BfxThreadLock asyncListLock;
	LpcAsyncData* asyncList;

	int port;
	uv_sem_t loopSem;

	uv_async_t* async;
	uv_thread_t thread;
	uv_thread_t timer;

	uv_tcp_t* socket;
	uint8_t* recvCache;
	uint16_t cacheOffset;
	bool start;

	IdleHandler idleHandler;

	LpcClientStartedCallback cb;
	void* data;
};

LpcClient* LpcClientCreate(int port, LpcClientStartedCallback cb, void* data)
{
	LpcClient* client = BFX_MALLOC_OBJ(LpcClient);
	memset(client, 0, sizeof(LpcClient));
	BfxThreadLockInit(&client->handlerMapLock);
	BfxThreadLockInit(&client->asyncListLock);
	client->port = port;
	uv_sem_init(&client->loopSem, 0);
	client->recvCache = BFX_MALLOC_ARRAY(uint8_t, TCP_RECV_CACHE_LEN);
	client->cacheOffset = 0;
	client->cb = cb;
	client->data = data;
	return client;
}

void LpcClientDestory(LpcClient* client)
{
	BfxThreadLockDestroy(&client->handlerMapLock);
	BfxThreadLockDestroy(&client->asyncListLock);
	uv_sem_destroy(&client->loopSem);
	BFX_FREE(client);
}

void _ClientAsyncCB(uv_async_t* async)
{
	LpcClient* client = (LpcClient*)(async->data);
	BfxThreadLockLock(&client->asyncListLock);
	LpcAsyncData* asyncList = client->asyncList;
	client->asyncList = NULL;
	BfxThreadLockUnlock(&client->asyncListLock);

	while (true) 
	{
		LpcAsyncData* data = sglib_async_data_get_first(asyncList);
		if (!data)
		{
			break;
		}

		sglib_async_data_delete(&asyncList, data);
		data->handle(data, async->loop);
	}
}

void _IdleCb(uv_idle_t* handle)
{
	LpcClient* client = (LpcClient*)handle->data;
	if (client->idleHandler)
	{
		client->idleHandler(client->data);
	}
}

void _TimerLibuvHandler(struct LpcAsyncData* c, uv_loop_t* loop)
{
	LpcClient* client = (LpcClient*)(loop->data);
	if (client->idleHandler)
	{
		client->idleHandler(client->data);
	}
	BFX_FREE(c);
}

void _ClientThreadTimer(void* data)
{
	LpcClient* client = (LpcClient*)data;

	while (true)
	{
		MySleep(2);
		LpcAsyncData* newAsyncData = BFX_MALLOC_OBJ(LpcAsyncData);
		newAsyncData->handle = _TimerLibuvHandler;
		BfxThreadLockLock(&client->asyncListLock);
		sglib_async_data_add(&client->asyncList, newAsyncData);
		BfxThreadLockUnlock(&client->asyncListLock);
		uv_async_send(client->async);
	}
}

void _ClientThreadMain(void* data)
{
	LpcClient* client = (LpcClient*)data;

	uv_loop_t* loop = BFX_MALLOC_OBJ(uv_loop_t);
	loop->data = data;
	int ret = uv_loop_init(loop);
	assert(!ret);

	/*uv_idle_t idler;
	uv_idle_init(loop, &idler);
	idler.data = data;
	uv_idle_start(&idler, _IdleCb);*/

	client->async = BFX_MALLOC_OBJ(uv_async_t);
	client->async->data = data;
	ret = uv_async_init(loop, client->async, _ClientAsyncCB);
	assert(!ret);
	uv_sem_post(&client->loopSem);
	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	//TODO ÊÍ·ÅÄÚ´æ
}


void TcpReadBuffAllocCB(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	LpcClient* client = (LpcClient*)handle->data;
	buf->base = client->recvCache + client->cacheOffset;
	buf->len = TCP_RECV_CACHE_LEN - client->cacheOffset;
}

void TcpReadCB(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
	LpcClient* client = (LpcClient*)stream->data;
	if (nread <= 0)
	{
		return;
	}
	client->cacheOffset += (uint32_t)nread;

	do
	{
		uint32_t offset = 0;
		if (client->cacheOffset < CMD_LENGTH_BYTES)
		{
			break;
		}
		uint16_t cmdLen = *((uint16_t*)(client->recvCache));
		offset += CMD_LENGTH_BYTES;

		if (client->cacheOffset - offset < cmdLen)
		{
			break;
		}

		LpcCommand command;
		command.bytesLen = *((uint16_t*)(client->recvCache + offset));
		offset += sizeof(uint16_t);
		cmdLen -= sizeof(uint16_t);

		if (!command.bytesLen)
		{
			command.buffer = NULL;
		}
		else
		{
			command.buffer = client->recvCache + offset;
			offset += command.bytesLen;
			cmdLen -= command.bytesLen;
		}

		uint8_t* jsonStr = BFX_MALLOC_ARRAY(uint8_t, cmdLen + 1);
		memset(jsonStr, 0, cmdLen + 1);
		memcpy(jsonStr, client->recvCache + offset, cmdLen);
		offset += cmdLen;

		uint8_t* tempBuffer = client->recvCache;
		client->cacheOffset -= offset;
		if (client->cacheOffset > 0)
		{
			client->recvCache = BFX_MALLOC_ARRAY(uint8_t, TCP_RECV_CACHE_LEN);
			memcpy(client->recvCache, tempBuffer + offset, client->cacheOffset);
		}

		command.jsonRoot = cJSON_Parse(jsonStr);
		if (command.jsonRoot != NULL)
		{
			do
			{
				cJSON* nameObj = cJSON_GetObjectItem(command.jsonRoot, "name");
				if (!nameObj)
				{
					printf("this command not found 'name' flied, str=%s", cJSON_Print(command.jsonRoot));
					break;
				}

				char* name = cJSON_GetStringValue(nameObj);
				LpcCommandHandlerMap toFind;
				strcpy(toFind.name, name);
				BfxThreadLockLock(&client->handlerMapLock);
				LpcCommandHandlerMap* item = sglib_handler_map_find_member(client->handlerMap, &toFind);
				BfxThreadLockUnlock(&client->handlerMapLock);
				if (!item)
				{
					BLOG_INFO("not found handler, name=%s", name);
					break;
				}

				item->handler(&command, client->data);
			} while (false);
			cJSON_Delete(command.jsonRoot);
		}
		BFX_FREE(jsonStr);
		if (tempBuffer != client->recvCache)
		{
			BFX_FREE(tempBuffer);
		}
	} while (true);
}

void ConnectCb(uv_connect_t* req, int status)
{
	LpcClient* client = (LpcClient*)(req->data);
	const char* err = uv_strerror(status);
	if (!status)
	{
		client->start = true;
		uv_read_start((uv_stream_t*)client->socket, TcpReadBuffAllocCB, TcpReadCB); 
	}

	BFX_FREE(req);

	client->cb(client->data);
}
void ConnectLibuvHandler(struct LpcAsyncData* c, uv_loop_t* loop)
{
	LpcClient* client = (LpcClient*)(loop->data);
	client->socket = BFX_MALLOC_OBJ(uv_tcp_t);
	client->socket->data = client;
	uv_tcp_init(loop, client->socket);

	uv_connect_t* connObj = BFX_MALLOC_OBJ(uv_connect_t);
	connObj->data = client;

	struct sockaddr_in addr;
	uv_ip4_addr("127.0.0.1", client->port, &addr);

	int ret = uv_tcp_connect(connObj, client->socket, (struct sockaddr*)&addr, ConnectCb);
	if (ret)
	{
		BFX_FREE(connObj);
	}
	BFX_FREE(c);
}

uint32_t LpcClientStart(LpcClient* client)
{
	if (client->start)
	{
		return BFX_RESULT_INVALID_STATUS;
	}
	

	int ret = uv_thread_create(&client->thread, _ClientThreadMain, client);
	uv_sem_wait(&client->loopSem);

	ret = uv_thread_create(&client->timer, _ClientThreadTimer, client);
	LpcAsyncData* newAsyncData = BFX_MALLOC_OBJ(LpcAsyncData);
	newAsyncData->handle = ConnectLibuvHandler;
	BfxThreadLockLock(&client->asyncListLock);
	sglib_async_data_add(&client->asyncList, newAsyncData);
	BfxThreadLockUnlock(&client->asyncListLock);
	uv_async_send(client->async);
	
	return BFX_RESULT_SUCCESS;
}

uint32_t LpcClientAddHandler(LpcClient* client, const char* name, LpcCommandHandler handler)
{
	if (strlen(name) > BDT_LPC_COMMAND_NAME)
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	LpcCommandHandlerMap* item = BFX_MALLOC_OBJ(LpcCommandHandlerMap);
	strcpy(item->name, name);
	item->handler = handler;

	uint32_t errcode = BFX_RESULT_SUCCESS;
	BfxThreadLockLock(&client->handlerMapLock);
	LpcCommandHandlerMap* findItem = sglib_handler_map_find_member(client->handlerMap, item);
	if (!findItem)
	{
		sglib_handler_map_add(&client->handlerMap, item);
	}
	else
	{
        BFX_FREE(item);
		errcode = BFX_RESULT_ALREADY_EXISTS;
	}
	BfxThreadLockUnlock(&client->handlerMapLock);

	return errcode;
}


void TcpWriteCB(uv_write_t* req, int status)
{

}

typedef struct LpcAsyncDataSend
{
	LpcAsyncData base;
	LpcCommand* command;
}LpcAsyncDataSend;

void SendLibuvHandler(struct LpcAsyncData* c, uv_loop_t* loop)
{
	LpcClient* client = (LpcClient*)(loop->data);

	LpcAsyncDataSend* data = (LpcAsyncDataSend*)c;
	char* buffer = cJSON_PrintUnformatted(data->command->jsonRoot);
	uint16_t len = (uint16_t)strlen(buffer) + (uint16_t)sizeof(uint16_t) + data->command->bytesLen;

	if (data->command->bytesLen)
	{
		uv_buf_t uvBuffer[4];
		uvBuffer[0].len = CMD_LENGTH_BYTES;
		uvBuffer[0].base = (char*)&len;

		uvBuffer[1].len = sizeof(uint16_t);
		uvBuffer[1].base = (char*)(&data->command->bytesLen);

		uvBuffer[2].len = data->command->bytesLen;
		uvBuffer[2].base = (char*)(data->command->buffer);

		uvBuffer[3].len = (uint16_t)strlen(buffer);
		uvBuffer[3].base = buffer;
		int ret = uv_try_write((uv_stream_t*)(client->socket), uvBuffer, 4);
	}
	else
	{
		uv_buf_t uvBuffer[3];
		uvBuffer[0].len = CMD_LENGTH_BYTES;
		uvBuffer[0].base = (char*)&len;

		uvBuffer[1].len = sizeof(uint16_t);
		uvBuffer[1].base = (char*)(&data->command->bytesLen);

		uvBuffer[2].len = (uint16_t)strlen(buffer);
		uvBuffer[2].base = buffer;
		int ret = uv_try_write((uv_stream_t*)(client->socket), uvBuffer, 3);
	}
	
	if (data->command->buffer)
	{
		BFX_FREE(data->command->buffer);
	}
	cJSON_Delete(data->command->jsonRoot);
	BFX_FREE(data->command);
}
uint32_t LpcClientSendCommand(LpcClient* client, LpcCommand* command)
{
	LpcAsyncDataSend* newAsyncData = BFX_MALLOC_OBJ(LpcAsyncDataSend);
	newAsyncData->base.handle = SendLibuvHandler;
	newAsyncData->command = command;

	BfxThreadLockLock(&client->asyncListLock);
	sglib_async_data_add(&client->asyncList, (LpcAsyncData*)newAsyncData);
	BfxThreadLockUnlock(&client->asyncListLock);
	uv_async_send(client->async);

	return BFX_RESULT_SUCCESS;
}

uint32_t LpcClientAddIldeHandler(LpcClient* client, IdleHandler handler)
{
	client->idleHandler = handler;
	return BFX_RESULT_SUCCESS;
}
