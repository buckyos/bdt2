#ifndef __BDT_TCP_TUNNEL_IMP_INL__
#define __BDT_TCP_TUNNEL_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./TcpTunnel.inl"
#include "./TunnelContainer.inl"
#include "./SocketTunnel.inl"
#include "../Protocol/ProtocolModule.h"
#include <time.h>
#include <stdlib.h>

typedef struct TcpTunnelSendBuffer {
	//用来存放box好的buffer
	uint8_t boxBuffer[TCP_MAX_BOX_SIZE];
	size_t boxBufferLength;
	size_t boxBufferUsed;
} TcpTunnelSendBuffer;

// Send Buffer 
static void TcpTunnelSendBufferInit(TcpTunnelSendBuffer* sendBuffer)
{
	sendBuffer->boxBufferLength = 0;
	sendBuffer->boxBufferUsed = 0;
}

static uint8_t* TcpTunnelSendBufferGetBoxBuffer(TcpTunnelSendBuffer* sendBuffer)
{
	return sendBuffer->boxBuffer;
}

static uint8_t* TcpTunnelSendBufferGetBoxBufferData(TcpTunnelSendBuffer* sendBuffer,size_t* pRealLength)
{
	*pRealLength = sendBuffer->boxBufferLength - sendBuffer->boxBufferUsed;
	return sendBuffer->boxBuffer + sendBuffer->boxBufferUsed;
}

static void TcpTunnelSendBufferSetBoxBufferLength(TcpTunnelSendBuffer* sendBuffer, size_t boxBufferLength)
{
	sendBuffer->boxBufferLength = boxBufferLength;
}

static void TcpTunnelSendBufferUseData(TcpTunnelSendBuffer* sendBuffer, size_t boxBufferUsed)
{
	if (sendBuffer->boxBufferLength == sendBuffer->boxBufferUsed + boxBufferUsed)
	{
		sendBuffer->boxBufferUsed = 0;
		sendBuffer->boxBufferLength = 0;
	}
	else
	{
		sendBuffer->boxBufferUsed += boxBufferUsed;
	}
}

static int TcpTunnelSendBufferIsEmpty(TcpTunnelSendBuffer* sendBuffer)
{
	return sendBuffer->boxBufferLength == 0;
}

//=================================================================================================================
struct Bdt_TcpTunnel
{
	DECLARE_SOCKET_TUNNEL()
	BDT_DEFINE_TCP_INTERFACE_OWNER()

	int32_t refCount;

	BdtSystemFramework* framework;
	Bdt_NetManager* netManager;
	Bdt_StackTls* tls;
	Bdt_TunnelContainer* owner;

	BdtPeerFinder* peerFinder;	
	BdtTcpTunnelConfig config;

	Bdt_TcpTunnelState state;
	Bdt_TcpInterface* tcpInterface;
	
	// sendBufferLock protected members begin
	BfxThreadLock sendBufferLock;
	TcpTunnelSendBuffer sendBuffer;
	// sendBufferLock protected members end

	uint32_t connectSeq;
	int32_t activeCount;
};


static Bdt_TcpInterfaceOwner* TcpTunnelAsInterfaceOwner(TcpTunnel* action)
{
	return (Bdt_TcpInterfaceOwner*)&(action->addRef);
}

static TcpTunnel* TcpTunnelFromInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	static TcpTunnel ins;
	return (TcpTunnel*)((intptr_t)owner - ((intptr_t)TcpTunnelAsInterfaceOwner(&ins) - (intptr_t)&ins));
}

static int32_t TcpTunnelAsInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	return Bdt_TcpTunnelAddRef(TcpTunnelFromInterfaceOwner(owner));
}

static int32_t TcpTunnelAsInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	return Bdt_TcpTunnelRelease(TcpTunnelFromInterfaceOwner(owner));
}

static void TcpTunnelOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error);
static uint32_t TcpTunnelOnTcpEstablish(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface);
static uint32_t TcpTunnelOnTcpDrain(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface);
static uint32_t TcpTunnelOnTcpPackage(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package** packages);

static void TcpTunnelInitAsInterfaceOwner(TcpTunnel* tunnel)
{
	tunnel->addRef = TcpTunnelAsInterfaceOwnerAddRef;
	tunnel->release = TcpTunnelAsInterfaceOwnerRelease;
	tunnel->onEstablish = TcpTunnelOnTcpEstablish;
	tunnel->onDrain = TcpTunnelOnTcpDrain;
	tunnel->onFirstResp = TcpTunnelOnTcpFirstResp;
	tunnel->onPackage = TcpTunnelOnTcpPackage; 
	tunnel->onError = TcpTunnelOnTcpError;
}

static void TcpTunnelOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	TcpTunnel* tunnel = TcpTunnelFromInterfaceOwner(owner);
	if (BDT_TCP_TUNNEL_STATE_DEAD == BfxAtomicExchange32((int32_t*)&tunnel->state, BDT_TCP_TUNNEL_STATE_DEAD))
	{
		TunnelContainerOnTunnelDead(tunnel->owner, (SocketTunnel*)tunnel, SocketTunnelGetLastRecv((SocketTunnel*)tunnel));
	}
}

// with lock
static uint32_t TcpTunnelBindTcpInterface(
	TcpTunnel* tunnel, 
	Bdt_TcpInterface* tcpInterface)
{
	int result = Bdt_TcpInterfaceSetOwner(
		tcpInterface,
		TcpTunnelAsInterfaceOwner(tunnel));
	assert(result == BFX_RESULT_SUCCESS);
	Bdt_TcpInterfaceAddRef(tcpInterface);
	tunnel->tcpInterface = tcpInterface;
	
	return result;
}

static void TcpTunnelEnterAlive(Bdt_TcpTunnel* tunnel)
{
	// send buffer init
	TcpTunnelSendBufferInit(&tunnel->sendBuffer);
	BfxAtomicCompareAndSwap32((int32_t*)&tunnel->state, BDT_TCP_TUNNEL_STATE_CONNECTING, BDT_TCP_TUNNEL_STATE_ALIVE);
}



int32_t Bdt_TcpTunnelAddRef(Bdt_TcpTunnel* tunnel)
{
	return BfxAtomicInc32(&tunnel->refCount);
}

int32_t Bdt_TcpTunnelRelease(Bdt_TcpTunnel* tunnel)
{
	int32_t refCount = BfxAtomicDec32(&tunnel->refCount);
	if (refCount <= 0) {

		BfxRwLockDestroy(&tunnel->lastRecvLock);
		
		BfxFree(tunnel);
	}
	return refCount;
}

static TcpTunnel* TcpTunnelCreate(
	BdtSystemFramework* framework,
	Bdt_NetManager* netManager,
	Bdt_StackTls* tls,
	BdtPeerFinder* peerFinder,
	Bdt_TunnelContainer* owner, 
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	const BdtTcpTunnelConfig* config
)
{
	TcpTunnel* tunnel = BFX_MALLOC_OBJ(TcpTunnel);
	memset(tunnel, 0, sizeof(TcpTunnel));
	TcpTunnelInitAsInterfaceOwner(tunnel);

	tunnel->flags = SOCKET_TUNNEL_FLAGS_TCP;
	tunnel->state = BDT_TCP_TUNNEL_STATE_UNKNOWN;
	// tofix: loop ref; should release in close method!!!
	Bdt_TunnelContainerAddRef(owner);

	tunnel->owner = owner;
	tunnel->refCount = 1;
	tunnel->tls = tls;
	tunnel->peerFinder = peerFinder;
	tunnel->netManager = netManager;
	tunnel->config = *config;

	tunnel->localEndpoint = *localEndpoint;
	tunnel->remoteEndpoint = *remoteEndpoint;
	tunnel->remotePeerid = *BdtTunnel_GetRemotePeerid(owner);

	BfxRwLockInit(&tunnel->lastRecvLock);
	

	const char* prename = "TcpTunnel";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(localEndpoint, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remoteEndpoint, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	tunnel->name = name;

	return tunnel;
}

static void TcpTunnelGenSynPackage(
	TcpTunnel* tunnel, 
	const Bdt_TunnelEncryptOptions* encrypt, 
	Bdt_Package** packages, 
	uint8_t* outCount)
{
	uint8_t packageCount = 0;

	Bdt_SynTunnelPackage* syn = Bdt_SynTunnelPackageCreate();
	syn->fromPeerid = *BdtTunnel_GetLocalPeerid(tunnel->owner);
	syn->toPeerid = tunnel->remotePeerid;
	syn->seq = tunnel->connectSeq;
	syn->fromContainerId = BDT_UINT32_ID_INVALID_VALUE;
	syn->sendTime = 0;
	syn->proxyPeerInfo = NULL;
	syn->peerInfo = BdtPeerFinderGetLocalPeer(tunnel->peerFinder);

	if (encrypt->exchange)
	{
		Bdt_ExchangeKeyPackage* exchange = Bdt_ExchangeKeyPackageCreate();
		Bdt_ExchangeKeyPackageFillWithNext(exchange, (Bdt_Package*)syn);

		packages[packageCount] = (Bdt_Package*)exchange;
		packageCount++;
	}

	packages[packageCount] = (Bdt_Package*)&syn;
	packageCount++;

	*outCount = packageCount;
}

static BFX_BUFFER_HANDLE TcpTunnelGenSnCallPayload(
	TcpTunnel* tunnel,
	const Bdt_Package* callPackage, 
	const Bdt_Package** firstPackages,
	size_t* inoutCount
)
{
	Bdt_TunnelEncryptOptions encrypt;
	BdtTunnel_GetEnryptOptions(tunnel->owner, &encrypt);
	Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	uint8_t synCount = 0;
	TcpTunnelGenSynPackage(tunnel, &encrypt, packages, &synCount);
	// todo: measure first packages, may not merge firstPackages to payload
	for (size_t ix = 0; ix < *inoutCount; ++ix)
	{
		firstPackages[synCount + ix] = firstPackages[ix];
	}
	
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(tunnel->tls);
	size_t encodeLength = sizeof(tlsData->udpEncodeBuffer);
	uint32_t result = BFX_RESULT_SUCCESS;
	if (encrypt.exchange)
	{
		result = Bdt_BoxEncryptedUdpPackageStartWithExchange(
			packages,
			*inoutCount, 
			callPackage,
			encrypt.key,
			encrypt.remoteConst->publicKeyType,
			(const uint8_t*)&encrypt.remoteConst->publicKey,
			encrypt.localSecret->secretLength,
			(const uint8_t*)&encrypt.localSecret->secret,
			tlsData->udpEncodeBuffer,
			&encodeLength
		);
	}
	else
	{
		result = Bdt_BoxEncryptedUdpPackage(
			packages,
			*inoutCount, 
			callPackage,
			encrypt.key,
			tlsData->udpEncodeBuffer,
			&encodeLength
		);
	}

	for (uint8_t ix = 0; ix < synCount; ++ix)
	{
		Bdt_PackageFree(packages[ix]);
	}
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_DEBUG("%s box udp package failed for %u", tunnel->name, result);
		*inoutCount = 0;
		return NULL;
	}

	BFX_BUFFER_HANDLE payload = BfxCreateBuffer(encodeLength);
	BfxBufferWrite(payload, 0, encodeLength, tlsData->udpEncodeBuffer, 0);

	return payload;
}

static void BFX_STDCALL TcpTunnelAsRefUserDataAddRef(void* userData)
{
	Bdt_TcpTunnelAddRef((TcpTunnel*)userData);
}

static void BFX_STDCALL TcpTunnelAsRefUserDataRelease(void* userData)
{
	Bdt_TcpTunnelRelease((TcpTunnel*)userData);
}

static void TcpTunnelAsRefUserData(TcpTunnel* tunnel, BfxUserData* userData)
{
	userData->lpfnAddRefUserData = TcpTunnelAsRefUserDataAddRef;
	userData->lpfnReleaseUserData = TcpTunnelAsRefUserDataRelease;
	userData->userData = tunnel;
}

static void TcpTunnelSnCallCallback(
	uint32_t errorCode, 
	BdtSnClient_CallSession* session, 
	const BdtPeerInfo* remotePeerInfo, 
	void* userData)
{
	// do nothing
}

static uint32_t TcpTunnelRequestReverseConnect(
	TcpTunnel* tunnel,
	uint32_t seq,
	const Bdt_Package** packages,
	size_t* inoutCount
)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BdtPeerFinder* peerFinder = TunnelContainerGetPeerFinder(tunnel->owner);
	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetCached(peerFinder, BdtTunnel_GetRemotePeerid(tunnel->owner));
	if (!remoteInfo)
	{
		return BFX_RESULT_FAILED;
	}
	size_t snListLen = 0;
	const BdtPeerid* snList = BdtPeerInfoListSn(remoteInfo, &snListLen);
	if (!snListLen)
	{
		BdtReleasePeerInfo(remoteInfo);
		return BFX_RESULT_FAILED;
	}
	//todo: use which sn?
	const BdtPeerInfo* snInfo = BdtPeerFinderGetCached(tunnel->peerFinder, snList + 0);
	BdtReleasePeerInfo(remoteInfo);
	if (!snInfo)
	{
		return BFX_RESULT_FAILED;
	}

	BdtSuperNodeClient* snClient = TunnelContainerGetSnClient(tunnel->owner);
	BdtSnClient_CallSession* session = NULL;

	BfxUserData userData;
	TcpTunnelAsRefUserData(tunnel, &userData);

    BdtPeerInfo* localPeerInfo = NULL;
    Bdt_PackageWithRef* callPkg = NULL;
	result = BdtSnClient_CreateCallSession(
		snClient,
        localPeerInfo,
		BdtTunnel_GetRemotePeerid(tunnel->owner),
		Bdt_SocketTunnelGetLocalEndpoint((SocketTunnel*)tunnel),
		1,
		snInfo,
		false,
		true,
		TcpTunnelSnCallCallback,
		&userData,
		&session,
        &callPkg);
	assert(result == BFX_RESULT_SUCCESS);
	BdtReleasePeerInfo(snInfo);

	BFX_BUFFER_HANDLE payload = TcpTunnelGenSnCallPayload(tunnel, Bdt_PackageWithRefGet(callPkg), packages, inoutCount);
	Bdt_PackageRelease(callPkg);

	result = BdtSnClient_CallSessionStart(session, payload);
	BfxBufferRelease(payload);

	assert(result == BFX_RESULT_SUCCESS);
	BdtSnClient_CallSessionRelease(session);
	return BFX_RESULT_SUCCESS;
}

static void BFX_STDCALL TcpTunnelOnConnectTimeout(BDT_SYSTEM_TIMER timer, void* userData)
{
	TcpTunnel* tunnel = (TcpTunnel*)userData;
	if (BDT_TCP_TUNNEL_STATE_CONNECTING == BfxAtomicCompareAndSwap32(
		(int32_t*)&tunnel->state, 
		BDT_TCP_TUNNEL_STATE_CONNECTING, 
		BDT_TCP_TUNNEL_STATE_DEAD))
	{
		TunnelContainerOnTunnelDead(tunnel->owner, (SocketTunnel*)tunnel, SocketTunnelGetLastRecv((SocketTunnel*)tunnel));
	}
}

static uint32_t TcpTunnelConnect(
	TcpTunnel* tunnel, 
	uint32_t seq, 
	const Bdt_Package** packages, 
	size_t* inoutCount, 
	TcpTunnelState* outState)
{
	BLOG_DEBUG("%s connect with seq %u", tunnel->name, seq);
	
	TcpTunnelState oldState = BfxAtomicCompareAndSwap32((int32_t*)&tunnel->state, BDT_TCP_TUNNEL_STATE_UNKNOWN, BDT_TCP_TUNNEL_STATE_CONNECTING);
	*outState = oldState;
	if (oldState != BDT_TCP_TUNNEL_STATE_UNKNOWN)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	BfxUserData udTimeout;
	TcpTunnelAsRefUserData(tunnel, &udTimeout);
	BdtCreateTimeout(
		tunnel->framework, 
		TcpTunnelOnConnectTimeout, 
		&udTimeout, 
		tunnel->config.connectTimeout);
	uint32_t result = BFX_RESULT_SUCCESS;
	if (Bdt_IsTcpEndpointPassive(&tunnel->localEndpoint, &tunnel->remoteEndpoint))
	{
		Bdt_TcpInterface* tcpInterface = NULL;
		Bdt_NetManagerCreateTcpInterface(
			tunnel->netManager,
			&tunnel->localEndpoint,
			&tunnel->remoteEndpoint,
			&tcpInterface);
		assert(tcpInterface);
		result = TcpTunnelBindTcpInterface(tunnel, tcpInterface);
		Bdt_TcpInterfaceRelease(tcpInterface);

		if (result == BFX_RESULT_SUCCESS)
		{
			tunnel->connectSeq = seq;
			result = Bdt_TcpInterfaceConnect(tcpInterface);
		}
	}
	else
	{
		result = TcpTunnelRequestReverseConnect(tunnel, seq, packages, inoutCount);
	}

	return result;
}


Bdt_TcpTunnelState Bdt_TcpTunnelGetState(Bdt_TcpTunnel* tunnel)
{
	return tunnel->state;
}


void Bdt_TcpTunnelMarkDead(Bdt_TcpTunnel* tunnel)
{
	if (BDT_TCP_TUNNEL_STATE_UNKNOWN == BfxAtomicCompareAndSwap32(
		(int32_t*)&tunnel->state,
		BDT_TCP_TUNNEL_STATE_UNKNOWN,
		BDT_TCP_TUNNEL_STATE_DEAD))
	{
		TunnelContainerOnTunnelDead(tunnel->owner, (SocketTunnel*)tunnel, SocketTunnelGetLastRecv((SocketTunnel*)tunnel));
	}
}


static uint32_t TcpTunnelOnTcpEstablish(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	TcpTunnel* tunnel = TcpTunnelFromInterfaceOwner(owner);
	Bdt_TunnelContainer* tunnelContainer = tunnel->owner;
	
	//给TcpInterface对象设置正确的初始状态,准备firstPackages
	Bdt_TunnelEncryptOptions encrypt;
	BdtTunnel_GetEnryptOptions(tunnel->owner, &encrypt);
	Bdt_Package* firstPackages[2];
	uint8_t packageCount = 0;
	
	TcpTunnelGenSynPackage(tunnel, &encrypt, firstPackages, &packageCount);

	Bdt_TcpInterfaceSetAesKey(tcpInterface, encrypt.key);

	Bdt_TcpInterfaceSetState(tcpInterface, BDT_TCP_INTERFACE_STATE_ESTABLISH, BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);

	uint32_t result = BdtTunnel_SendFirstTcpPackage(tunnel->tls, tcpInterface, firstPackages, packageCount, &encrypt);

	for (size_t ix = 0; ix < packageCount; ++ix)
	{
		Bdt_PackageFree(firstPackages[ix]);
	}
	
	return BFX_RESULT_SUCCESS;
}



static uint32_t TcpTunnelOnTcpFirstResp(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* ti, const Bdt_Package* package)
{
	TcpTunnel* tunnel = TcpTunnelFromInterfaceOwner(owner);
	
	if (package->cmdtype != BDT_ACK_TUNNEL_PACKAGE)
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	Bdt_AckTunnelPackage* ack = (Bdt_AckTunnelPackage*)package;
	BdtPeerFinderAddCached(tunnel->peerFinder, ack->peerInfo, 0);

	Bdt_TcpInterfaceSetState(ti, BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP, BDT_TCP_INTERFACE_STATE_PACKAGE);
	TcpTunnelEnterAlive(tunnel);

	BdtTunnel_OnTcpFirstResp(tunnel->owner, tunnel, ti, ack->peerInfo);

	return BFX_RESULT_SUCCESS;
}

static uint32_t TcpTunnelOnTcpFirstPackage(
	TcpTunnel* tunnel,
	Bdt_TcpInterface* tcpInterface, 
	const Bdt_Package* firstPackage)
{
	if (firstPackage->cmdtype != BDT_SYN_TUNNEL_PACKAGE)
	{
		return BFX_RESULT_INVALID_PARAM;
	}
	// tofix: verify syn package 
	Bdt_TcpTunnelState oldState = BfxAtomicCompareAndSwap32(
		(int32_t*)&tunnel->state,
		BDT_TCP_TUNNEL_STATE_UNKNOWN,
		BDT_TCP_TUNNEL_STATE_ALIVE
	);
	if (oldState == BDT_TCP_TUNNEL_STATE_DEAD)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	else if (oldState == BDT_TCP_TUNNEL_STATE_CONNECTING)
	{
		// may enter connecting state by send sn call
		// either remote peer connect local by got sn called or call connect from tunnel send or builder, enter alive 
		oldState = BfxAtomicCompareAndSwap32(
			(int32_t*)&tunnel->state,
			BDT_TCP_TUNNEL_STATE_CONNECTING,
			BDT_TCP_TUNNEL_STATE_ALIVE
		);
	}
	TcpTunnelBindTcpInterface(tunnel, tcpInterface);

	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_Package* package = NULL;
	
	const Bdt_SynTunnelPackage* syn = (Bdt_SynTunnelPackage*)firstPackage;
	BdtPeerFinderAddCached(tunnel->peerFinder, syn->peerInfo, 0);
	result = TcpTunnelBindTcpInterface(
		tunnel, 
		tcpInterface);
	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}
	Bdt_AckTunnelPackage* ack = Bdt_AckTunnelPackageCreate(); 
	package = (Bdt_Package*)ack;
	Bdt_AckTunnelPackageInit(ack);
	ack->seq = syn->seq;
	ack->peerInfo = BdtPeerFinderGetLocalPeer(tunnel->peerFinder);
	Bdt_TcpInterfaceSetState(tcpInterface, BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE, BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);
	
	const Bdt_Package* packages[] = { package };

	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(tunnel->tls);
	size_t encodeLen = 0;
	result = Bdt_TcpInterfaceBoxPackage(
		tcpInterface,
		packages,
		1,
		tlsData->tcpEncodeBuffer,
		sizeof(tlsData->tcpEncodeBuffer),
		&encodeLen);
	Bdt_PackageFree(package);
	if (result)
	{
		return result;
	}
	size_t sent = 0;
	result = Bdt_TcpInterfaceSend(tcpInterface, tlsData->tcpEncodeBuffer, encodeLen, &sent);
	if (result)
	{
		assert(false);
	}

	Bdt_TunnelContainer* tunnelContainer = tunnel->owner;
	TunnelContainerOnTcpTunnelReverseConnected(tunnelContainer, tunnel);
	return BFX_RESULT_SUCCESS;
}


static void TcpTunnelTrySend(Bdt_TcpTunnel* tunnel)
{
	Bdt_TcpInterface* tcpInterface = tunnel->tcpInterface;

	if (TcpTunnelSendBufferIsEmpty(&(tunnel->sendBuffer)))
	{
		return;
	}

	//尝试发送sendbox中剩下的数据
	size_t datalen;
	size_t sent;
	int ret = 0;
	uint8_t* pData = NULL;

	pData = TcpTunnelSendBufferGetBoxBufferData(&(tunnel->sendBuffer), &datalen);
	Bdt_TcpInterfaceSend(tcpInterface, pData, datalen, &sent);
	if (sent > 0)
	{
		TcpTunnelSendBufferUseData(&(tunnel->sendBuffer), sent);
	}
	else
	{
		BLOG_WARN("send box data in tcp drain event error!");
	}
}


static uint32_t TcpTunnelOnTcpDrain(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	TcpTunnel* tunnel = TcpTunnelFromInterfaceOwner(owner);
	TcpTunnelTrySend(tunnel);
	return BFX_RESULT_SUCCESS;
}

static uint32_t TcpTunnelSend(
	TcpTunnel* tunnel,
	const Bdt_Package** packages,
	size_t packageCount
)
{
	uint32_t r = BFX_RESULT_SUCCESS;
	TcpTunnelState state = Bdt_TcpTunnelGetState(tunnel);
	if (state == BDT_TCP_TUNNEL_STATE_DEAD)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	else if (state == BDT_TCP_TUNNEL_STATE_ALIVE)
	{
		if (TcpTunnelSendBufferIsEmpty(&(tunnel->sendBuffer)))
		{
			uint8_t* boxBuffer = TcpTunnelSendBufferGetBoxBuffer(&(tunnel->sendBuffer));
			size_t writeBytes = 0;
			r = Bdt_TcpInterfaceBoxPackage(tunnel->tcpInterface, packages, packageCount, boxBuffer, TCP_MAX_BOX_SIZE, &writeBytes);
			if (r != BFX_RESULT_SUCCESS)
			{
				BLOG_WARN("tcp tunnel encode error:%d", r);
				return BFX_RESULT_INVALID_FORMAT;
			}
			TcpTunnelSendBufferSetBoxBufferLength(&(tunnel->sendBuffer), writeBytes);

			r = BFX_RESULT_FAILED;
			size_t sent = 0;
			r = Bdt_TcpInterfaceSend(tunnel->tcpInterface, boxBuffer, writeBytes, &sent);
			if (r == BFX_RESULT_SUCCESS)
			{
				if (sent == writeBytes)
				{
					//全部发送成功
					return BFX_RESULT_SUCCESS;
				}
				else
				{
					//部分发送成功
					TcpTunnelSendBufferUseData(&(tunnel->sendBuffer), sent);
					return BFX_RESULT_SUCCESS;
				}
			}
		}
		else 
		{
			BLOG_INFO("tcp tunnel drop package,send buffer is not empty");
			//否则把数据放入sendbuffer => package queue是不是放在container层更好?
			//Bdt_TcpTunnelSendBufferAddPackages(&(tunnel->sendBuffer), packages);
		}
	}
	else if (state == BDT_TCP_TUNNEL_STATE_UNKNOWN)
	{
		//创建tcpInterface并发送firstPackage。注意处理onEstablish事件是才会真正发送。
		TcpTunnelConnect(
			tunnel, 
			BdtTunnel_GetNextSeq(tunnel->owner), 
			packages, 
			&packageCount, 
			&state);
	}
	return BFX_RESULT_SUCCESS;
}


void Bdt_TcpTunnelSendBufferInit(TcpTunnelSendBuffer* sendBuffer) {
	sendBuffer->boxBufferLength = 0;
	sendBuffer->boxBufferUsed = 0;
	//srand((unsigned int)time(NULL));
}


static uint32_t TcpTunnelOnTcpPackage(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package** packages)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_TunnelContainer* container = TcpTunnelFromInterfaceOwner(owner)->owner;
	for (size_t ix = 0; ix < BDT_PROTOCOL_MIN_PACKAGE_LENGTH; ++ix)
	{
		const Bdt_Package* package = packages[ix];
		if (!package)
		{
			break;
		}
		bool handled = false;
		result = BdtTunnel_PushSessionPackage(container, package, &handled);
	}
	return result;
}


static void TcpTunnelStartActivitor(Bdt_TcpTunnel* tunnel)
{
	//todo: need heartbeat?
}

static void TcpTunnelStopActivitor(Bdt_TcpTunnel* tunnel)
{
	//todo: need heartbeat?
}

#endif //__BDT_TCP_TUNNEL_IMP_INL__