#include <BDT2.h>
#include <LibuvTcpSocket.h>
#include <assert.h>
#include <uv.h>
#include <stdio.h>

uv_sem_t semclient;
uv_sem_t semserver;
uv_sem_t semListenerClose;
uv_sem_t semServerInit;
uv_sem_t semCloseClient;
uv_sem_t semClientCanSendData;
BDT_SYSTEM_TCP_SOCKET serverclientSocket = NULL;
BDT_SYSTEM_TCP_SOCKET clientSocket = NULL;

extern void BFX_STDCALL BdtLibuvFrameworkDestroySelf(struct BdtSystemFramework* framework);

void BFX_STDCALL StackEventCallbackClient(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData)
{
	switch (eventID)
	{
	case BDT_TCP_SOCKET_EVENT_ESTABLISH:
	{
		const char* pData = "0123456789";
		BFX_BUFFER_HANDLE sendBuff = BfxCreateBuffer(strlen(pData));
		size_t bufferLen;
		void* ptr = BfxBufferGetData(sendBuff, &bufferLen);
		memcpy(ptr, pData, bufferLen);

		size_t nSend = 0;
		uint32_t errcode = BdtLibuvTcpSocketSend(clientSocket, sendBuff, &nSend);
		if (errcode)
		{
			assert(false);
		}
		printf("client send data:%s \n", pData);
		BfxBufferRelease(sendBuff);
		break;
	}
	case BDT_TCP_SOCKET_EVENT_DRAIN:
	{
		BDT_SYSTEM_TCP_SOCKET socket = *((BDT_SYSTEM_TCP_SOCKET*)eventParam);
		uv_sem_post(&semClientCanSendData);
	}
	case BDT_TCP_SOCKET_EVENT_CLOSE:
	{
		printf("client socket close \n");
		uv_sem_post(&(semclient));
		break;
	}
	default:
		break;
	}
}

void startClient(void* data)
{
	//uv_sem_wait(&semServerInit);
	//BdtStack* handle = NULL;

	//uint32_t errcode = BdtLibuvCreateStack(NULL, NULL, 0, StackEventCallbackClient, NULL, &handle);
	//if (errcode)
	//{
	//	assert(false);
	//	return ;
	//}
	//printf("client create stack succ \n");

	//uint8_t* pHandle = (uint8_t*)handle;
	////struct SystemFramework* framework = *((struct SystemFramework**)(pHandle + 4));
	//struct SystemFramework* framework = (struct SystemFramework*)(handle);
	//clientSocket = BdtLibuvTcpSocketCreate(framework, NULL);
	//if (!clientSocket)
	//{
	//	assert(false);
	//	return ;
	//}
	//printf("client create socket succ \n");

	////BdtEndpoint ep = { .flag = BDT_ENDPOINT_IP_VERSION_4,.address = "127.0.0.1",.port = 10000 };
	//BdtEndpoint ep;
	//BdtEndpointFromString(&ep, "v4tcp127.0.0.1:10000");
	//errcode = BdtLibuvTcpSocketConnect(clientSocket, NULL, &ep);
	//if (errcode)
	//{
	//	assert(false);
	//	return ;
	//}

	//uv_sem_wait(&semCloseClient);
	//BdtLibuvTcpSocketClose(clientSocket, false);
	//uv_sem_wait(&semclient);
	//BdtLibuvFrameworkDestroySelf(framework);
	//printf("client thread finish \n");
}


void BFX_STDCALL StackEventCallbackServer(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData)
{
	switch (eventID)
	{
	case 1000:
	{
		printf("server listener succ \n");
		uv_sem_post(&semServerInit);
		break;
	}
	case BDT_TCP_SOCKET_EVENT_CONNECTION:
	{
		printf("server new connection \n");
		serverclientSocket = *((BDT_SYSTEM_TCP_SOCKET*)eventParam);
		break;
	}
	case 100:
	{
		BDT_SYSTEM_TCP_SOCKET socket = *((BDT_SYSTEM_TCP_SOCKET*)eventParam);
		BFX_BUFFER_HANDLE data = (BFX_BUFFER_HANDLE)userData;
		size_t len = 0;
		uint8_t* pData = BfxBufferGetData(data, &len);
		char* sz = (char*)malloc(sizeof(char) * (len + 1));
		if (sz)
		{
			memset(sz, 0, sizeof(char) * (len + 1));
			memcpy(sz, pData, len);
		}
		printf("server receive data: %s \n", sz);
		free(sz);
		BdtLibuvTcpSocketClose(socket, false);
		uv_sem_post(&(semCloseClient));
		break;
	}
	case BDT_TCP_SOCKET_EVENT_CLOSE:
	{
		BDT_SYSTEM_TCP_SOCKET socket = *((BDT_SYSTEM_TCP_SOCKET*)eventParam);
		if (socket == serverclientSocket)
		{
			printf("server client socket close \n");
			uv_sem_post(&(semserver));
		}
		else
		{
			printf("server socket close \n");
			uv_sem_post(&(semListenerClose));
		}
		break;
	}
	default:
		break;
	}
}
void startServer(void* data)
{

	//BdtStack* handle = NULL;

	//uint32_t errcode = BdtLibuvCreateStack(NULL, NULL, 0, StackEventCallbackServer, NULL, &handle);
	//if (errcode)
	//{
	//	assert(false);
	//	return ;
	//}
	//printf("server create stack succ \n");

	////uint8_t* pHandle = (uint8_t*)handle;
	////struct SystemFramework* framework = *((struct SystemFramework**)(pHandle + 4));
	//struct SystemFramework* framework = (struct SystemFramework*)(handle);
	//BDT_SYSTEM_TCP_SOCKET tcpsocket = BdtLibuvTcpSocketCreate(framework, NULL);
	//if (!tcpsocket)
	//{
	//	assert(false);
	//	return ;
	//}
	//printf("server create socket succ \n");

	////BdtEndpoint ep = { .flag = BDT_ENDPOINT_IP_VERSION_4,.address = "127.0.0.1",.port = 10000 };
	//BdtEndpoint ep;
	//BdtEndpointFromString(&ep, "v4tcp127.0.0.1:10000");
	//errcode = BdtLibuvTcpSocketListen(tcpsocket, &ep);
	//if (errcode)
	//{
	//	assert(false);
	//	return ;
	//}

	//uv_sem_wait(&semserver);
	//BdtLibuvTcpSocketClose(tcpsocket, false);
	//uv_sem_wait(&semListenerClose);
	//BdtLibuvFrameworkDestroySelf(framework);
	//printf("server thread finish \n");
}

int main(int argc, char** argv)
{
	//uv_sem_init(&semserver, 0);
	//uv_sem_init(&semclient, 0);
	//uv_sem_init(&semListenerClose, 0);
	//uv_sem_init(&semServerInit, 0);
	//uv_sem_init(&semCloseClient, 0);
	//uv_sem_init(&semClientCanSendData, 0);

	//uv_thread_t threadClient;
	//uv_thread_t threadServer;
	//uv_thread_create(&threadServer, startServer, NULL);
	//uv_thread_create(&threadClient, startClient, NULL);
	//uv_thread_join(&threadServer);
	//uv_thread_join(&threadClient);
	//BdtLibuvPrintObjectMem();
	////assert(false);
}