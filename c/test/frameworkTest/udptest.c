#include <BDT2.h>
#include <LibuvUdpSocket.h>
#include <assert.h>
#include <uv.h>
#include <stdio.h>

BDT_SYSTEM_UDP_SOCKET udpClientSocket = NULL;
BDT_SYSTEM_UDP_SOCKET udpServerSocket = NULL;
uv_sem_t semServerInit;
uv_sem_t semClientCanSendData;
uv_sem_t semClientWaitToClose;
uv_sem_t semClientStackWaitToClose;
uv_sem_t semServerWaitToClose;
uv_sem_t semServerStackWaitToClose;

extern void BFX_STDCALL BdtLibuvFrameworkDestroySelf(struct BdtSystemFramework* framework);

void BFX_STDCALL StackEventCallbackClient(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData)
{
	switch (eventID)
	{
	case 100:
	{
		BFX_BUFFER_HANDLE data = (BFX_BUFFER_HANDLE)userData;
		size_t len = 0;
		uint8_t* pData = BfxBufferGetData(data, &len);
		char* sz = (char*)malloc(sizeof(char) * (len + 1));
		memset(sz, 0, sizeof(char) * (len + 1));
		memcpy(sz, pData, len);
		BfxBufferRelease(data);
		struct sockaddr_in* addr = (struct sockaddr_in*)userData;

		printf("client receive data: %s from %d \n", sz, addr->sin_port);
		free(sz);

		break;
	}
	case BDT_TCP_SOCKET_EVENT_CLOSE:
	{
		uv_sem_post(&(semClientStackWaitToClose));
		break;
	}
	default:
		break;
	}
}

void BFX_STDCALL StackEventCallbackServer(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData)
{
	switch (eventID)
	{
	case 100:
	{
		BFX_BUFFER_HANDLE data = (BFX_BUFFER_HANDLE)userData;
		size_t len = 0;
		uint8_t* pData = BfxBufferGetData(data, &len);
		char* sz = (char*)malloc(sizeof(char) * (len + 1));
		memset(sz, 0, sizeof(char) * (len + 1));
		memcpy(sz, pData, len);
		struct sockaddr_in* addr = (struct sockaddr_in*)userData;

		printf("server receive data: %s from %d \n", sz, addr->sin_port);
		free(sz);

		uv_sem_post(&(semClientWaitToClose));
		uv_sem_post(&(semServerWaitToClose));
		break;
	}
	case BDT_TCP_SOCKET_EVENT_CLOSE:
	{
		uv_sem_post(&(semServerStackWaitToClose));
		break;
	}
	default:
		break;
	}
}

void startClient(void* data)
{
	uv_sem_wait(&semServerInit);
	BdtStack* handle = NULL;

	uint32_t errcode = BdtLibuvCreateStack(NULL, NULL, 0, StackEventCallbackClient, NULL, &handle);
	if (errcode)
	{
		assert(false);
		return ;
	}
	printf("client create stack succ \n");

	BdtEndpoint epSelf;
	BdtEndpointFromString(&epSelf, "v4tcp127.0.0.1:10000");

	uint8_t* pHandle = (uint8_t*)handle;
	//struct SystemFramework* framework = *((struct SystemFramework**)(pHandle+4));
	struct SystemFramework* framework = (struct SystemFramework*)(handle);
	udpClientSocket = BdtLibuvUdpSocketCreate(framework, NULL);
	if (!udpClientSocket)
	{
		assert(false);
		return 0;
	}
	printf("client create socket succ \n");

	BdtEndpoint epServer;
	BdtEndpointFromString(&epServer, "v4tcp127.0.0.1:10001");

	const char* pData = "0123456789";
	BFX_BUFFER_HANDLE sendBuff = BfxCreateBuffer(strlen(pData));
	size_t bufferLen;
	void* ptr = BfxBufferGetData(sendBuff, &bufferLen);
	memcpy(ptr, pData, bufferLen);

	int nSend = BdtLibuvUdpSocketSendto(udpClientSocket, &epServer, pData, bufferLen);
	if (!nSend)
	{
		assert(false);
		return 0;
	}
	BfxBufferRelease(sendBuff);
	printf("client senddata: %s to %d \n", pData, epServer.port);

	uv_sem_wait(&semClientWaitToClose);
	BdtLibuvUdpSocketClose(udpClientSocket);
	uv_sem_wait(&semClientStackWaitToClose);

	BdtLibuvFrameworkDestroySelf(framework);

	printf("client thread finish \n");
}

void startServer(void* data)
{
	BdtStack* handle = NULL;

	uint32_t errcode = BdtLibuvCreateStack(NULL, NULL, 0, StackEventCallbackServer, NULL, &handle);
	if (errcode)
	{
		assert(false);
		return 0;
	}
	printf("server create stack succ \n");

	BdtEndpoint epSelf;
	BdtEndpointFromString(&epSelf, "v4tcp127.0.0.1:10001");

	uint8_t* pHandle = (uint8_t*)handle;
	//struct SystemFramework* framework = *((struct SystemFramework**)(pHandle + 4));
	struct SystemFramework* framework = (struct SystemFramework*)(handle);
	udpServerSocket = BdtLibuvUdpSocketCreate(framework, &epSelf, NULL);
	if (!udpServerSocket)
	{
		assert(false);
		return 0;
	}
	printf("server create socket succ \n");

	uv_sem_post(&(semServerInit));

	uv_sem_wait(&semServerWaitToClose);
	BdtLibuvUdpSocketClose(udpServerSocket);
	uv_sem_wait(&semServerStackWaitToClose);
	BdtLibuvFrameworkDestroySelf(framework);
	printf("server thread finish \n");
}

int main(int argc, char** argv)
{
	uv_sem_init(&semServerInit, 0);
	uv_sem_init(&semClientCanSendData, 0);
	uv_sem_init(&semClientWaitToClose, 0);
	uv_sem_init(&semClientStackWaitToClose, 0);
	uv_sem_init(&semServerWaitToClose, 0);
	uv_sem_init(&semServerStackWaitToClose, 0);

	uv_thread_t threadClient;
	uv_thread_t threadServer;
	uv_thread_create(&threadServer, startServer, NULL);
	uv_thread_create(&threadClient, startClient, NULL);
	uv_thread_join(&threadServer);
	uv_thread_join(&threadClient);
	BdtLibuvPrintObjectMem();
}