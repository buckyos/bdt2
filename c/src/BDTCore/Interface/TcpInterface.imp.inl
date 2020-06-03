#ifndef __BDT_TCP_INTERFACE_IMP_INL__
#define __BDT_TCP_INTERFACE_IMP_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__
#include "./TcpInterface.inl"
#include <mbedtls/aes.h>
#include <assert.h>
#include "./NetManager.inl"
#include "../BdtSystemFramework.h"
#include "../Stack.h"

#include "../Protocol/PackageDecoder.h"
#include "../Protocol/PackageEncoder.h"

#include "../Tunnel/TcpTunnel.h"
#include "../Connection/ConnectionModule.h"
#include "../SnClient/CallSession.h"


bool Bdt_IsTcpEndpointPassive(const BdtEndpoint* local, const BdtEndpoint* remote)
{
	return !!local->port;
}

void Bdt_MarkTcpEndpointPassive(BdtEndpoint* local, BdtEndpoint* remote, bool passive)
{
	local->port = passive ? local->port : 0;
	remote->port = passive ? 0 : remote->port;
}

static const BdtEndpoint* TcpListenerGetLocal(const TcpListener* tl)
{
	return &tl->localEndpoint;
}

static TcpListener* TcpListenerCreate(BdtSystemFramework* framework, const BdtEndpoint* localEndpoint)
{
	TcpListener* tcpListener = BFX_MALLOC_OBJ(TcpListener);
	memset(tcpListener, 0, sizeof(TcpListener));
	tcpListener->framework = framework;
	BfxUserData owner = { .userData = tcpListener,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BDT_SYSTEM_TCP_SOCKET socket = BdtCreateTcpSocket(framework, &owner);
	tcpListener->socket = socket;
	tcpListener->localEndpoint = *localEndpoint;
	return tcpListener;
}

static uint32_t TcpListenerListen(TcpListener* listener)
{
	return BdtTcpSocketListen(listener->framework, listener->socket, &listener->localEndpoint);
}

static void BFX_STDCALL OnTcpWaitFirstPackageOvertime(BDT_SYSTEM_TIMER timer, void* owner)
{
	// tofix: timer triggers in same thread as socket's event ?
	Bdt_TcpInterface* ti = (Bdt_TcpInterface*)owner;
	assert(ti->waitFirstPackageTimer = timer);
	ti->waitFirstPackageTimer = NULL;
	if (ti->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE)
	{
		BLOG_DEBUG("%s wait first package over time", Bdt_TcpInterfaceGetName(ti));
		Bdt_TcpInterfaceReset(ti);
	}
}

static void BFX_STDCALL TcpInterfaceAsUserDataAddRef(void* owner)
{
	Bdt_TcpInterfaceAddRef((Bdt_TcpInterface*)owner);
}

static void BFX_STDCALL TcpInterfaceAsUserDataRelease(void* owner)
{
	Bdt_TcpInterfaceRelease((Bdt_TcpInterface*)owner);
}

static BDT_SYSTEM_TCP_SOCKET TcpInterfaceAddRefSocket(Bdt_TcpInterface* ti)
{
	BfxAtomicInc32(&ti->socketRefCount);
	return ti->socket;
}

static void TcpInterfaceReleaseSocket(Bdt_TcpInterface* ti)
{
	if (BfxAtomicDec32(&ti->socketRefCount) <= 0)
	{
		BdtDestroyTcpSocket(ti->framework, ti->socket);
		ti->socket = NULL;
	}
}

static void TcpInterfaceAsRefUserData(Bdt_TcpInterface* ti, BfxUserData* outUserData)
{
	outUserData->userData = ti;
	outUserData->lpfnAddRefUserData = TcpInterfaceAsUserDataAddRef;
	outUserData->lpfnReleaseUserData = TcpInterfaceAsUserDataRelease;
}


static Bdt_TcpInterface* TcpInterfaceCreate(
	BdtSystemFramework* framework,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	const BdtNetTcpConfig* config,
	BDT_SYSTEM_TCP_SOCKET passiveSocket, 
	int32_t localId
)
{
	Bdt_TcpInterface* tcpInterface = BFX_MALLOC_OBJ(Bdt_TcpInterface);
	memset(tcpInterface, 0, sizeof(Bdt_TcpInterface));


	tcpInterface->config = config;
	tcpInterface->refCount = 1;
	tcpInterface->firstBlockSize = TCP_FIRST_PACKAGE_SIZE;
	tcpInterface->framework = framework;

	tcpInterface->localEndpoint = *localEndpoint;
	tcpInterface->localEndpoint.flag |= BDT_ENDPOINT_PROTOCOL_TCP;
	tcpInterface->remoteEndpoint = *remoteEndpoint;
	tcpInterface->remoteEndpoint.flag |= BDT_ENDPOINT_PROTOCOL_TCP;

	BfxRwLockInit(&tcpInterface->stateLock);
	tcpInterface->owner = NULL;

	if (passiveSocket)
	{
		tcpInterface->socket = passiveSocket;
		tcpInterface->remoteEndpoint.port = 0;
		BfxUserData socketUserData;
		TcpInterfaceAsRefUserData(tcpInterface, &socketUserData);
		BdtTcpSocketInitUserData(tcpInterface->framework, passiveSocket, &socketUserData);

		BfxUserData timerUserData;
		TcpInterfaceAsRefUserData(tcpInterface, &timerUserData);
		tcpInterface->waitFirstPackageTimer = BdtCreateTimeout(tcpInterface->framework, OnTcpWaitFirstPackageOvertime, &timerUserData, tcpInterface->config->firstPackageWaitTime);
		tcpInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE;
		BdtTcpSocketResume(tcpInterface->framework, passiveSocket);
	} 
	else
	{
		tcpInterface->localEndpoint.port = 0;
		BfxUserData socketUserData;
		TcpInterfaceAsRefUserData(tcpInterface, &socketUserData);
		BDT_SYSTEM_TCP_SOCKET socket = BdtCreateTcpSocket(tcpInterface->framework, &socketUserData);
		tcpInterface->socket = socket;
		tcpInterface->state = BDT_TCP_INTERFACE_STATE_NONE;
	}
	tcpInterface->socketRefCount = 1;

	tcpInterface->cache = tcpInterface->cacheData;
	tcpInterface->backCache = tcpInterface->backCacheData;


	const char* prename = "TcpInterface";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen 
		+ 1/*sperator -*/ + 13/*local id*/ + 1/*sperator*/ 
		+ BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH 
		+ 1/*0*/;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(localEndpoint, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remoteEndpoint, remotestr);
	sprintf(name, "%s-%i-%s%s%s", prename, localId, localstr, epsplit, remotestr);
	tcpInterface->name = name;

	return tcpInterface;
}



int32_t Bdt_TcpInterfaceAddRef(const Bdt_TcpInterface* ti)
{
	Bdt_TcpInterface* i = (Bdt_TcpInterface*)ti;
	return BfxAtomicInc32(&i->refCount);
}

int32_t Bdt_TcpInterfaceRelease(const Bdt_TcpInterface* ti)
{
	Bdt_TcpInterface* i = (Bdt_TcpInterface*)ti;
	int32_t ref = BfxAtomicDec32(&i->refCount);
	if (ref <= 0)
	{
		//tofix: release all members
		BfxFree(i->name);
		BfxRwLockDestroy(&i->stateLock);
		BfxFree(i);
	}

	return ref;
}

const BdtEndpoint* Bdt_TcpInterfaceGetLocal(const Bdt_TcpInterface* ti)
{
	return &ti->localEndpoint;
}

const BdtEndpoint* Bdt_TcpInterfaceGetRemote(const Bdt_TcpInterface* ti)
{
	return &ti->remoteEndpoint;
}

const char* Bdt_TcpInterfaceGetName(const Bdt_TcpInterface* ti)
{
	return ti->name;
}


uint32_t Bdt_TcpInterfaceSetOwner(Bdt_TcpInterface* ti, Bdt_TcpInterfaceOwner* owner)
{
	Bdt_TcpInterfaceOwner* old = NULL;
	uint32_t result = BFX_RESULT_SUCCESS;

	BfxRwLockWLock(&ti->stateLock);
	do
	{
		if (ti->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		old = ti->owner;
		ti->owner = owner;
		if (owner)
		{
			owner->addRef(owner);
		}
	} while (false);
	BfxRwLockWUnlock(&ti->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}
	if (old)
	{
		old->release(old);
	}
	return BFX_RESULT_SUCCESS;
}

static Bdt_TcpInterfaceOwner* TcpInterfaceGetOwner(Bdt_TcpInterface* ti)
{
	Bdt_TcpInterfaceOwner* owner = NULL;

	BfxRwLockRLock(&ti->stateLock);
	do
	{
		owner = ti->owner;
		if (owner)
		{
			owner->addRef(owner);
		}
		
	} while (false);
	BfxRwLockRUnlock(&ti->stateLock);

	return owner;
}


Bdt_TcpInterfaceState Bdt_TcpInterfaceGetState(const Bdt_TcpInterface* ti)
{
	return ti->state;
}

Bdt_TcpInterfaceState Bdt_TcpInterfaceSetState(Bdt_TcpInterface* ti, Bdt_TcpInterfaceState oldState, Bdt_TcpInterfaceState newState)
{
	Bdt_TcpInterfaceState fromState = BDT_TCP_INTERFACE_STATE_NONE;
	BfxRwLockWLock(&ti->stateLock);
	do 
	{
		if ((fromState = ti->state) != oldState)
		{
			break;
		}
		ti->state = newState;
		TcpInterfaceAddRefSocket(ti);
	} while (false);
	BfxRwLockWUnlock(&ti->stateLock);
	
	if (oldState != fromState)
	{
		BLOG_DEBUG("%s ignore set state from %u to %u for invalid old state %s", ti->name, oldState, newState, fromState);
		return fromState;
	}

	BLOG_DEBUG("%s set state from %u to %u ", ti->name, oldState, newState);
	if (newState == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP)
	{
		BdtTcpSocketResume(ti->framework, ti->socket);
	}
	TcpInterfaceReleaseSocket(ti);
	return fromState;
}

void Bdt_TcpInterfaceSetAesKey(Bdt_TcpInterface* ti, const uint8_t aesKey[BDT_AES_KEY_LENGTH])
{
	{
		uint8_t mixHash[BDT_MIX_HASH_LENGTH];
		BdtHistory_KeyManagerCalcKeyHash(aesKey, mixHash);
		BLOG_DEBUG("%s set key:%02x%02x%02x%02x%02x%02x%02x%02x",
			ti->name,
			mixHash[0],
			mixHash[1],
			mixHash[2],
			mixHash[3],
			mixHash[4],
			mixHash[5],
			mixHash[6],
			mixHash[7]);
	}
	
	memcpy(ti->aesKey, aesKey, BDT_AES_KEY_LENGTH);
	ti->isCrypto = true;
}

uint32_t Bdt_TcpInterfaceClose(Bdt_TcpInterface* ti)
{
	BLOG_DEBUG("close %s", ti->name);

	Bdt_TcpInterfaceOwner* owner = NULL;
	Bdt_TcpInterfaceState oldState = BDT_TCP_INTERFACE_STATE_NONE;
	BfxRwLockWLock(&ti->stateLock);
	do {
		oldState = ti->state;
		if (ti->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			break;
		}
		TcpInterfaceAddRefSocket(ti);
		if (ti->state == BDT_TCP_INTERFACE_STATE_STREAM)
		{
			ti->state = BDT_TCP_INTERFACE_STATE_STREAM_CLOSING;
			break;
		}
		ti->state = BDT_TCP_INTERFACE_STATE_CLOSED;
		owner = ti->owner;
		ti->owner = NULL;
	} while (false);
	BfxRwLockWUnlock(&ti->stateLock);

	if (oldState == BDT_TCP_INTERFACE_STATE_CLOSED)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	BdtTcpSocketClose(ti->framework, ti->socket, false);

	if (oldState != BDT_TCP_INTERFACE_STATE_STREAM)
	{
		TcpInterfaceReleaseSocket(ti);
	}
	TcpInterfaceReleaseSocket(ti);

	if (owner && owner->release)
	{
		owner->release(owner);
	}

	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_TcpInterfaceReset(Bdt_TcpInterface* ti)
{
	BLOG_DEBUG("reset %s", ti->name);

	Bdt_TcpInterfaceOwner* owner = NULL;
	Bdt_TcpInterfaceState oldState = BDT_TCP_INTERFACE_STATE_NONE;
	BfxRwLockWLock(&ti->stateLock);
	do {
		oldState = ti->state;
		if (ti->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			break;
		}
		ti->state = BDT_TCP_INTERFACE_STATE_CLOSED;
		owner = ti->owner;
		ti->owner = NULL;
	} while (false);
	BfxRwLockWUnlock(&ti->stateLock);

	if (oldState == BDT_TCP_INTERFACE_STATE_CLOSED)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	if (owner && owner->release)
	{
		owner->release(owner);
	}

	BdtTcpSocketClose(ti->framework, ti->socket, true);
	TcpInterfaceReleaseSocket(ti);

	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_TcpInterfaceConnect(Bdt_TcpInterface* ti)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockWLock(&ti->stateLock);
	do
	{
		if (BDT_TCP_INTERFACE_STATE_NONE != ti->state)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		ti->state = BDT_TCP_INTERFACE_STATE_CONNECTING;
		TcpInterfaceAddRefSocket(ti);
	} while (false);
	BfxRwLockWUnlock(&ti->stateLock);
	
	if (result != BFX_RESULT_SUCCESS)
	{
		BLOG_DEBUG("%s ignore connect for %u", ti->name, result);
		return result;
	}
	BLOG_DEBUG("%s connect", ti->name);
	result = BdtTcpSocketConnect(
		ti->framework,
		ti->socket,
		&ti->localEndpoint,
		&ti->remoteEndpoint);
	TcpInterfaceReleaseSocket(ti);
	return result;
}

uint32_t Bdt_TcpInterfaceSend(Bdt_TcpInterface* tcpInterface, uint8_t* data, size_t len, size_t* outSent)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockRLock(&tcpInterface->stateLock);
	do
	{
		if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		TcpInterfaceAddRefSocket(tcpInterface);
	} while (false);
	BfxRwLockRUnlock(&tcpInterface->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

    BLOG_DEBUG("%s send, len:%d", tcpInterface->name, (int)len);
    result = BdtTcpSocketSend(tcpInterface->framework, tcpInterface->socket, data, &len);
	*outSent = len;
	TcpInterfaceReleaseSocket(tcpInterface);
	return result;
}

uint32_t Bdt_TcpInterfacePause(Bdt_TcpInterface* tcpInterface)
{
	BLOG_DEBUG("%s pause", tcpInterface->name);
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockRLock(&tcpInterface->stateLock);
	do
	{
		if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		TcpInterfaceAddRefSocket(tcpInterface);
	} while (false);
	BfxRwLockRUnlock(&tcpInterface->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

	result = BdtTcpSocketPause(tcpInterface->framework, tcpInterface->socket);
	TcpInterfaceReleaseSocket(tcpInterface);
	return result;
}

uint32_t Bdt_TcpInterfaceResume(Bdt_TcpInterface* tcpInterface)
{
	BLOG_DEBUG("%s resume", tcpInterface->name);
	uint32_t result = BFX_RESULT_SUCCESS;
	BfxRwLockRLock(&tcpInterface->stateLock);
	do
	{
		if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_CLOSED)
		{
			result = BFX_RESULT_INVALID_STATE;
			break;
		}
		TcpInterfaceAddRefSocket(tcpInterface);
	} while (false);
	BfxRwLockRUnlock(&tcpInterface->stateLock);

	if (result != BFX_RESULT_SUCCESS)
	{
		return result;
	}

	result = BdtTcpSocketResume(tcpInterface->framework, tcpInterface->socket);
	TcpInterfaceReleaseSocket(tcpInterface);
	return result;
}

static uint16_t _tcpInterfaceDecryptoBoxSize(Bdt_TcpInterface* tcpInterface, uint16_t boxSize)
{
	if (tcpInterface->isCrypto)
	{
		//TODO:测试阶段先不加密box header.方便调试
		return boxSize;
	}

	return boxSize;
}


static uint16_t _tcpInterfaceCryptoBoxSize(Bdt_TcpInterface* tcpInterface, uint16_t boxSize)
{
	//TODO:测试阶段先不加密box header.方便调试
	return boxSize;
}


uint32_t Bdt_TcpInterfaceBoxFirstPackage(
	Bdt_TcpInterface* tcpInterface,
	const Bdt_Package** packages,
	size_t packageCount,
	uint8_t* buffer,
	size_t bufferLen,
	size_t* pWriteBytes,
	uint8_t remotePublicType,
	const uint8_t* remotePublic,
	uint16_t secretLength,
	const uint8_t* secret)
{
	int r = 0;
	size_t wirteBytes = 0;
	size_t used = 0;
	if (packageCount == 0)
	{
		return BFX_RESULT_SUCCESS;
	}

	if (tcpInterface->isCrypto)
	{
		if (packages[0]->cmdtype == BDT_EXCHANGEKEY_PACKAGE)
		{
			//1)write aeskey
			size_t olen = 0;
			used = 0;
			int ret = Bdt_RsaEncrypt(
				tcpInterface->aesKey,
				BDT_AES_KEY_LENGTH,
				remotePublic,
				BdtGetPublicKeyLength(remotePublicType),
				buffer,
				&olen,
				bufferLen);
			if (ret)
			{
				return ret;
			}
			used += olen;
			//2) write Key hash
			Bdt_HashAesKey(tcpInterface->aesKey, buffer + used);
			used += 8;
			//3) write boxsize after encode.
			uint16_t* pBoxHeader = (uint16_t*)(buffer + used);
			used += 2;

			//4) 准备seqAndKeySign 
			Bdt_ExchangeKeyPackage* pkg = (Bdt_ExchangeKeyPackage*)packages[0];
			uint8_t buf4sign[sizeof(pkg->seq) + BDT_AES_KEY_LENGTH];
			memcpy(buf4sign, (const uint8_t*)(&(pkg->seq)), sizeof(pkg->seq));
			memcpy(buf4sign + sizeof(pkg->seq), tcpInterface->aesKey, BDT_AES_KEY_LENGTH);

			uint8_t sign[BDT_MAX_SIGN_LENGTH];
			size_t signlen = BDT_MAX_SIGN_LENGTH;
			ret = Bdt_RsaSignMd5(buf4sign, sizeof(buf4sign), secret, secretLength, sign, &signlen);
			if (ret != BFX_RESULT_SUCCESS)
			{
				return ret;
			}

			memcpy(pkg->seqAndKeySign, sign, sizeof(pkg->seqAndKeySign));

			size_t outLen = 0;
			size_t remainLen = bufferLen - used;
			int result = Bdt_EncodePackage(
				packages, 
				packageCount, 
				NULL, 
				buffer + used, 
				remainLen, 
				&outLen);
			if (result != BFX_RESULT_SUCCESS)
			{
				return result;
			}

			int targetLen = Bdt_AesEncryptDataWithPadding(tcpInterface->aesKey, buffer + used, outLen, remainLen, buffer + used, remainLen);
			if (targetLen < 0)
			{
				return BFX_RESULT_FAILED;
			}
			if (targetLen > 0xffff)
			{
				return BFX_RESULT_OVERFLOW;
			}
			used += targetLen;
			*pBoxHeader = _tcpInterfaceCryptoBoxSize(tcpInterface, targetLen);
			*pWriteBytes = used;
			return BFX_RESULT_SUCCESS;
		}
		else
		{
			//1 write mix hash
			uint8_t sha[32];
			Bdt_Sha256Hash(tcpInterface->aesKey, sizeof(tcpInterface->aesKey), sha);
			memcpy(buffer, sha, BDT_MIX_HASH_LENGTH);
			used += BDT_MIX_HASH_LENGTH;
			//2 write boxsize
			uint16_t* pBoxHeader = (uint16_t*)(buffer + used);
			used += 2;

			//3 write data			
			size_t outLen = 0;
			size_t remainLen = bufferLen - used;
			int result = Bdt_EncodePackage(
				packages,
				packageCount,
				NULL,
				buffer + used,
				remainLen,
				&outLen);
			if (result != BFX_RESULT_SUCCESS)
			{
				return result;
			}

			int targetLen = Bdt_AesEncryptDataWithPadding(tcpInterface->aesKey, buffer + used, outLen, remainLen, buffer + used, remainLen);
			if (targetLen < 0)
			{
				return BFX_RESULT_FAILED;
			}
			if (targetLen > 0xffff)
			{
				return BFX_RESULT_OVERFLOW;
			}
			used += targetLen;
			*pBoxHeader = _tcpInterfaceCryptoBoxSize(tcpInterface, targetLen);
			*pWriteBytes = used;

			return BFX_RESULT_SUCCESS;

		}
	}
	else
	{
		buffer[0] = BDT_PROTOCOL_UDP_MAGIC_NUM_0;
		buffer[1] = BDT_PROTOCOL_UDP_MAGIC_NUM_1;
		r = Bdt_EncodePackage(
			packages, 
			packageCount, 
			NULL, 
			buffer + 4, 
			bufferLen - 2, 
			&wirteBytes);
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		assert(wirteBytes <= 0xffff);
		if (r > 0xffff)
		{
			return BFX_RESULT_OVERFLOW;
		}
		uint16_t* pBoxHeader = (uint16_t*)(buffer + 2);
		*pBoxHeader = (uint16_t)wirteBytes;
		*pWriteBytes = 2 + 2 + wirteBytes;
		return r;
	}
}

uint32_t Bdt_TcpInterfaceBoxPackage(
	Bdt_TcpInterface* tcpInterface, 
	const Bdt_Package** packages, 
	size_t packageCount, 
	uint8_t* buffer, 
	size_t bufferLen, 
	size_t* pWriteBytes)
{
	size_t wirteBytes = 0;
	uint16_t* pBoxHeader = (uint16_t*)(buffer);
	int r = Bdt_EncodePackage(
		packages, 
		packageCount, 
		NULL, 
		buffer + 2, 
		bufferLen - 2, 
		&wirteBytes);
	if (r == BFX_RESULT_SUCCESS)
	{
		if (tcpInterface->isCrypto)
		{
			size_t boxsize = Bdt_AesEncryptDataWithPadding(tcpInterface->aesKey, buffer + 2, wirteBytes, bufferLen - 2, buffer + 2, bufferLen - 2);
			assert(boxsize <= 0xffff);
			if (boxsize > 0xffff)
			{
				return BFX_RESULT_OVERFLOW;
			}
			*pBoxHeader = _tcpInterfaceCryptoBoxSize(tcpInterface, (uint16_t)boxsize);
			*pWriteBytes = boxsize + 2;
		}
		else
		{
			*pBoxHeader = (uint16_t)wirteBytes;
		}
	}

	return r;
}


#define TCPINTERFACE_DATA_BOX_HEADER_SIZE			2
#define TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE		16
uint32_t Bdt_TcpInterfaceBeginCryptoData(
	Bdt_TcpInterface* tcpInterface,
	uint8_t* buffer, 
	size_t bufferLen, 
	Bdt_TcpInterfaceCryptoDataContext* context
)
{
	assert(tcpInterface->isCrypto);
	assert(tcpInterface->state == BDT_TCP_INTERFACE_STATE_STREAM);

	memset(context, 0, sizeof(Bdt_TcpInterfaceCryptoDataContext));
	mbedtls_aes_init((mbedtls_aes_context*)context->aesContext);
	mbedtls_aes_setkey_enc((mbedtls_aes_context*)context->aesContext, tcpInterface->aesKey, BDT_AES_KEY_LENGTH * 8);
	context->buffer = buffer;
	context->bufferLen = bufferLen;
	context->data = buffer + TCPINTERFACE_DATA_BOX_HEADER_SIZE;
	context->dataOffset = 0;

	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_TcpInterfaceAppendCryptoData(
	Bdt_TcpInterface* tcpInterface,
	Bdt_TcpInterfaceCryptoDataContext* context,
	const uint8_t* data,
	size_t dataLen
)
{
	if (!dataLen)
	{
		return BFX_RESULT_SUCCESS;
	}
	size_t thisBlock = context->dataOffset / TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE * TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE;
	size_t appendOffset = context->dataOffset + dataLen;
	size_t dataRemain = dataLen;
	if (context->dataOffset != thisBlock)
	{
		size_t nextBlock = thisBlock + TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE;
		size_t toCopy = nextBlock - context->dataOffset;
		toCopy = toCopy > dataLen ? dataLen : toCopy;
		memcpy(context->data + context->dataOffset, data, toCopy);
		context->dataOffset += toCopy;
		data += toCopy;
		dataRemain -= toCopy;
		if (context->dataOffset == nextBlock)
		{
			int ret = mbedtls_aes_crypt_cbc(
				(mbedtls_aes_context*)context->aesContext,
				MBEDTLS_AES_ENCRYPT, 
				TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE,
				context->iv,
				context->data + context->dataOffset,
				context->data + context->dataOffset);
		}
	}
	
	if (!dataRemain)
	{
		return BFX_RESULT_SUCCESS;
	}
	size_t appendBlock = dataRemain / TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE * TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE;
	if (appendBlock)
	{
		int ret = mbedtls_aes_crypt_cbc(
			(mbedtls_aes_context*)context->aesContext,
			MBEDTLS_AES_ENCRYPT,
			appendBlock,
			context->iv,
			data,
			context->data + thisBlock);
		
		context->dataOffset += appendBlock;
		data += appendBlock;
		dataRemain -= appendBlock;
	}

	if (!dataRemain)
	{
		return BFX_RESULT_SUCCESS;
	}
	memcpy(context->data + context->dataOffset, data, dataRemain);
	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_TcpInterfaceFinishCryptoData(
	Bdt_TcpInterface* tcpInterface,
	Bdt_TcpInterfaceCryptoDataContext* context
)
{
	size_t nextBlock = (context->dataOffset / TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE + 1) * TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE;
	uint8_t padding = (uint8_t)(nextBlock - context->dataOffset);
	for (uint8_t i = 0; i < padding; i++)
	{
		context->data[context->dataOffset + i] = padding;
	}

	int ret = mbedtls_aes_crypt_cbc(
		(mbedtls_aes_context*)context->aesContext,
		MBEDTLS_AES_ENCRYPT,
		TCPINTERFACE_DATA_CRYPTO_AES_BLOCK_SIZE,
		context->iv,
		context->data + context->dataOffset,
		context->data + context->dataOffset);
	context->dataOffset = nextBlock;
	*(uint16_t*)context->buffer = _tcpInterfaceCryptoBoxSize(tcpInterface, (uint16_t)context->dataOffset);
	context->bufferLen = context->dataOffset + 2;
	mbedtls_aes_free((mbedtls_aes_context*)context->aesContext);
	return BFX_RESULT_SUCCESS;
}

// 注意:这个操作是InPlace Encode，所以buffer前面要留有至少2字节的长度空间。使用时要特别小心 
uint32_t Bdt_TcpInterfaceBoxCryptoData(
	Bdt_TcpInterface* tcpInterface, 
	uint8_t* buffer, 
	size_t dataLen, 
	size_t bufferLen, 
	size_t* boxsize, 
	size_t* boxHeaderLen)
{
	assert(tcpInterface->isCrypto);
	assert(tcpInterface->state != BDT_TCP_INTERFACE_STATE_PACKAGE &&
        tcpInterface->state != BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE &&
        tcpInterface->state != BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);

	int len = Bdt_AesEncryptDataWithPadding(tcpInterface->aesKey, buffer, dataLen, bufferLen, buffer, bufferLen);
	size_t boxlen = _tcpInterfaceCryptoBoxSize(tcpInterface, len);
	uint16_t* pBoxHeader = (uint16_t*)(buffer - 2);
	assert(boxlen <= 0xffff);
	if (boxlen > 0xffff)
	{
		return BFX_RESULT_OVERFLOW;
	}
	*pBoxHeader = (uint16_t)boxlen;
	*boxHeaderLen = 2;
	if (boxsize)
	{
		*boxsize = boxlen;
	}

	return BFX_RESULT_SUCCESS;
}

BDT_API(void) BdtPushTcpSocketEvent(
	BdtStack* stack, 
	BDT_SYSTEM_TCP_SOCKET socket, 
	uint32_t eventId, 
	void* eventParam, 
	const void* userData)
{
	uint32_t result = BFX_RESULT_SUCCESS;
	switch (eventId)
	{
	case BDT_TCP_SOCKET_EVENT_CONNECTION:
	{
		// accept socket event，也许以后应该换个专门的函数来push
		const BdtTcpSocketConnectionEvent* connectionParams = (const BdtTcpSocketConnectionEvent*)eventParam;
		// accept new tcp connection. 交给NetManager处理，默认情况下会接受 
        const TcpListener* tcpListener = (const TcpListener*)userData;
		assert(tcpListener);

		Bdt_TcpInterface* tcpInterface = NetManagerAcceptTcpSocket(
			BdtStackGetNetManager(stack),
			connectionParams->socket,
			&(tcpListener->localEndpoint),
			connectionParams->remoteEndpoint);
		BLOG_INFO("accept tcp interface %s", tcpInterface->name);
		Bdt_TcpInterfaceRelease(tcpInterface);

		break;
	}
	case BDT_TCP_SOCKET_EVENT_ESTABLISH:
	{
		Bdt_TcpInterface* tcpInterface = (Bdt_TcpInterface*)userData;
		if (BDT_TCP_INTERFACE_STATE_CONNECTING != Bdt_TcpInterfaceSetState(tcpInterface, BDT_TCP_INTERFACE_STATE_CONNECTING, BDT_TCP_INTERFACE_STATE_ESTABLISH))
		{
			//todo: reset it ?

		}
		else
		{
			BLOG_INFO("%s established", tcpInterface->name);
			Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
			if (owner)
			{
				if (owner->onEstablish)
				{
					result = owner->onEstablish(owner, tcpInterface);
				}
				else
				{
					result = BFX_RESULT_NOT_IMPL;
				}
				owner->release(owner);
			}
			else
			{
				result = BFX_RESULT_NOT_FOUND;
			}
		}
		break;
	}
	case BDT_TCP_SOCKET_EVENT_DRAIN:
	{
		Bdt_TcpInterface* tcpInterface = (Bdt_TcpInterface*)userData;
		BLOG_DEBUG("%s drain", tcpInterface->name);
		Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
		if (owner)
		{
			if (owner->onDrain)
			{
				result = owner->onDrain(owner, tcpInterface);
			}
			else
			{
				result = BFX_RESULT_NOT_IMPL;
			}
			owner->release(owner);
		}
		else
		{
			result = BFX_RESULT_NOT_FOUND;
		}
		break;
	}

	case BDT_TCP_SOCKET_EVENT_ERROR:
	{
		Bdt_TcpInterface* tcpInterface = (Bdt_TcpInterface*)userData;
		uint32_t error = BFX_GET_RESULTCODE(*(uint32_t*)eventParam);
		BLOG_DEBUG("%s got error %u", tcpInterface->name, error);
		Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
		if (owner)
		{
			if (owner->onError)
			{
				owner->onError(owner, tcpInterface, error);
			}
			owner->release(owner);
		}
		break;
	}
	default:
	{
		result = BFX_RESULT_INVALID_PARAM;
	}
	}

	if (result != BFX_RESULT_SUCCESS)
	{
		// tofix: reset and fire last event
	}
}


typedef struct TcpInterfaceFirstParams
{
	BdtPeerid remotePeerid;
	bool isStartWithExchangeKey;
}TcpInterfaceFirstParams;

typedef struct TcpInterfaceBox
{
	uint8_t* data;
	size_t bytes;
} TcpInterfaceBox;

static bool TcpInterfaceIsValidFirstPackage(uint8_t cmdType)
{
	return true;
}

static void TcpInterfaceSwapCache(Bdt_TcpInterface* tcpInterface)
{
	uint8_t* pTemp = tcpInterface->cache;
	tcpInterface->cache = tcpInterface->backCache;
	tcpInterface->backCache = pTemp;
}

static void TcpInterfaceAppendCache(Bdt_TcpInterface* tcpInterface, const uint8_t* data, size_t bytes)
{
	assert(tcpInterface->cacheLength + bytes < TCP_INTERFACE_CACHE_SIZE);
	memcpy(tcpInterface->cache + tcpInterface->cacheLength, data, bytes);
	tcpInterface->cacheLength += bytes;
}



static uint32_t TcpInterfaceOnData(BdtStack* stack, Bdt_TcpInterface* tcpInterface, const uint8_t* data, size_t bytes)
{
	// for release tcp interface in owner's callback
	Bdt_TcpInterfaceAddRef(tcpInterface);

	//BLOG_DEBUG("%s got data length %d", tcpInterface->name, (int)bytes);
	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
	if (owner)
	{
		if (owner->onData)
		{
			result = owner->onData(owner, tcpInterface, data, bytes);
		}
		else
		{
			result = BFX_RESULT_NOT_IMPL;
		}
		owner->release(owner);
	}
	else
	{
		result = BFX_RESULT_NOT_FOUND;
	}

	Bdt_TcpInterfaceRelease(tcpInterface);
	return result;
}

static uint32_t TcpInterfaceOnPackages(BdtStack* stack, Bdt_TcpInterface* tcpInterface, Bdt_Package** packages)
{
	// for release tcp interface in owner's callback
	Bdt_TcpInterfaceAddRef(tcpInterface);

	BLOG_DEBUG("%s got packages", tcpInterface->name);
	uint32_t result = BFX_RESULT_SUCCESS;
	Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
	if (owner)
	{
		if (owner->onPackage)
		{
			result = owner->onPackage(owner, tcpInterface, packages);
		}
		else
		{
			result = BFX_RESULT_NOT_IMPL;
		}
		owner->release(owner);
	}
	else
	{
		result = BFX_RESULT_NOT_FOUND;
	}

	Bdt_TcpInterfaceRelease(tcpInterface);
	return result;
}

static uint32_t TcpInterfaceOnFirstPackages(
	BdtStack* stack, 
	Bdt_TcpInterface* tcpInterface, 
	Bdt_Package** packages, 
	const BdtPeerid* keyPeerid)
{
	// for release tcp interface in owner's callback
	Bdt_TcpInterfaceAddRef(tcpInterface);

	BLOG_DEBUG("%s got first packages", tcpInterface->name);
	const Bdt_Package* firstPackage = NULL;
	if (packages[0]->cmdtype == BDT_EXCHANGEKEY_PACKAGE)
	{
		Bdt_ExchangeKeyPackage* exchange = (Bdt_ExchangeKeyPackage*)packages[0];
		firstPackage = packages[1];
		assert(!packages[2]);
		BdtHistory_KeyManagerAddKey(BdtStackGetKeyManager(stack), tcpInterface->aesKey, &exchange->fromPeerid, true);
	}
	else
	{
		firstPackage = packages[0];
		assert(!packages[1]);
	}
	uint32_t result = BFX_RESULT_FAILED;
	assert(firstPackage);

	Bdt_TunnelContainer* container = NULL;
	container = BdtTunnel_GetContainerByRemotePeerid(
		BdtStackGetTunnelManager(stack),
		keyPeerid);
	if (!container)
	{
		if (firstPackage->cmdtype == BDT_SYN_TUNNEL_PACKAGE
			|| firstPackage->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE)
		{
			container = BdtTunnel_CreateContainer(
				BdtStackGetTunnelManager(stack),
				keyPeerid);
		}
	}
	if (!container)
	{
		result = BFX_RESULT_INVALID_PARAM;
	}
	else
	{
		result = BdtTunnel_OnTcpFirstPackage(container, tcpInterface, firstPackage);
		Bdt_TunnelContainerRelease(container);
	}

	Bdt_TcpInterfaceRelease(tcpInterface);
	return result;
}


static uint32_t TcpInterfaceOnFirstResp(BdtStack* stack, Bdt_TcpInterface* tcpInterface, Bdt_Package** packages)
{
	// for release tcp interface in owner's callback
	Bdt_TcpInterfaceAddRef(tcpInterface);

	BLOG_DEBUG("%s got first responses", tcpInterface->name);
	const Bdt_Package* firstPackage = packages[0];
	assert(!packages[1]);
	uint32_t result = BFX_RESULT_SUCCESS;
	assert(firstPackage);
	Bdt_TcpInterfaceOwner* owner = TcpInterfaceGetOwner(tcpInterface);
	if (owner)
	{
		if (owner->onFirstResp)
		{
			result = owner->onFirstResp(owner, tcpInterface, firstPackage);
		}
		else
		{
			result = BFX_RESULT_NOT_IMPL;
		}
		owner->release(owner);
	}
	else
	{
		result = BFX_RESULT_NOT_FOUND;
	}

	Bdt_TcpInterfaceRelease(tcpInterface);
	return result;
}

static void TcpInterfaceTryFillCache(Bdt_TcpInterface* tcpInterface, size_t targetLength, const uint8_t* data, size_t datalen, size_t* pCopysize)
{
	size_t copysize = 0;
	if (datalen + tcpInterface->cacheLength >= targetLength)
	{
		copysize = targetLength - tcpInterface->cacheLength;
	}
	else
	{
		copysize = datalen;
	}
	TcpInterfaceAppendCache(tcpInterface, data, copysize);
	*pCopysize = copysize;
	return;
}

static int TcpInterfaceUnboxParserPushData(
	BdtStack* stack, 
	Bdt_TcpInterface* tcpInterface, 
	const uint8_t* data, 
	size_t bytes, 
	TcpInterfaceBox* pBoxes, 
	size_t* pBoxesLen, 
	size_t* pUsed, 
	TcpInterfaceFirstParams* pFirstFaram)
{
    assert(pBoxesLen != NULL);
    assert(*pBoxesLen > 0);
    assert(pUsed != NULL);
	if (bytes == 0)
	{
        *pBoxesLen = 0;
		return BFX_RESULT_SUCCESS;
	}

	int r = 0;
	const uint8_t* pData = NULL;
    size_t boxCount = 0;
	size_t dataLen = 0;
	size_t readed = 0;

	switch (tcpInterface->state)
	{

	case BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE:
		if (tcpInterface->unboxState == BDT_TCP_UNBOX_STATE_READING_LENGTH)
		{
			if (tcpInterface->cacheLength > 0)
			{
				size_t copysize = 0;
				TcpInterfaceTryFillCache(tcpInterface, tcpInterface->firstBlockSize, data, bytes, &copysize);

				readed = copysize;
				if (tcpInterface->cacheLength == tcpInterface->firstBlockSize)
				{
					pData = tcpInterface->cache;
				}
				else
				{
                    *pBoxesLen = boxCount;
					return BFX_RESULT_SUCCESS;
				}
			}
			else
			{
				if (bytes < tcpInterface->firstBlockSize)
				{
					TcpInterfaceAppendCache(tcpInterface, data, bytes);
                    *pBoxesLen = boxCount;
                    return BFX_RESULT_SUCCESS;
				}

				pData = data;
			}

			//start guess
			if (pData)
			{
				bool decodeOK = false;

				if (tcpInterface->firstBlockSize == TCP_FIRST_PACKAGE_SIZE)
				{
					if (pData[0] == BDT_PROTOCOL_UDP_MAGIC_NUM_0 && pData[1] == BDT_PROTOCOL_UDP_MAGIC_NUM_1)
					{
						BfxBufferStream bufferStream;
						BfxBufferStreamInit(&bufferStream, (uint8_t*)(pData + 2), TCP_FIRST_BLCOK_SIZE - 2);

						uint16_t firstLen = 0;
						BfxBufferReadUInt16(&bufferStream, &firstLen);
						if (firstLen < TCP_FIRST_PACKAGE_MAX_SIZE && firstLen > TCP_FIRST_PACKAGE_MIN_SIZE)
						{
							uint16_t cmdlen = 0;
							BfxBufferReadUInt16(&bufferStream, &cmdlen);
							uint8_t firstCmdType = 0;
							BfxBufferReadUInt8(&bufferStream, &firstCmdType);
							if (cmdlen <= firstLen && TcpInterfaceIsValidFirstPackage(firstCmdType))
							{
								decodeOK = true;//这是一个明文包
								tcpInterface->isCrypto = false;
								tcpInterface->boxSize = firstLen;

								tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_BODY;
								*pUsed += readed;
								if (pData == tcpInterface->cache)
								{
									//重新调整cache
									size_t cacheReaded = 2 + 2;
									memcpy(tcpInterface->cache, tcpInterface->cache + cacheReaded, tcpInterface->cacheLength - cacheReaded);
									tcpInterface->cacheLength -= cacheReaded;
									*pUsed += readed;
									return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
								}
								readed = 4;
								*pUsed += readed;
								return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);

							}
						}
					}
					//继续
					tcpInterface->firstBlockSize = TCP_FIRST_MIXHASH_SIZE;
					if (pData == tcpInterface->cache)
					{
						*pUsed += readed;
						return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
					}
					else
					{
						return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data, bytes, pBoxes, pBoxesLen, pUsed, pFirstFaram);
					}
				}
				else if (tcpInterface->firstBlockSize == TCP_FIRST_MIXHASH_SIZE)
				{
					//看看是不是aeskey
					if (BdtHistory_KeyManagerFindByMixHash(BdtStackGetKeyManager(stack), data, tcpInterface->aesKey, &pFirstFaram->remotePeerid, true, true) == BFX_RESULT_SUCCESS)
					{
						//aeskeyAES Unbox,先解一个TCP_CRYPTO_BOX 加密的2字节长度
						tcpInterface->isCrypto = true;
						BfxBufferStream bufferStream;
						BfxBufferStreamInit(&bufferStream, (uint8_t*)(pData + BDT_MIX_HASH_LENGTH), TCP_FIRST_BLCOK_SIZE - BDT_MIX_HASH_LENGTH);
						uint16_t aesBoxLen = 0;
						BfxBufferReadUInt16(&bufferStream, &aesBoxLen);
						tcpInterface->boxSize = _tcpInterfaceDecryptoBoxSize(tcpInterface, aesBoxLen);

						//重新调整cache
						readed = BDT_MIX_HASH_LENGTH + 2;
						tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_BODY;
						*pUsed += readed;
						if (pData == tcpInterface->cache)
						{
							size_t cacheReaded = BDT_MIX_HASH_LENGTH + 2;
							memcpy(tcpInterface->cache, tcpInterface->cache + cacheReaded, tcpInterface->cacheLength - cacheReaded);
							tcpInterface->cacheLength -= cacheReaded;
							*pUsed += readed;
							return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
						}
						readed = BDT_MIX_HASH_LENGTH + 2;
						*pUsed += readed;
						return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
					}

					tcpInterface->firstBlockSize = TCP_FIRST_EXCHANGE_SIZE;
					if (pData == tcpInterface->cache)
					{
						*pUsed += readed;
						return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
					}
					else
					{
						return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data, bytes, pBoxes, pBoxesLen, pUsed, pFirstFaram);
					}
				}
				else if (tcpInterface->firstBlockSize == TCP_FIRST_EXCHANGE_SIZE)
				{
					const BdtPeerSecret* secret = BdtStackGetSecret(stack);
					assert(secret);

					Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(BdtStackGetTls(stack));


					// 解出AESKey
					size_t olen = 0;
					size_t inLen = 0;
					r = Bdt_RsaDecrypt(pData, (uint8_t*)(secret->secret.rsa1024), secret->secretLength, tcpInterface->aesKey, &olen, BDT_AES_KEY_LENGTH, &inLen);
					if (r == 0)
					{
						BfxBufferStream bufferStream;
						BfxBufferStreamInit(&bufferStream, (uint8_t*)(pData + inLen), TCP_FIRST_BLCOK_SIZE - inLen);
						//先读keyhash
						uint8_t keyHash[8];
						BfxBufferReadByteArray(&bufferStream, keyHash, 8);
						//验证keyhash.
						if (Bdt_CheckAesKeyHash(tcpInterface->aesKey, keyHash))
						{
							uint16_t aesBoxLen = 0;
							BfxBufferReadUInt16(&bufferStream, &aesBoxLen);
							tcpInterface->boxSize = _tcpInterfaceDecryptoBoxSize(tcpInterface, aesBoxLen);

							pFirstFaram->isStartWithExchangeKey = true;
							tcpInterface->isCrypto = true;
							tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_BODY;
							if (pData == tcpInterface->cache)
							{
								//重新调整cache
								size_t cacheReaded = olen + 8 + 2;
								memcpy(tcpInterface->cache, tcpInterface->cache + cacheReaded, tcpInterface->cacheLength - cacheReaded);
								tcpInterface->cacheLength -= cacheReaded;
								*pUsed += readed;
								return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
							}
							readed = inLen + 8 + 2;
							*pUsed += readed;
							return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + readed, bytes - readed, pBoxes, pBoxesLen, pUsed, pFirstFaram);
						}
					}
					else
					{
						BLOG_WARN("decode tcp first block error.");
                        *pBoxesLen = boxCount;
                        return BFX_RESULT_INVALID_FORMAT;
					}
				}
			}
		}
		else if (tcpInterface->unboxState == BDT_TCP_UNBOX_STATE_READING_BODY)
		{
			if (tcpInterface->cacheLength != 0)
			{
				//cache存在 
				//TODO：从性能测试的角度考虑，这里最好统计一下发生了多少次数据copy
				size_t copysize = 0;
				TcpInterfaceTryFillCache(tcpInterface, tcpInterface->boxSize, data, bytes, &copysize);

				if (tcpInterface->cacheLength == tcpInterface->boxSize)
				{
					tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
					pBoxes->data = tcpInterface->cache;
					pBoxes->bytes = tcpInterface->cacheLength;
                    boxCount++;
					tcpInterface->cacheLength = 0;
					
					TcpInterfaceSwapCache(tcpInterface);
				}
                *pBoxesLen = boxCount;
				(*pUsed) += copysize;
				return BFX_RESULT_SUCCESS;
			}
			else
			{
				if (bytes >= tcpInterface->boxSize)
				{
					tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
					pBoxes->data = (uint8_t*)data;
					pBoxes->bytes = tcpInterface->boxSize;
                    boxCount++;
                    *pBoxesLen = boxCount;
					(*pUsed) += tcpInterface->boxSize;
					return BFX_RESULT_SUCCESS;
				}
				else
				{

					TcpInterfaceAppendCache(tcpInterface, data, bytes);
					(*pUsed) += bytes;
                    *pBoxesLen = boxCount;
                    return BFX_RESULT_SUCCESS;
				}
			}
		}
		else
		{
			assert(0);
		}

		break;
	case BDT_TCP_INTERFACE_STATE_PACKAGE:
	case BDT_TCP_INTERFACE_STATE_STREAM:
	case BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP:
		if (tcpInterface->unboxState == BDT_TCP_UNBOX_STATE_READING_LENGTH)
		{
			//cache存在
			if (tcpInterface->cacheLength > 0)
			{
				assert(tcpInterface->cacheLength == 1);
				tcpInterface->cache[1] = data[0];
				tcpInterface->boxSize = *((uint16_t*)(tcpInterface->cache));//TODO:没有正确处理字节序

				tcpInterface->boxSize = _tcpInterfaceDecryptoBoxSize(tcpInterface, tcpInterface->boxSize);
				tcpInterface->cacheLength = 0;
				tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_BODY;
				(*pUsed) += 1;

				return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + 1, bytes - 1, pBoxes, pBoxesLen, pUsed, pFirstFaram);
			}
			else
			{
				if (bytes > 1)
				{
					tcpInterface->boxSize = *((uint16_t*)(data));//TODO:没有正确处理字节序
					tcpInterface->boxSize = _tcpInterfaceDecryptoBoxSize(tcpInterface, tcpInterface->boxSize);
					tcpInterface->cacheLength = 0;
					tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_BODY;
					(*pUsed) += 2;
					return TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + 2, bytes - 2, pBoxes, pBoxesLen, pUsed, pFirstFaram);
				}
				else
				{
					(*pUsed) += 1;
					TcpInterfaceAppendCache(tcpInterface, data, bytes);
                    *pBoxesLen = boxCount;
                    return BFX_RESULT_SUCCESS;
				}
			}
		}
		else if (tcpInterface->unboxState == BDT_TCP_UNBOX_STATE_READING_BODY)
		{
			if (tcpInterface->cacheLength != 0)
			{
				//cache存在
				//TODO：从性能测试的角度考虑，这里最好统计一下发生了多少次数据copy
				size_t copysize = 0;
				TcpInterfaceTryFillCache(tcpInterface, tcpInterface->boxSize, data, bytes, &copysize);
				(*pUsed) += copysize;
				if (tcpInterface->cacheLength >= tcpInterface->boxSize)
				{
					tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
					pBoxes->data = tcpInterface->cache;
					pBoxes->bytes = tcpInterface->cacheLength;
                    boxCount++;
					tcpInterface->cacheLength = 0;

					TcpInterfaceSwapCache(tcpInterface);
					if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP)
					{
                        *pBoxesLen = boxCount;
						return BFX_RESULT_SUCCESS;
					}
                    if (boxCount < *pBoxesLen)
                    {
                        size_t leftBoxLen = *pBoxesLen - boxCount;
					    int ret = TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + copysize, bytes - copysize, pBoxes + 1, &leftBoxLen, pUsed, pFirstFaram);
                        boxCount += leftBoxLen;
                        *pBoxesLen = boxCount;
                        return ret;
                    }
				}
                *pBoxesLen = boxCount;
				return BFX_RESULT_SUCCESS;
			}
			else
			{
				if (bytes >= tcpInterface->boxSize)
				{
					tcpInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
					pBoxes->data = (uint8_t*)data;
					pBoxes->bytes = tcpInterface->boxSize;
                    boxCount++;
					(*pUsed) += tcpInterface->boxSize;
					if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP)
					{
                        *pBoxesLen = boxCount;
						return BFX_RESULT_SUCCESS;
					}
                    if (boxCount < *pBoxesLen)
                    {
                        size_t leftBoxLen = *pBoxesLen - boxCount;
					    int ret = TcpInterfaceUnboxParserPushData(stack, tcpInterface, data + tcpInterface->boxSize, bytes - tcpInterface->boxSize, pBoxes + 1, &leftBoxLen, pUsed, pFirstFaram);
                        boxCount += leftBoxLen;
                        *pBoxesLen = boxCount;
                        return ret;
                    }
				}
				else
				{
					(*pUsed) += bytes;
					TcpInterfaceAppendCache(tcpInterface, data, bytes);
                    *pBoxesLen = boxCount;
					return BFX_RESULT_SUCCESS;
				}
			}
		}
		else
		{
			assert(0);
			BLOG_ERROR("unknown unbox state!");
            *pBoxesLen = boxCount;
			return BFX_RESULT_INVALID_STATE;
		}

		break;
	}

    *pBoxesLen = boxCount;
	return BFX_RESULT_SUCCESS;
}

static void TcpInterfaceFreePackages(Bdt_Package** resultPackages)
{
	for (size_t ix = 0; ix < BDT_PROTOCOL_MIN_PACKAGE_LENGTH; ++ix)
	{
		Bdt_Package* package = resultPackages[ix];
		if (!package)
		{
			break;
		}
		Bdt_PackageFree(package);
	}
}


BDT_API(void) BdtPushTcpSocketData(
	BdtStack* stack, 
	BDT_SYSTEM_TCP_SOCKET socket, 
	const uint8_t* data, 
	size_t bytes, 
	const void* owner)
{
	int r = 0;
	Bdt_TcpInterface* tcpInterface = (Bdt_TcpInterface*)owner;

    assert(bytes <= 64 * 1024 && "large block, please be careful the delay interval.");

	BLOG_DEBUG("%s recv raw tcp data:%d, %d", tcpInterface->name, (int)bytes, (int)tcpInterface->cacheLength);

	if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_STREAM && (!tcpInterface->isCrypto))
	{
		TcpInterfaceOnData(stack, tcpInterface, data, bytes);
		return;
	}

	if (bytes == 0)
	{
		//tofix: ignore decoding state of tcp interface, any safity problem?
        Bdt_TcpInterfaceState oldState = BDT_TCP_INTERFACE_STATE_NONE;
		Bdt_TcpInterfaceOwner* owner = NULL;
		BfxRwLockWLock(&tcpInterface->stateLock);
		do {
            oldState = tcpInterface->state;
			if (oldState == BDT_TCP_INTERFACE_STATE_STREAM)
			{
				tcpInterface->state = BDT_TCP_INTERFACE_STATE_STREAM_RECV_CLOSED;
				break;
			}
			else if (oldState == BDT_TCP_INTERFACE_STATE_STREAM_CLOSING)
			{
				tcpInterface->state = BDT_TCP_INTERFACE_STATE_CLOSED;
				owner = tcpInterface->owner;
				tcpInterface->owner = NULL;
			}
			else
			{
				// do nothing wait send error
			}
		} while (false);
		BfxRwLockWUnlock(&tcpInterface->stateLock);

		if (oldState == BDT_TCP_INTERFACE_STATE_STREAM) // remote closed
		{
			TcpInterfaceOnData(stack, tcpInterface, NULL, 0);
		}
        if (oldState == BDT_TCP_INTERFACE_STATE_STREAM_CLOSING) // remote/local closed
        {
            TcpInterfaceReleaseSocket(tcpInterface);
        }
		if (owner && owner->release)
		{
			owner->release(owner);
		}
		return;
	}

	
	TcpInterfaceBox boxes[TCP_MAX_BOX_LEN];
	size_t boxesLen = TCP_MAX_BOX_LEN;
	size_t used = 0;
	TcpInterfaceFirstParams firstParams;
	memset(&firstParams, 0, sizeof(firstParams));

	Bdt_Package* resultPackages[BDT_MAX_PACKAGE_MERGE_LENGTH] = { 0 };
	r = TcpInterfaceUnboxParserPushData(
		stack, 
		tcpInterface, 
		data, 
		bytes, 
		boxes, 
		&boxesLen, 
		&used, 
		&firstParams);

	BLOG_DEBUG("%s pushed raw data to parser, cache:%zu, parsed:%zu, boxes: %zu", 
		tcpInterface->name, 
		tcpInterface->cacheLength, 
		used, 
		boxesLen);

	if (r != BFX_RESULT_SUCCESS)
	{
		BLOG_WARN("%s unbox parser push data return error(result = %d),will break", tcpInterface->name, r);
		Bdt_TcpInterfaceReset(tcpInterface);
		return;
	}

	if (boxesLen < 1)
	{
		return;
	}

	if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_STREAM)
	{
		assert(tcpInterface->isCrypto);
		for (size_t i = 0; i < boxesLen; ++i)
		{
			TcpInterfaceBox* pBox = boxes + i;
			r = Bdt_AesDecryptDataWithPadding(
				tcpInterface->aesKey, 
				pBox->data, 
				pBox->bytes, 
				pBox->data, 
				pBox->bytes);
			if (r <= 0)
			{
				BLOG_WARN("%s unbox parser decrypto data return error(result = %d),will break.", tcpInterface->name, r);
				Bdt_TcpInterfaceReset(tcpInterface);
				return;
			}
			TcpInterfaceOnData(stack, tcpInterface, pBox->data, r);
		}
	}
	else if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_PACKAGE)
	{
		for (size_t i = 0; i < boxesLen; ++i)
		{
			TcpInterfaceBox* pBox = boxes + i;
			size_t realSize = pBox->bytes;
			if (tcpInterface->isCrypto)
			{
				r = Bdt_AesDecryptDataWithPadding(
					tcpInterface->aesKey, 
					pBox->data, 
					pBox->bytes, 
					pBox->data, 
					pBox->bytes);
				if (r <= 0)
				{
					BLOG_WARN("%s unbox parser decrypto data return error(result = %d),will break.", tcpInterface->name, r);
					Bdt_TcpInterfaceReset(tcpInterface);
					return;
				}
				realSize = r;
			}

			r = Bdt_DecodePackage(
				pBox->data, 
				realSize, 
				NULL, 
				resultPackages, 
				false);
			if (r < 0)
			{
				BLOG_WARN("%s decode package return error(result = %d),will break.", tcpInterface->name, r);
				Bdt_TcpInterfaceReset(tcpInterface);
				return;
			}
			if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_PACKAGE)
			{
				TcpInterfaceOnPackages(stack, tcpInterface, resultPackages);
			}
			TcpInterfaceFreePackages(resultPackages);
		}
	}
	else if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE
		|| tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP)
	{
		assert(boxesLen == 1);
		TcpInterfaceBox* pBox = boxes;
		size_t realSize = pBox->bytes;
		if (tcpInterface->isCrypto)
		{
			r = Bdt_AesDecryptDataWithPadding(
				tcpInterface->aesKey, 
				pBox->data, 
				pBox->bytes, 
				pBox->data, 
				pBox->bytes);
			if (r <= 0)
			{
				BLOG_WARN("%s unbox parser decrypto data return error(result = %d),will break.", tcpInterface->name, r);
				Bdt_TcpInterfaceReset(tcpInterface);
				return;
			}
			realSize = r;
		}

		r = Bdt_DecodePackage(
			pBox->data, 
			realSize, 
			NULL, 
			resultPackages, 
			firstParams.isStartWithExchangeKey);
		if (r < 0)
		{
			BLOG_WARN("%s decode package return error(result = %d),will break.", tcpInterface->name, r);
			Bdt_TcpInterfaceReset(tcpInterface);
			return;
		}
		if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE)
		{
			r = TcpInterfaceOnFirstPackages(
				stack, 
				tcpInterface, 
				resultPackages, 
				&firstParams.remotePeerid);
		}
		else if (tcpInterface->state == BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP)
		{
			r = TcpInterfaceOnFirstResp(stack, tcpInterface, resultPackages);
		}
		TcpInterfaceFreePackages(resultPackages);

        if (r != BFX_RESULT_SUCCESS)
        {
            return;
        }
	}
	else
	{
		assert(0);
        return;
	}

	if (used < bytes)
	{
		//TODO:注意这个递归处理
		BdtPushTcpSocketData(stack, socket, data + used, bytes - used, owner);
	}
	return;
}

#endif //__BDT_TCP_INTERFACE_IMP_INL__
