#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

#include "./Package.inl"
#include <assert.h>
#include <string.h>

//---------------------------------------------------------------------------------------

size_t Bdt_GetPackageMinSize(uint8_t cmdtype)
{
	switch (cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		return 94;
	}
	return 2; // <TODO>去掉cmdType后其实是2，但这里应该是返回整个包长，暂时改一下避免error
}

int ReadPackageFieldValueFromReference(uint8_t cmdtype, uint16_t fieldFlags, const Bdt_Package* refPackage, void* pFieldValue)
{
	const char* fieldName = GetPackageFieldName(cmdtype, fieldFlags);
	if (fieldName)
	{
		return GetPackageFieldValueByName(refPackage, fieldName, pFieldValue);
	}
	return BFX_RESULT_NOT_FOUND;
}


Bdt_ExchangeKeyPackage* Bdt_ExchangeKeyPackageCreate()
{
	Bdt_ExchangeKeyPackage* pResult =  BFX_MALLOC_OBJ(Bdt_ExchangeKeyPackage);
	Bdt_ExchangeKeyPackageInit(pResult);

	return pResult;
}
void Bdt_ExchangeKeyPackageInit(Bdt_ExchangeKeyPackage* package)
{
	memset(package, 0, sizeof(Bdt_ExchangeKeyPackage));
	package->cmdtype = BDT_EXCHANGEKEY_PACKAGE;
}

void Bdt_ExchangeKeyPackageFinalize(Bdt_ExchangeKeyPackage* package)
{
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}
void Bdt_ExchangeKeyPackageFree(Bdt_ExchangeKeyPackage* package)
{
	Bdt_ExchangeKeyPackageFinalize(package);
	BFX_FREE(package);
}

uint32_t Bdt_ExchangeKeyPackageFillWithNext(
	Bdt_ExchangeKeyPackage* package, 
	const Bdt_Package* nextPackage)
{
	if (nextPackage->cmdtype == BDT_SYN_TUNNEL_PACKAGE)
	{
		package->seq = ((const Bdt_SynTunnelPackage*)nextPackage)->seq;
		package->fromPeerid = ((const Bdt_SynTunnelPackage*)nextPackage)->fromPeerid;
		package->peerInfo = ((const Bdt_SynTunnelPackage*)nextPackage)->peerInfo;
		BdtAddRefPeerInfo(package->peerInfo);
	}
	else if (nextPackage->cmdtype == BDT_ACK_TUNNEL_PACKAGE)
	{
		package->seq = ((const Bdt_AckTunnelPackage*)nextPackage)->seq;
		package->fromPeerid = *BdtPeerInfoGetPeerid(((const Bdt_AckTunnelPackage*)nextPackage)->peerInfo);
		package->peerInfo = ((const Bdt_AckTunnelPackage*)nextPackage)->peerInfo;
		BdtAddRefPeerInfo(package->peerInfo);
	}
	else
	{
		assert(false);
		return BFX_RESULT_INVALIDPARAM;
	}
	return BFX_RESULT_SUCCESS;
}

Bdt_SynTunnelPackage* Bdt_SynTunnelPackageCreate()
{
	Bdt_SynTunnelPackage* pResult = BFX_MALLOC_OBJ(Bdt_SynTunnelPackage);
	Bdt_SynTunnelPackageInit(pResult);
	return pResult;
}

void Bdt_SynTunnelPackageInit(Bdt_SynTunnelPackage* package)
{
	memset(package, 0, sizeof(Bdt_SynTunnelPackage));
	package->cmdtype = BDT_SYN_TUNNEL_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}
void Bdt_SynTunnelPackageFinalize(Bdt_SynTunnelPackage* package)
{
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
	if (package->proxyPeerInfo)
	{
		BdtReleasePeerInfo(package->proxyPeerInfo);
		package->proxyPeerInfo = NULL;
	}
}

void Bdt_SynTunnelPackageFree(Bdt_SynTunnelPackage* package)
{
	Bdt_SynTunnelPackageFinalize(package);
	BFX_FREE(package);
	return;
}

Bdt_AckTunnelPackage* Bdt_AckTunnelPackageCreate()
{
	Bdt_AckTunnelPackage* pResult = BFX_MALLOC_OBJ(Bdt_AckTunnelPackage);
	
	Bdt_AckTunnelPackageInit(pResult);
	return pResult;
}	

void Bdt_AckTunnelPackageInit(Bdt_AckTunnelPackage* package)
{
	memset(package, 0, sizeof(Bdt_AckTunnelPackage));
	package->cmdtype = BDT_ACK_TUNNEL_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_AckTunnelPackageFinalize(Bdt_AckTunnelPackage* package)
{
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}

void Bdt_AckTunnelPackageFree(Bdt_AckTunnelPackage* package)
{
	Bdt_AckTunnelPackageFinalize(package);
	BFX_FREE(package);
	return;
}


Bdt_AckAckTunnelPackage* Bdt_AckAckTunnelPackageCreate()
{
	Bdt_AckAckTunnelPackage* pResult = BFX_MALLOC_OBJ(Bdt_AckAckTunnelPackage);
	
	Bdt_AckAckTunnelPackageInit(pResult);
	return pResult;
}
void Bdt_AckAckTunnelPackageInit(Bdt_AckAckTunnelPackage* package)
{
	memset(package, 0, sizeof(Bdt_AckAckTunnelPackage));
	package->cmdtype = BDT_ACKACK_TUNNEL_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}
void Bdt_AckAckTunnelPackageFinalize(Bdt_AckAckTunnelPackage* package)
{

}
void Bdt_AckAckTunnelPackageFree(Bdt_AckAckTunnelPackage* package)
{
	Bdt_AckAckTunnelPackageFinalize(package);
	BFX_FREE(package);
	return;
}

Bdt_PingTunnelPackage* Bdt_PingTunnelPackageCreate()
{
	Bdt_PingTunnelPackage* pResult = BFX_MALLOC_OBJ(Bdt_PingTunnelPackage);
	
	Bdt_PingTunnelPackageInit(pResult);
	return pResult;
}
void Bdt_PingTunnelPackageInit(Bdt_PingTunnelPackage* package)
{
	memset(package, 0, sizeof(Bdt_PingTunnelPackage));
	package->cmdtype = BDT_PING_TUNNEL_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}
void Bdt_PingTunnelPackageFinalize(Bdt_PingTunnelPackage* package)
{
	return;
}
void Bdt_PingTunnelPackageFree(Bdt_PingTunnelPackage* package)
{
	Bdt_PingTunnelPackageFinalize(package);
	BFX_FREE(package);
	return;
}

Bdt_PingTunnelRespPackage* Bdt_PingTunnelRespPackageCreate()
{
	Bdt_PingTunnelRespPackage* pResult = BFX_MALLOC_OBJ(Bdt_PingTunnelRespPackage);
	
	Bdt_PingTunnelRespPackageInit(pResult);
	return pResult;
}
void Bdt_PingTunnelRespPackageInit(Bdt_PingTunnelRespPackage* package)
{
	memset(package, 0, sizeof(Bdt_PingTunnelRespPackage));
	package->cmdtype = BDT_PING_TUNNEL_RESP_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}
void Bdt_PingTunnelRespPackageFinalize(Bdt_PingTunnelRespPackage* package)
{
	return;
}
void Bdt_PingTunnelRespPackageFree(Bdt_PingTunnelRespPackage* package)
{
	Bdt_PingTunnelRespPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnCallPackage* Bdt_SnCallPackageCreate()
{
	Bdt_SnCallPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnCallPackage);
	
	Bdt_SnCallPackageInit(pResult);
	return pResult;
}
void Bdt_SnCallPackageInit(Bdt_SnCallPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnCallPackage));
	package->cmdtype = BDT_SN_CALL_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
	BdtEndpointArrayInit(&package->reverseEndpointArray, 0);
}
void Bdt_SnCallPackageFinalize(Bdt_SnCallPackage* package)
{
	BdtEndpointArrayUninit(&package->reverseEndpointArray);
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}

	if (package->payload)
	{
		BfxBufferRelease(package->payload);
		package->payload = NULL;
	}

	return;
}
void Bdt_SnCallPackageFree(Bdt_SnCallPackage* package)
{
	Bdt_SnCallPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnCallRespPackage* Bdt_SnCallRespPackageCreate()
{
	Bdt_SnCallRespPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnCallRespPackage);
	
	Bdt_SnCallRespPackageInit(pResult);
	return pResult;
}
void Bdt_SnCallRespPackageInit(Bdt_SnCallRespPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnCallRespPackage));
	package->cmdtype = BDT_SN_CALL_RESP_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_SnCallRespPackageFinalize(Bdt_SnCallRespPackage* package)
{
	if (package->toPeerInfo)
	{
		BdtReleasePeerInfo(package->toPeerInfo);
		package->toPeerInfo = NULL;
	}
	return;
}

void Bdt_SnCallRespPackageFree(Bdt_SnCallRespPackage* package)
{
	Bdt_SnCallRespPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnCalledPackage* Bdt_SnCalledPackageCreate()
{
	Bdt_SnCalledPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnCalledPackage);
	
	Bdt_SnCalledPackageInit(pResult);
	return pResult;
}
void Bdt_SnCalledPackageInit(Bdt_SnCalledPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnCalledPackage));
	package->cmdtype = BDT_SN_CALLED_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
	BdtEndpointArrayInit(&package->reverseEndpointArray, 0);
}
void Bdt_SnCalledPackageFinalize(Bdt_SnCalledPackage* package)
{
	BdtEndpointArrayUninit(&package->reverseEndpointArray);
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
	if (package->payload)
	{
		BfxBufferRelease(package->payload);
		package->payload = NULL;
	}
}
void Bdt_SnCalledPackageFree(Bdt_SnCalledPackage* package)
{
	Bdt_SnCalledPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnCalledRespPackage* Bdt_SnCalledRespPackageCreate()
{
	Bdt_SnCalledRespPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnCalledRespPackage);
	
	Bdt_SnCalledRespPackageInit(pResult);
	return pResult;
}

void Bdt_SnCalledRespPackageInit(Bdt_SnCalledRespPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnCalledRespPackage));
	package->cmdtype = BDT_SN_CALLED_RESP_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_SnCalledRespPackageFinalize(Bdt_SnCalledRespPackage* package)
{
	
	return;
}

void Bdt_SnCalledRespPackageFree(Bdt_SnCalledRespPackage* package)
{
	Bdt_SnCalledRespPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnPingPackage* Bdt_SnPingPackageCreate()
{
	Bdt_SnPingPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnPingPackage);
	
	Bdt_SnPingPackageInit(pResult);
	return pResult;
}

void Bdt_SnPingPackageInit(Bdt_SnPingPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnPingPackage));
	package->cmdtype = BDT_SN_PING_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_SnPingPackageFinalize(Bdt_SnPingPackage* package)
{
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}

void Bdt_SnPingPackageFree(Bdt_SnPingPackage* package)
{
	Bdt_SnPingPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_SnPingRespPackage* Bdt_SnPingRespPackageCreate()
{
	Bdt_SnPingRespPackage* pResult = BFX_MALLOC_OBJ(Bdt_SnPingRespPackage);
	
	Bdt_SnPingRespPackageInit(pResult);
	return pResult;
}

void Bdt_SnPingRespPackageInit(Bdt_SnPingRespPackage* package)
{
	memset(package, 0, sizeof(Bdt_SnPingRespPackage));
	package->cmdtype = BDT_SN_PING_RESP_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
	BdtEndpointArrayInit(&package->endpointArray, 0);
}

void Bdt_SnPingRespPackageFinalize(Bdt_SnPingRespPackage* package)
{
	BdtEndpointArrayUninit(&package->endpointArray);
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}
void Bdt_SnPingRespPackageFree(Bdt_SnPingRespPackage* package)
{
	Bdt_SnPingRespPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_DataGramPackage* Bdt_DataGramPackageCreate()
{
	Bdt_DataGramPackage* pResult = BFX_MALLOC_OBJ(Bdt_DataGramPackage);
	
	Bdt_DataGramPackageInit(pResult);
	return pResult;
}
void Bdt_DataGramPackageInit(Bdt_DataGramPackage* package)
{
	memset(package, 0, sizeof(Bdt_DataGramPackage));
	package->cmdtype = BDT_DATA_GRAM_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_DataGramPackageFinalize(Bdt_DataGramPackage* package)
{
	if (package->authorPeerInfo)
	{
		BdtReleasePeerInfo(package->authorPeerInfo);
		package->authorPeerInfo = NULL;
	}
}
void Bdt_DataGramPackageFree(Bdt_DataGramPackage* package)
{
	Bdt_DataGramPackageFinalize(package);
	BFX_FREE(package);
}


Bdt_TcpSynConnectionPackage* Bdt_TcpSynConnectionPackageCreate()
{
	Bdt_TcpSynConnectionPackage* pResult = BFX_MALLOC_OBJ(Bdt_TcpSynConnectionPackage);
	
	Bdt_TcpSynConnectionPackageInit(pResult);
	return pResult;
}
void Bdt_TcpSynConnectionPackageInit(Bdt_TcpSynConnectionPackage* package)
{
	memset(package, 0, sizeof(Bdt_TcpSynConnectionPackage));
	package->cmdtype = BDT_TCP_SYN_CONNECTION_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
	BdtEndpointArrayInit(&package->reverseEndpointArray,0);
}

void Bdt_TcpSynConnectionPackageFinalize(Bdt_TcpSynConnectionPackage* package)
{
	BdtEndpointArrayUninit(&package->reverseEndpointArray);
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}
void Bdt_TcpSynConnectionPackageFree(Bdt_TcpSynConnectionPackage* package)
{
	Bdt_TcpSynConnectionPackageFinalize(package);
	BFX_FREE(package);
}

Bdt_TcpAckConnectionPackage* Bdt_TcpAckConnectionPackageCreate()
{
	Bdt_TcpAckConnectionPackage* pResult = BFX_MALLOC_OBJ(Bdt_TcpAckConnectionPackage);

	Bdt_TcpAckConnectionPackageInit(pResult);
	return pResult;
}
void Bdt_TcpAckConnectionPackageInit(Bdt_TcpAckConnectionPackage* package)
{
	memset(package, 0, sizeof(Bdt_TcpAckConnectionPackage));
	package->cmdtype = BDT_TCP_ACK_CONNECTION_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}

void Bdt_TcpAckConnectionPackageFinalize(Bdt_TcpAckConnectionPackage* package)
{
	if (package->peerInfo)
	{
		BdtReleasePeerInfo(package->peerInfo);
		package->peerInfo = NULL;
	}
}

void Bdt_TcpAckConnectionPackageFree(Bdt_TcpAckConnectionPackage* package)
{
	Bdt_TcpAckConnectionPackageFinalize(package);
	BFX_FREE(package);
}


Bdt_TcpAckAckConnectionPackage* Bdt_TcpAckAckConnectionPackageCreate()
{
	Bdt_TcpAckAckConnectionPackage* pResult = BFX_MALLOC_OBJ(Bdt_TcpAckAckConnectionPackage);

	Bdt_TcpAckAckConnectionPackageInit(pResult);
	return pResult;
}
void Bdt_TcpAckAckConnectionPackageInit(Bdt_TcpAckAckConnectionPackage* package)
{
	memset(package, 0, sizeof(Bdt_TcpAckAckConnectionPackage));
	package->cmdtype = BDT_TCP_ACKACK_CONNECTION_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
}
void Bdt_TcpAckAckConnectionPackageFinalize(Bdt_TcpAckAckConnectionPackage* package)
{
	return;
}
void Bdt_TcpAckAckConnectionPackageFree(Bdt_TcpAckAckConnectionPackage* package)
{
	Bdt_TcpAckAckConnectionPackageFinalize(package);
	BFX_FREE(package);
}


//------------------------------------------------------------------------------------------------
Bdt_SessionDataPackage* Bdt_SessionDataPackageCreate()
{
	Bdt_SessionDataPackage* pResult = BFX_MALLOC_OBJ(Bdt_SessionDataPackage);
	
	Bdt_SessionDataPackageInit(pResult);
	return pResult;
}

void Bdt_SessionDataPackageInit(Bdt_SessionDataPackage* package)
{
	memset(package, 0, sizeof(Bdt_SessionDataPackage));
	package->cmdtype = BDT_SESSION_DATA_PACKAGE;
	package->cmdflags = BDT_PACKAGE_FLAG_DEFAULT;
	package->synPart = NULL;
	package->packageIDPart = NULL;
}

void Bdt_SessionDataPackageFinalize(Bdt_SessionDataPackage* package)
{	
	if (package->synPart)
	{
		BFX_FREE(package->synPart);
		package->synPart = NULL;
	}

	if (package->packageIDPart)
	{
		BFX_FREE(package->packageIDPart);
		package->packageIDPart = NULL;
	}

	if (package->sackArray)
	{
		BFX_FREE(package->sackArray);
		package->sackArray = NULL;
	}
	return;
}

void Bdt_SessionDataPackageFree(Bdt_SessionDataPackage* package)
{
	Bdt_SessionDataPackageFinalize(package);
	BFX_FREE(package);
}

 int Bdt_BufferWritePeerid(BfxBufferStream* bufferStream, const BdtPeerid* pPeerID, size_t* pWriteBytes)
{
	*pWriteBytes = BDT_PEERID_LENGTH;
	return BfxBufferWriteByteArray(bufferStream, pPeerID->pid, BDT_PEERID_LENGTH);
}

 int Bdt_BufferReadPeerid(BfxBufferStream* bufferStream, BdtPeerid* pResult)
{
	return BfxBufferReadByteArray(bufferStream, pResult->pid, BDT_PEERID_LENGTH);
}

 int Bdt_BufferReadEndpointArray(BfxBufferStream* bufferStream, BdtEndpointArray* pResult)
{
	int r = 0;
	uint16_t arraySize;
	int totalRead = 0;
	r = BfxBufferReadUInt16(bufferStream, &arraySize);
	if (r <= 0)
	{
		return 0;
	}
	totalRead += r;
	if (BfxBufferStreamGetTailLength(bufferStream) < (size_t)(arraySize) * 6)
	{
		BLOG_WARN("readd endpoint array error: array.size too large.\n");
		return 0;
	}
	BdtEndpointArrayInit(pResult, arraySize);
	for (int i = 0; i < arraySize; ++i)
	{
		BdtEndpoint ep;
		r = BfxBufferReadUInt8(bufferStream, &ep.flag);
		if (r <= 0)
		{
			return 0;
		}
		totalRead += r;
		
		r = BfxBufferReadUInt16(bufferStream, &ep.port);
		if (r <= 0)
		{
			return 0;
		}
		totalRead += r;

		if (ep.flag & BDT_ENDPOINT_IP_VERSION_4)
		{
			r = BfxBufferReadByteArray(bufferStream, ep.address, 4);
		} 
		else 
		{
			r = BfxBufferReadByteArray(bufferStream, (uint8_t*)ep.addressV6, 16);
		}
		
		if (r <= 0)
		{
			return 0;
		}
		totalRead += r;

		BdtEndpointArrayPush(pResult, &ep);
	}

	return totalRead;
}

 int Bdt_BufferWriteEndpointArray(BfxBufferStream* bufferStream, BdtEndpointArray* pResult, size_t* pWriteBytes)
{
	int r = 0;
	*pWriteBytes = 0;
	size_t writeBytes = 0;
	size_t totalWrite = 0;

	r = BfxBufferWriteUInt16(bufferStream, (uint16_t)pResult->size, &writeBytes);
	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWrite += writeBytes;
	writeBytes = 0;

	for (int i = 0; i < pResult->size; ++i)
	{
		BdtEndpoint* pEP = pResult->list + i;
		r = BfxBufferWriteUInt8(bufferStream, pEP->flag, &writeBytes);
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWrite += writeBytes;
		writeBytes = 0;

		r = BfxBufferWriteUInt16(bufferStream, pEP->port, &writeBytes);
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWrite += writeBytes;
		writeBytes = 0;

		if (pEP->flag & BDT_ENDPOINT_IP_VERSION_4)
		{
			r = BfxBufferWriteByteArray(bufferStream, pEP->address, 4);
			writeBytes = 4;
		}
		else
		{
			r = BfxBufferWriteByteArray(bufferStream, (uint8_t*)pEP->addressV6, 16);
			writeBytes = 16;
		}
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWrite += writeBytes;
		writeBytes = 0;
	}
	*pWriteBytes = totalWrite;
	return BFX_RESULT_SUCCESS;
}

bool Bdt_IsEqualPackage(const Bdt_Package* lhs, const Bdt_Package* rhs)
{
	if (lhs->cmdtype != rhs->cmdtype)
	{
		return false;
	}

	Bdt_ExchangeKeyPackage* pExchangeKeyPackage1, * pExchangeKeyPackage2;
	Bdt_SynTunnelPackage* pSynTunnelPackage1, * pSynTunnelPackage2;
	Bdt_AckTunnelPackage* pAckTunnelPackage1, * pAckTunnelPackage2;
	Bdt_SnCallPackage* pSNCallPackage1, * pSNCallPackage2;
	Bdt_SnCallRespPackage* pSNCallRespPackage1,* pSNCallRespPackage2;
	Bdt_SnCalledPackage* pSNCalledPackage1, * pSNCalledPackage2;
	Bdt_SnPingPackage* pSNPingPackage1, * pSNPingPackage2;
	Bdt_SnPingRespPackage* pSNPingRespPackage1,* pSNPingRespPackage2;
	Bdt_DataGramPackage* pDataGramPackage1,* pDataGramPackage2;
	Bdt_TcpSynConnectionPackage* pTcpSynConnectionPackage1,* pTcpSynConnectionPackage2;
	Bdt_TcpAckConnectionPackage* pTcpAckConnectionPackage1,* pTcpAckConnectionPackage2;
	Bdt_SessionDataPackage* pSessionDataPackage1, * pSessionDataPackage2;

	switch (lhs->cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		pExchangeKeyPackage1 = (Bdt_ExchangeKeyPackage*)lhs;
		pExchangeKeyPackage2 = (Bdt_ExchangeKeyPackage*)rhs;
		if (memcmp(pExchangeKeyPackage1, pExchangeKeyPackage2,
			sizeof(Bdt_ExchangeKeyPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pExchangeKeyPackage1->peerInfo, pExchangeKeyPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_SYN_TUNNEL_PACKAGE:
		pSynTunnelPackage1 = (Bdt_SynTunnelPackage*)lhs;
		pSynTunnelPackage2 = (Bdt_SynTunnelPackage*)rhs;
		if (memcmp(pSynTunnelPackage1, pSynTunnelPackage2,
			sizeof(Bdt_SynTunnelPackage) - sizeof(BdtPeerInfo*) * 2) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pSynTunnelPackage1->peerInfo, pSynTunnelPackage2->peerInfo))
		{
			if (BdtPeerInfoIsEqual(pSynTunnelPackage1->proxyPeerInfo, pSynTunnelPackage2->proxyPeerInfo))
			{
				return true;
			}
		}
		return false;
	case BDT_ACK_TUNNEL_PACKAGE:
		pAckTunnelPackage1 = (Bdt_AckTunnelPackage*)lhs;
		pAckTunnelPackage2 = (Bdt_AckTunnelPackage*)rhs;
		if (memcmp(pAckTunnelPackage1, pAckTunnelPackage2,
			sizeof(Bdt_AckTunnelPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pAckTunnelPackage1->peerInfo, pAckTunnelPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_SN_CALL_PACKAGE:
		pSNCallPackage1 = (Bdt_SnCallPackage*)lhs;
		pSNCallPackage2 = (Bdt_SnCallPackage*)rhs;
		if (memcmp(pSNCallPackage1, pSNCallPackage2,
			sizeof(Bdt_SnCallPackage) - sizeof(BdtPeerInfo*) - sizeof(BFX_BUFFER_HANDLE)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pSNCallPackage1->peerInfo, pSNCallPackage2->peerInfo))
		{
			if (BfxBufferIsEqual(pSNCallPackage1->payload, pSNCallPackage2->payload))
			{
				return true;
			}
		}
		return false;
	case BDT_SN_CALL_RESP_PACKAGE:
		pSNCallRespPackage1 = (Bdt_SnCallRespPackage*)lhs;
		pSNCallRespPackage2 = (Bdt_SnCallRespPackage*)rhs;
		if (memcmp(pSNCallRespPackage1, pSNCallRespPackage2,
			sizeof(Bdt_SnCallRespPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}

		if (BdtPeerInfoIsEqual(pSNCallRespPackage1->toPeerInfo, pSNCallRespPackage2->toPeerInfo))
		{
			return true;
		}
		return false;
	case BDT_SN_CALLED_PACKAGE:
		pSNCalledPackage1 = (Bdt_SnCalledPackage*)lhs;
		pSNCalledPackage2 = (Bdt_SnCalledPackage*)lhs;
		if (memcmp(pSNCalledPackage1, pSNCalledPackage2,
			sizeof(Bdt_SnCalledPackage) - sizeof(BdtPeerInfo*) - sizeof(BFX_BUFFER_HANDLE)) != 0)
		{
			return false;
		}

		if (BdtPeerInfoIsEqual(pSNCalledPackage1->peerInfo, pSNCalledPackage2->peerInfo))
		{
			if (BfxBufferIsEqual(pSNCalledPackage1->payload, pSNCalledPackage2->payload))
			{
				return true;
			}
		}
		return false;
	case BDT_SN_PING_PACKAGE:
		pSNPingPackage1 = (Bdt_SnPingPackage*)lhs;
		pSNPingPackage2 = (Bdt_SnPingPackage*)rhs;
		if (memcmp(pSNPingPackage1, pSNPingPackage2,
			sizeof(Bdt_SnPingPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pSNPingPackage1->peerInfo, pSNPingPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_SN_PING_RESP_PACKAGE:
		pSNPingRespPackage1 = (Bdt_SnPingRespPackage*)lhs;
		pSNPingRespPackage2 = (Bdt_SnPingRespPackage*)rhs;
		if (memcmp(pSNPingRespPackage1, pSNPingRespPackage2,
			sizeof(Bdt_SnPingRespPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pSNPingRespPackage1->peerInfo, pSNPingRespPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_DATA_GRAM_PACKAGE:
		pDataGramPackage1 = (Bdt_DataGramPackage*)lhs;
		pDataGramPackage2 = (Bdt_DataGramPackage*)rhs;
		if (memcmp(pDataGramPackage1, pDataGramPackage2,
			sizeof(Bdt_DataGramPackage) - sizeof(BdtPeerInfo*) - sizeof(BFX_BUFFER_HANDLE)) != 0)
		{
			return false;
		}

		if (BdtPeerInfoIsEqual(pDataGramPackage1->authorPeerInfo, pDataGramPackage2->authorPeerInfo))
		{
			if (BfxBufferIsEqual(pDataGramPackage1->data, pDataGramPackage2->data))
			{
				return true;
			}
		}
		return false;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		pTcpSynConnectionPackage1 = (Bdt_TcpSynConnectionPackage*)lhs;
		pTcpSynConnectionPackage2 = (Bdt_TcpSynConnectionPackage*)rhs;
		if (memcmp(pTcpSynConnectionPackage1, pTcpSynConnectionPackage2,
			sizeof(Bdt_TcpSynConnectionPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pTcpSynConnectionPackage1->peerInfo, pTcpSynConnectionPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		pTcpAckConnectionPackage1 = (Bdt_TcpAckConnectionPackage*)lhs;
		pTcpAckConnectionPackage2 = (Bdt_TcpAckConnectionPackage*)rhs;
		if (memcmp(pTcpAckConnectionPackage1, pTcpAckConnectionPackage2,
			sizeof(Bdt_TcpAckConnectionPackage) - sizeof(BdtPeerInfo*)) != 0)
		{
			return false;
		}
		if (BdtPeerInfoIsEqual(pTcpAckConnectionPackage1->peerInfo, pTcpAckConnectionPackage2->peerInfo))
		{
			return true;
		}
		return false;
	case BDT_SESSION_DATA_PACKAGE:
		pSessionDataPackage1 = (Bdt_SessionDataPackage*)lhs;
		pSessionDataPackage2 = (Bdt_SessionDataPackage*)rhs;
		if (memcmp(pSessionDataPackage1, pSessionDataPackage2,
			sizeof(Bdt_SessionDataPackage) - sizeof(Bdt_SessionDataPackageIdPart*) - sizeof(Bdt_SessionDataSynPart*) - sizeof(uint8_t*)) != 0)
		{
			return false;
		}
		if (pSessionDataPackage1->synPart != pSessionDataPackage2->synPart)
		{
			if (pSessionDataPackage1->synPart != NULL && pSessionDataPackage2->synPart != NULL)
			{
				if (memcmp(pSessionDataPackage1->synPart, pSessionDataPackage2->synPart, sizeof(Bdt_SessionDataSynPart)) != 0)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
			
		}

		if (pSessionDataPackage1->packageIDPart != pSessionDataPackage2->packageIDPart)
		{
			if (pSessionDataPackage1->packageIDPart != NULL && pSessionDataPackage2->packageIDPart != NULL)
			{
				if (memcmp(pSessionDataPackage1->packageIDPart, pSessionDataPackage2->packageIDPart, sizeof(Bdt_SessionDataPackageIdPart)) != 0)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		if (pSessionDataPackage1->payload != pSessionDataPackage2->payload)
		{
			if (pSessionDataPackage1->payload != NULL && pSessionDataPackage2->payload != NULL)
			{
				if (memcmp(pSessionDataPackage1->payload, pSessionDataPackage2->payload, pSessionDataPackage1->payloadLength) == 0)
				{
					return true;
				}
			}
			else
			{
				return false;
			}
		}

		return false;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		return memcmp(lhs, rhs, sizeof(Bdt_TcpAckAckConnectionPackage)) == 0;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		return memcmp(lhs, rhs, sizeof(Bdt_AckAckTunnelPackage)) == 0;
	case BDT_PING_TUNNEL_PACKAGE:
		return memcmp(lhs, rhs, sizeof(Bdt_PingTunnelPackage)) == 0;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		return memcmp(lhs, rhs, sizeof(Bdt_PingTunnelRespPackage)) == 0;
	case BDT_SN_CALLED_RESP_PACKAGE:
		return memcmp(lhs, rhs, sizeof(Bdt_SnCalledRespPackage)) == 0;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
const char* Bdt_PackageGetNameByCmdType(uint8_t cmdtype)
{
	switch (cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		return "PACKAGE_EXCHANGEKEY";
	case BDT_SYN_TUNNEL_PACKAGE:
		return "PACKAGE_SYN_TUNNEL";
	case BDT_ACK_TUNNEL_PACKAGE:
		return "PACKAGE_ACK_TUNNEL";
	case BDT_ACKACK_TUNNEL_PACKAGE:
		return "PACKAGE_ACKACK_TUNNEL";
	case BDT_PING_TUNNEL_PACKAGE:
		return "PACKAGE_PING_TUNNEL";
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		return "PACKAGE_PING_TUNNEL_RESP";
	case BDT_SN_CALL_PACKAGE:
		return "PACKAGE_SN_CALL";
	case BDT_SN_CALL_RESP_PACKAGE:
		return "PACKAGE_SN_CALL_RESP";
	case BDT_SN_CALLED_PACKAGE:
		return "PACKAGE_SN_CALLED";
	case BDT_SN_CALLED_RESP_PACKAGE:
		return "PACKAGE_SN_CALLED_RESP";
	case BDT_SN_PING_PACKAGE:
		return "PACKAGE_SN_PING";
	case BDT_SN_PING_RESP_PACKAGE:
		return "PACKAGE_SN_PING_RESP";
	case BDT_DATA_GRAM_PACKAGE:
		return "PACKAGE_DATA_GRAM";
	case BDT_SESSION_DATA_PACKAGE:
		return "PACKAGE_SESSION_DATA";
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		return "PACKAGE_TCP_SYN_CONNECTION";
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		return "PACKAGE_TCP_ACK_CONNECTION";
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		return "PACKAGE_TCP_ACKACK_CONNECTION";

	}
	return "PACKAGE_UNKNOWN";
}


void Bdt_PackageFinalize(Bdt_Package* pkg)
{
	switch (pkg->cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		Bdt_ExchangeKeyPackageFinalize((Bdt_ExchangeKeyPackage*)pkg);
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		Bdt_SynTunnelPackageFinalize((Bdt_SynTunnelPackage*)pkg);
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		Bdt_AckTunnelPackageFinalize((Bdt_AckTunnelPackage*)pkg);
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		Bdt_AckAckTunnelPackageFinalize((Bdt_AckAckTunnelPackage*)pkg);
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		Bdt_PingTunnelPackageFinalize((Bdt_PingTunnelPackage*)pkg);
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		Bdt_PingTunnelRespPackageFinalize((Bdt_PingTunnelRespPackage*)pkg);
		break;
	case BDT_SN_CALL_PACKAGE:
		Bdt_SnCallPackageFinalize((Bdt_SnCallPackage*)pkg);
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		Bdt_SnCallRespPackageFinalize((Bdt_SnCallRespPackage*)pkg);
		break;
	case BDT_SN_CALLED_PACKAGE:
		Bdt_SnCalledPackageFinalize((Bdt_SnCalledPackage*)pkg);
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		Bdt_SnCalledRespPackageFinalize((Bdt_SnCalledRespPackage*)pkg);
		break;
	case BDT_SN_PING_PACKAGE:
		Bdt_SnPingPackageFinalize((Bdt_SnPingPackage*)pkg);
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		Bdt_SnPingRespPackageFinalize((Bdt_SnPingRespPackage*)pkg);
		break;
	case BDT_DATA_GRAM_PACKAGE:
		Bdt_DataGramPackageFinalize((Bdt_DataGramPackage*)pkg);
		break;
	case BDT_SESSION_DATA_PACKAGE:
		Bdt_SessionDataPackageFinalize((Bdt_SessionDataPackage*)pkg);
		break;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		Bdt_TcpSynConnectionPackageFinalize((Bdt_TcpSynConnectionPackage*)pkg);
		break;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		Bdt_TcpAckConnectionPackageFinalize((Bdt_TcpAckConnectionPackage*)pkg);
		break;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		Bdt_TcpAckAckConnectionPackageFinalize((Bdt_TcpAckAckConnectionPackage*)pkg);
		break;
	default:
		assert(false);
	}
}

void Bdt_PackageFree(Bdt_Package* pkg)
{
	switch (pkg->cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		Bdt_ExchangeKeyPackageFree((Bdt_ExchangeKeyPackage*)pkg);
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		Bdt_SynTunnelPackageFree((Bdt_SynTunnelPackage*)pkg);
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		Bdt_AckTunnelPackageFree((Bdt_AckTunnelPackage*)pkg);
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		Bdt_AckAckTunnelPackageFree((Bdt_AckAckTunnelPackage*)pkg);
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		Bdt_PingTunnelPackageFree((Bdt_PingTunnelPackage*)pkg);
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		Bdt_PingTunnelRespPackageFree((Bdt_PingTunnelRespPackage*)pkg);
		break;
	case BDT_SN_CALL_PACKAGE:
		Bdt_SnCallPackageFree((Bdt_SnCallPackage*)pkg);
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		Bdt_SnCallRespPackageFree((Bdt_SnCallRespPackage*)pkg);
		break;
	case BDT_SN_CALLED_PACKAGE:
		Bdt_SnCalledPackageFree((Bdt_SnCalledPackage*)pkg);
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		Bdt_SnCalledRespPackageFree((Bdt_SnCalledRespPackage*)pkg);
		break;
	case BDT_SN_PING_PACKAGE:
		Bdt_SnPingPackageFree((Bdt_SnPingPackage*)pkg);
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		Bdt_SnPingRespPackageFree((Bdt_SnPingRespPackage*)pkg);
		break;
	case BDT_DATA_GRAM_PACKAGE:
		Bdt_DataGramPackageFree((Bdt_DataGramPackage*)pkg);
		break;
	case BDT_SESSION_DATA_PACKAGE:
		Bdt_SessionDataPackageFree((Bdt_SessionDataPackage*)pkg);
		break;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		Bdt_TcpSynConnectionPackageFree((Bdt_TcpSynConnectionPackage*)pkg);
		break;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		Bdt_TcpAckConnectionPackageFree((Bdt_TcpAckConnectionPackage*)pkg);
		break;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		Bdt_TcpAckAckConnectionPackageFree((Bdt_TcpAckAckConnectionPackage*)pkg);
		break;
	default:
		assert(false);
	}
}

bool Bdt_IsSnPackage(const Bdt_Package* package)
{
	return package->cmdtype >= BDT_SN_CALL_PACKAGE && package->cmdtype <= BDT_SN_PING_RESP_PACKAGE;
}
bool Bdt_IsTcpPackage(const Bdt_Package* package)
{
	return package->cmdtype >= BDT_TCP_SYN_CONNECTION_PACKAGE && package->cmdtype <= BDT_TCP_ACKACK_CONNECTION_PACKAGE;
}

struct Bdt_PackageWithRef
{
	int32_t refCount;
	uint8_t package[0];
};

Bdt_PackageWithRef* Bdt_PackageCreateWithRef(uint8_t cmdtype)
{
	Bdt_PackageWithRef* package;
	switch (cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_ExchangeKeyPackage));
		Bdt_ExchangeKeyPackageInit((Bdt_ExchangeKeyPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SynTunnelPackage));
		Bdt_SynTunnelPackageInit((Bdt_SynTunnelPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_AckTunnelPackage));
		Bdt_AckTunnelPackageInit((Bdt_AckTunnelPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_AckAckTunnelPackage));
		Bdt_AckAckTunnelPackageInit((Bdt_AckAckTunnelPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_PingTunnelPackage));
		Bdt_PingTunnelPackageInit((Bdt_PingTunnelPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_PingTunnelRespPackage));
		Bdt_PingTunnelRespPackageInit((Bdt_PingTunnelRespPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_CALL_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnCallPackage));
		Bdt_SnCallPackageInit((Bdt_SnCallPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnCallRespPackage));
		Bdt_SnCallRespPackageInit((Bdt_SnCallRespPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_CALLED_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnCalledPackage));
		Bdt_SnCalledPackageInit((Bdt_SnCalledPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnCalledRespPackage));
		Bdt_SnCalledRespPackageInit((Bdt_SnCalledRespPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_PING_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnPingPackage));
		Bdt_SnPingPackageInit((Bdt_SnPingPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SnPingRespPackage));
		Bdt_SnPingRespPackageInit((Bdt_SnPingRespPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_DATA_GRAM_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_DataGramPackage));
		Bdt_DataGramPackageInit((Bdt_DataGramPackage*)Bdt_PackageWithRefGet(package));
		break;
	case BDT_SESSION_DATA_PACKAGE:
		package = (Bdt_PackageWithRef*)BfxMalloc(sizeof(Bdt_PackageWithRef) + sizeof(Bdt_SessionDataPackage));
		Bdt_SessionDataPackageInit((Bdt_SessionDataPackage*)Bdt_PackageWithRefGet(package));
		break;
	default:
		assert(false);
	}
	package->refCount = 1;
	return package;
}

int32_t Bdt_PackageAddRef(const Bdt_PackageWithRef* packageRef)
{
	Bdt_PackageWithRef* p = (Bdt_PackageWithRef*)packageRef;
	return BfxAtomicInc32(&p->refCount);
}

int32_t Bdt_PackageRelease(const Bdt_PackageWithRef* packageRef)
{
	Bdt_PackageWithRef* p = (Bdt_PackageWithRef*)packageRef;
	int32_t refCount = BfxAtomicDec32(&p->refCount);
	if (refCount <= 0)
	{
		Bdt_PackageFinalize(Bdt_PackageWithRefGet(p));
		BfxFree(p);
	}
	return refCount;
}

Bdt_Package* Bdt_PackageWithRefGet(const Bdt_PackageWithRef* packageRef)
{
	return (Bdt_Package*)packageRef->package;
}

size_t BdtProtocol_ConnectionFirstQuestionMaxLength()
{
	//todo: 
	return 100;
}

size_t BdtProtocol_ConnectionFirstResponseMaxLength()
{
	//todo:
	return 100;
}
