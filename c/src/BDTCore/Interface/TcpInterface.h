/*
TcpInterface��tcpsocket�ķ�װ�����������tcpinterface��ʵ�֣�����Ӧ���в���tcpsocket�ĵط�
Client TcpInterface�ڴ���ʱ��Ӧ������Owner,����¼�ת��Owner����
Passive TcpInterface��δ�յ�firstpackageʱ����֪���Լ���Owner,��ʱ�¼�����Ҫ��OnFirstPackage��ת��BdtTunnel_OnTcpFirstPackage �д���
������ɺ��Owner,����¼�����Owner����
                     

*/
#ifndef __BDT_TCP_INTERFACE_H__
#define __BDT_TCP_INTERFACE_H__
#include "../BdtCore.h"
#include "../BdtSystemFramework.h"
#include "../Protocol/Package.h"


typedef struct Bdt_TcpListener {
	BdtSystemFramework* framework;
	BDT_SYSTEM_TCP_SOCKET socket;
	BdtEndpoint localEndpoint;
} Bdt_TcpListener;

#define TCP_MAX_BOX_SIZE (1024*64)

typedef struct Bdt_TcpInterface Bdt_TcpInterface;

typedef enum Bdt_TcpInterfaceState
{
	BDT_TCP_INTERFACE_STATE_NONE = 0,
	BDT_TCP_INTERFACE_STATE_CONNECTING,
	BDT_TCP_INTERFACE_STATE_ESTABLISH,
	BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE,
	BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP,
	BDT_TCP_INTERFACE_STATE_PACKAGE,
	BDT_TCP_INTERFACE_STATE_STREAM,
	BDT_TCP_INTERFACE_STATE_STREAM_RECV_CLOSED,
	BDT_TCP_INTERFACE_STATE_STREAM_CLOSING, 
	BDT_TCP_INTERFACE_STATE_CLOSED,
} Bdt_TcpInterfaceState;

int32_t Bdt_TcpInterfaceAddRef(const Bdt_TcpInterface* ti);
int32_t Bdt_TcpInterfaceRelease(const Bdt_TcpInterface* ti);
const BdtEndpoint* Bdt_TcpInterfaceGetLocal(const Bdt_TcpInterface* ti);
const BdtEndpoint* Bdt_TcpInterfaceGetRemote(const Bdt_TcpInterface* ti);
const char* Bdt_TcpInterfaceGetName(const Bdt_TcpInterface* ti);
bool Bdt_IsTcpEndpointPassive(const BdtEndpoint* local, const BdtEndpoint* remote);
void Bdt_MarkTcpEndpointPassive(BdtEndpoint* local, BdtEndpoint* remote, bool passive);

#define BDT_DEFINE_TCP_INTERFACE_OWNER() \
	int32_t(*addRef)(struct Bdt_TcpInterfaceOwner* owner); \
	int32_t(*release)(struct Bdt_TcpInterfaceOwner* owner); \
	uint32_t(*onEstablish)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface); \
	void(*onError)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error); \
	uint32_t(*onFirstResp)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package* package); \
	uint32_t(*onPackage)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package** packages); \
	uint32_t(*onDrain)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface); \
	uint32_t(*onData)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const uint8_t* data, size_t bytes); \
	void(*onClosed)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface); \
	void(*onLastEvent)(struct Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface);

typedef struct Bdt_TcpInterfaceOwner
{
	BDT_DEFINE_TCP_INTERFACE_OWNER()
} Bdt_TcpInterfaceOwner;


uint32_t Bdt_TcpInterfaceSetOwner(
	Bdt_TcpInterface* ti, 
	Bdt_TcpInterfaceOwner* owner);

Bdt_TcpInterfaceState Bdt_TcpInterfaceGetState(const Bdt_TcpInterface* ti);
Bdt_TcpInterfaceState Bdt_TcpInterfaceSetState(Bdt_TcpInterface* ti, Bdt_TcpInterfaceState oldState, Bdt_TcpInterfaceState newState);

//���ú�tcpinterface��ɼ���ģʽ
void Bdt_TcpInterfaceSetAesKey(Bdt_TcpInterface* ti, const uint8_t aesKey[BDT_AES_KEY_LENGTH]);

//Socket��ؽӿ�
uint32_t Bdt_TcpInterfaceConnect(Bdt_TcpInterface* ti);
uint32_t Bdt_TcpInterfaceSend(
	Bdt_TcpInterface* tcpInterface,
	uint8_t* data,
	size_t len,
	size_t* outSent);
uint32_t Bdt_TcpInterfaceClose(Bdt_TcpInterface* ti);
uint32_t Bdt_TcpInterfaceReset(Bdt_TcpInterface* ti);
uint32_t Bdt_TcpInterfacePause(Bdt_TcpInterface* tcpInterface);
uint32_t Bdt_TcpInterfaceResume(Bdt_TcpInterface* tcpInterface);

//������box������
uint32_t Bdt_TcpInterfaceBoxFirstPackage(
	Bdt_TcpInterface* self,
	const Bdt_Package** packages,
	size_t packageCount,
	uint8_t* buffer,
	size_t bufferLen,
	size_t* pWriteBytes,
	uint8_t remotePublicType,
	const uint8_t* remotePublic,
	uint16_t secretLength,
	const uint8_t* secret);

uint32_t Bdt_TcpInterfaceBoxPackage(
	Bdt_TcpInterface* self,
	const Bdt_Package** packages,
	size_t packageCount,
	uint8_t* buffer,
	size_t bufferLen,
	size_t* pWriteBytes);

uint32_t Bdt_TcpInterfaceBoxCryptoData(
	Bdt_TcpInterface* self,
	uint8_t* buffer,
	size_t dataLen,
	size_t bufferLen,
	size_t* boxlen,
	size_t* boxHeaderLen);

typedef struct Bdt_TcpInterfaceCryptoDataContext
{
	uint8_t* buffer;
	size_t bufferLen;
	uint8_t* data;
	size_t dataOffset;
	uint8_t iv[16];
	// to isolate mbedtls header from tcp interface.h
	// make sure this buffer is greater than sizeof(mbedtls_aes_context)
	uint8_t aesContext[100];
} Bdt_TcpInterfaceCryptoDataContext;

uint32_t Bdt_TcpInterfaceBeginCryptoData(
	Bdt_TcpInterface* tcpInterface, 
	uint8_t* buffer,
	size_t bufferLen,
	Bdt_TcpInterfaceCryptoDataContext* context
);

uint32_t Bdt_TcpInterfaceAppendCryptoData(
	Bdt_TcpInterface* tcpInterface,
	Bdt_TcpInterfaceCryptoDataContext* context, 
	const uint8_t* data, 
	size_t dataLen
);

uint32_t Bdt_TcpInterfaceFinishCryptoData(
	Bdt_TcpInterface* tcpInterface,
	Bdt_TcpInterfaceCryptoDataContext* context
);

//�Ǽ������ݲ���box
#endif //__BDT_TCP_INTERFACE_H__