#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

//
// PackageParser.c
//
 
#include "./PackageDecoder.h"
#include "./PackageDecoder.inl"

#include <assert.h>
#include <BuckyBase/Buffer/Buffer.h>
#include "../Global/Module.h"


static int DecodeDataGramPackage(Bdt_DataGramPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_DATA_GRAM_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;

	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}

	bool haveField_seq = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read seq.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_SEQ, pRefPackage, &(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field seq read from refPackage.");
			}
		}
	}
	bool haveField_destZone = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE;
	if (haveField_destZone)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->destZone));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read destZone.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE, pRefPackage, &(pResult->destZone));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field destZone read from refPackage.");
			}
		}
	}
	bool haveField_hopLimit = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT;
	if (haveField_hopLimit)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->hopLimit));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read hopLimit.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT, pRefPackage, &(pResult->hopLimit));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field hopLimit read from refPackage.");
			}
		}
	}
	bool haveField_piece = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_PIECE;
	if (haveField_piece)
	{
		r = BfxBufferReadUInt16(bufferStream, &(pResult->piece));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read piece.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_PIECE, pRefPackage, &(pResult->piece));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field piece read from refPackage.");
			}
		}
	}
	bool haveField_ackSeq = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ;
	if (haveField_ackSeq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->ackSeq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read ackSeq.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ, pRefPackage, &(pResult->ackSeq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field ackSeq read from refPackage.");
			}
		}
	}
	bool haveField_sendTime = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendTime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read sendTime.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME, pRefPackage, &(pResult->sendTime));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field sendTime read from refPackage.");
			}
		}
	}
	bool haveField_authorPeerid = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID;
	if (haveField_authorPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->authorPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read authorPeerid.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID, pRefPackage, &(pResult->authorPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field authorPeerid read from refPackage.");
			}
		}
	}
	bool haveField_authorPeerInfo = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO;
	if (haveField_authorPeerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->authorPeerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read authorPeerInfo.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO, pRefPackage, (void*)&(pResult->authorPeerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field authorPeerInfo read from refPackage.");
			}
		}
	}
	bool haveField_dataSign = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN;
	if (haveField_dataSign)
	{
		r = 0;
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read dataSign.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN, pRefPackage, &(pResult->dataSign));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field dataSign read from refPackage.");
			}
		}
	}
	bool haveField_innerCmdType = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE;
	if (haveField_innerCmdType)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->innerCmdType));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read innerCmdType.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE, pRefPackage, &(pResult->innerCmdType));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PACKAGE_DATA_GRAM:field innerCmdType read from refPackage.");
			}
		}
	}



	bool haveField_data = cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DATA;
	if (haveField_data)
	{
		uint16_t datalen = totallen - (uint16_t)readlen;//剩下的都是
		assert(datalen > 0);
		if (datalen <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_DATA_GRAM error,cann't read data.");
			r = -1;
			goto END;
		}

		pResult->data = BfxCreateBindBuffer(bufferStream->buffer + bufferStream->pos, datalen);
		size_t thisPos = BfxBufferStreamGetPos(bufferStream);
		BfxBufferStreamSetPos(bufferStream, thisPos + datalen);
		readlen += datalen;
	}

	
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decodeBDT_PACKAGE_DATA_GRAM error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//--------------------------------------------------------------------------------------------------------
static int DecodeExchangeKeyPackage(Bdt_ExchangeKeyPackage* pPackage, BfxBufferStream* bufferStream, const Bdt_Package* pRefPackage)
{
	int r = 0;
	BdtPeerConstInfo* pInfo = NULL;
	size_t minsize = Bdt_GetPackageMinSize(BDT_EXCHANGEKEY_PACKAGE);
	uint16_t totallen = 0;
	uint16_t cmdflags;
	int readlen = 0;
	r = BfxBufferReadUInt16(bufferStream, &totallen);
	if (r <= 0)
	{
		BLOG_WARN("decode exchange package error,cann't read totallen.");
		goto END;
	}
	if (BfxBufferStreamGetTailLength(bufferStream) < totallen)
	{
		BLOG_WARN("decode exchange package error, package length(%zd) < package.totallen(%d).", BfxBufferStreamGetTailLength(bufferStream), totallen);
		goto END;
	}
	if (totallen < minsize)
	{
		BLOG_WARN("decode exchange package error,totallen(%hu) too small.",totallen);
		goto END;
	}
	readlen += r;

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode exchange package error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pPackage->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}

	r = BfxBufferReadUInt32(bufferStream, &(pPackage->seq));
	if (r <= 0)
	{
		BLOG_WARN("decode exchange package error,cann't read seq.");
		goto END;
	}
	readlen += r;

	r = BfxBufferReadByteArray(bufferStream, pPackage->seqAndKeySign, BDT_SEQ_AND_KEY_SIGN_LENGTH);
	if (r <= 0)
	{
		BLOG_WARN("decode exchange package error,cann't read seqAndKeySign.");
		goto END;
	}
	readlen += r;

	//fromPeerid
	bool haveField_fromPeerid = cmdflags & BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pPackage->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode exchange package error,cann't read fromPeerid.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pPackage->cmdtype, BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID, pRefPackage, (void*) & (pPackage->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode exchange package:field fromPeerid read from refPackage.");
			}
		}
	}


	//peerInfo
	bool haveField_peerInfo = cmdflags & BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pPackage->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode exchange package error,cann't read peerInfo.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pPackage->cmdtype, BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO, pRefPackage, (void*) & (pPackage->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode exchange package:field peerInfo read from refPackage.");
			}
		}
	}

	r = BfxBufferReadUInt64(bufferStream, &(pPackage->sendTime));
	if (r <= 0)
	{
		BLOG_WARN("decode exchange package error,cann't read sendTime.");
		goto END;
	}
	readlen += r;

	if (totallen != readlen)
	{
		r = 0;
		BLOG_WARN("decode exchange package error,totallen != readlen.");
		goto END;
	}

END:
	if (r <= 0)
	{
		return r;
	}

	return readlen;
	
}

int DecodeSessionDataPackage(Bdt_SessionDataPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SESSION_DATA_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;

	//sessionId;
	r = BfxBufferReadUInt32(bufferStream, &(pResult->sessionId));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read sessionId.");
		r = -1;
		goto END;
	}
	readlen += r;
	
	//streamPos;
	r = BfxBufferReadUInt48(bufferStream, &(pResult->streamPos));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read streamPos.");
		r = -1;
		goto END;
	}
	readlen += r;

	bool haveField_synPart = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN;
	if (haveField_synPart)
	{
		if (pResult->synPart == NULL)
		{
			pResult->synPart = BFX_MALLOC_OBJ(Bdt_SessionDataSynPart);
		}
		r = BfxBufferReadUInt32(bufferStream, &(pResult->synPart->synSeq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read synSeq.");
			r = -1;
			goto END;
		}
		readlen += r;

		r = BfxBufferReadUInt8(bufferStream, &(pResult->synPart->synControl));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read synControl.");
			r = -1;
			goto END;
		}
		readlen += r;

		r = BfxBufferReadUInt8(bufferStream, &(pResult->synPart->ccType));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read ccType.");
			r = -1;
			goto END;
		}
		readlen += r;

		r = BfxBufferReadUInt16(bufferStream, &(pResult->synPart->toVPort));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read toVPort.");
			r = -1;
			goto END;
		}
		readlen += r;

		r = BfxBufferReadUInt32(bufferStream, &(pResult->synPart->fromSessionId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read fromSessionId.");
			r = -1;
			goto END;
		}
		readlen += r;
	}

	bool haveField_packageId = (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_PACKAGEID) || (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK_PACKAGEID);
	if (haveField_packageId)
	{
		if (pResult->packageIDPart == NULL)
		{
			pResult->packageIDPart = BFX_MALLOC_OBJ(Bdt_SessionDataPackageIdPart);
		}
		r = BfxBufferReadUInt32(bufferStream, &(pResult->packageIDPart->packageId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read packageId.");
			r = -1;
			goto END;
		}
		readlen += r;

		r = BfxBufferReadUInt48(bufferStream, &(pResult->packageIDPart->totalRecv));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read totalRecv.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	
	
	bool haveField_ackStreamPos = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK;
	if (haveField_ackStreamPos)
	{
		r = BfxBufferReadUInt48(bufferStream, &(pResult->ackStreamPos));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read ackStreamPos.");
			r = -1;
			goto END;
		}
		readlen += r;
	}

	bool haveField_sackArray = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SACK;
	if (haveField_sackArray)
	{
		uint8_t count;
		r = BfxBufferReadUInt8(bufferStream, &count);
		if(r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read sack count.");
			r = -1;
			goto END;
		}
		readlen += r;
		if (count > PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT || count == 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,sack array count error.");
			r = -1;
			goto END;
		}
		//TODO:这里还是先用动态分配
		pResult->sackArray = BFX_MALLOC_ARRAY(BdtStreamRange, (count + 1));
		memset(pResult->sackArray, 0, sizeof(BdtStreamRange) * (count + 1));

		BdtStreamRange* pRange = pResult->sackArray;
		for (int i = 0; i < count; ++i)
		{
			r = BfxBufferReadUInt48(bufferStream, &(pRange->pos));
			if (r <= 0)
			{
				BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read sack pos.");
				r = -1;
				goto END;
			}
			readlen += r;
			r = BfxBufferReadUInt32(bufferStream, &(pRange->length));
			if (r <= 0)
			{
				BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read sack length.");
				r = -1;
				goto END;
			}
			readlen += r;
			pRange ++;
		}

	}

	bool haveField_toSessionId = (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN) && (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK);
	if (haveField_toSessionId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toSessionId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read toSessionId.");
			r = -1;
			goto END;
		}
		readlen += r;
	}

	
	bool haveField_recvSpeedlimit = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SPEEDLIMIT;
	if (haveField_recvSpeedlimit)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->recvSpeedlimit));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read recvSpeedlimit.");
			r = -1;
			goto END;
		}
		readlen += r;
	}
	
	bool haveField_sendtime = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendtime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read sendtime.");
			r = -1;
			goto END;
		}
		readlen += r;
	}

	bool have_Payload = cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_PAYLOAD;
	if (have_Payload)
	{
		uint16_t datalen = totallen - (uint16_t)readlen;//剩下的都是paylaod
		assert(datalen > 0);
		if (datalen <= 0)
		{
			BLOG_WARN("decode BDT_PACKAGE_SESSION_DATA error,cann't read payload.");
			r = -1;
			goto END;
		}
		

		pResult->payload = bufferStream->buffer + bufferStream->pos;
		pResult->payloadLength = datalen;
		size_t thisPos = BfxBufferStreamGetPos(bufferStream);
		BfxBufferStreamSetPos(bufferStream, thisPos + datalen);
		readlen += datalen;
	}
	


	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decodeBDT_PACKAGE_SESSION_DATA error,totallen != readlen.");
		r = -1;
	}
	return r;
}

int Bdt_DecodePackage(
	const uint8_t* buffer, 
	size_t length, 
	const Bdt_Package* refPackage, 
	Bdt_Package** result, 
	bool isStartWithExchangeKey)
{
	int result_pos = 0;
	int r = 0;

	BfxBufferStream bufferStream;
	BfxBufferStreamInit(&bufferStream, (uint8_t*)buffer, length);
	result[0] = NULL;
	if (isStartWithExchangeKey) {
		result[0] = (Bdt_Package*)Bdt_ExchangeKeyPackageCreate();
		r = DecodeExchangeKeyPackage((Bdt_ExchangeKeyPackage*)result[0], &bufferStream,refPackage);
		if (r <= 0)
		{
			r = BFX_RESULT_INVALID_ENCODE;
			goto END;
		}
		result_pos ++;
	}
	
	r = 0;
	while (BfxBufferStreamGetTailLength(&bufferStream) > 3)
	{
		uint16_t totallen;
		uint8_t cmdtype;

		BfxBufferReadUInt16(&bufferStream, &totallen);
		BfxBufferReadUInt8(&bufferStream, &cmdtype);
		uint16_t HEAD_LEN = 3;
		if (totallen < Bdt_GetPackageMinSize(cmdtype) || (totallen - 3) > BfxBufferStreamGetTailLength(&bufferStream))
		{
			BLOG_WARN("totallen error. decode package error");
			r = BFX_RESULT_INVALID_ENCODE;
			goto END;
		}


		Bdt_Package* pThisResult = DecodeSinglePackage(
			cmdtype, 
			(uint16_t)(totallen-HEAD_LEN), 
			&bufferStream, 
			refPackage ? (Bdt_Package*)refPackage : result[0]);//只会引用第一个包
		if (pThisResult == NULL)
		{
			r = BFX_RESULT_INVALID_ENCODE;
			goto END;
		}

		result[result_pos] = pThisResult;
		result_pos ++;
	}


END:
	return r;
}
