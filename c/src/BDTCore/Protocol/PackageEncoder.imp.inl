#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

#include "./PackageEncoder.inl"

//-------------------------------------------------------------------------------------------------------------------
static int EncodeExchangeKeyPackage(Bdt_Package* package, const Bdt_Package* refPackage,BfxBufferStream* pStream, size_t* pWriteBytes)
{
	//assert(refPackage == NULL);
	Bdt_ExchangeKeyPackage* pPackage = (Bdt_ExchangeKeyPackage*)package;
	size_t writeBytes = 0;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;
	bool haveRef = false;
	bool isEqual = false;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	//totallen最后写 totallen,cmdflags
	BfxBufferStreamSetPos(pStream, headerPos + 2 + 2);
	totalWriteBytes += 4;



	//写入数据
	r = BfxBufferWriteUInt32(pStream, pPackage->seq,&writeBytes);
	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWriteBytes += writeBytes;
	writeBytes = 0;

	r = BfxBufferWriteByteArray(pStream, pPackage->seqAndKeySign, BDT_SEQ_AND_KEY_SIGN_LENGTH);
	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWriteBytes += BDT_SEQ_AND_KEY_SIGN_LENGTH;
	writeBytes = 0;

	//fromPeerid
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) == 0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}


	//peerinfo
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo, *rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}


	r = BfxBufferWriteInt64(pStream, pPackage->sendTime, &writeBytes);
	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWriteBytes += writeBytes;
	writeBytes = 0;

	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags, &writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;

	return BFX_RESULT_SUCCESS;
}

/*

-------------------
PACKAGE_FLAG_DISABLE_REF打开，则无refpackage.

有refpackage
   有字段且和refpackage相等：flags=0
   有字段但和refpackage不相等：flags=1
   无字段相当于无refpackage

无ref
   和default相等：flags =0


大部分default value的含义为“该值为NULL”
*/
//------------------------------------------------------------------------------

static int EncodeDataGramPackage( Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_DataGramPackage* pPackage = (Bdt_DataGramPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	//cmdtype,totallen,cmdflags鏈€鍚庡啓
	BfxBufferStreamSetPos(pStream, headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_SEQ)
	{
		//鍐欏叆鏁版嵁
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE);
		if (fieldName)
		{
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->destZone == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE)
	{
		r = BfxBufferWriteUInt32(pStream, (pPackage->destZone), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT);
		if (fieldName)
		{
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->hopLimit == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT)
	{
		r = BfxBufferWriteUInt8(pStream, (pPackage->hopLimit), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_PIECE);
		if (fieldName)
		{
			uint16_t* rParam = (uint16_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->piece == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_PIECE);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_PIECE);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_PIECE))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_PIECE);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_PIECE);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_PIECE)
	{

		r = BfxBufferWriteUInt16(pStream, (pPackage->piece), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ);
		if (fieldName)
		{
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->ackSeq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ)
	{
		r = BfxBufferWriteUInt32(pStream, (pPackage->ackSeq), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME);
		if (fieldName)
		{
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->sendTime == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME)
	{

		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID);
		if (fieldName)
		{
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->authorPeerid.pid, BDT_PEERID_LENGTH) == 0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID)
	{

		r = Bdt_BufferWritePeerid(pStream, &(pPackage->authorPeerid), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO);
		if (fieldName)
		{
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->authorPeerInfo == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO)
	{
	
		r = BufferWritePeerInfo(pStream, (pPackage->authorPeerInfo), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN);
		if (fieldName)
		{
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				//isEqual = undefined;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN)
	{
	

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------

	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype, BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE);
		if (fieldName)
		{
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if (rParam)
			{
				haveRef = true;
				isEqual = (pPackage->innerCmdType == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE);
				}
			}
		}
	}

	if (!haveRef)
	{
		if (IsEqualPackageFieldDefaultValue(package, BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE);
		}
	}

	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE)
	{

		r = BfxBufferWriteUInt8(pStream, (pPackage->innerCmdType), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	if (pPackage->data == NULL)
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_DATA_GRAM_PACKAGE_FLAG_DATA);
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags | (BDT_DATA_GRAM_PACKAGE_FLAG_DATA);
	}


	if (pPackage->cmdflags & BDT_DATA_GRAM_PACKAGE_FLAG_DATA)
	{
		uint8_t* pData;
		size_t datalen;
		pData = BfxBufferGetData(pPackage->data,&datalen);
		r = BfxBufferWriteByteArray(pStream, pData, datalen);
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += datalen;
	}
	//-------------------------------------------------------------

	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes, &writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype, &writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags, &writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;

	return BFX_RESULT_SUCCESS;
}

static int EncodeSessionDataPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SessionDataPackage* pPackage = (Bdt_SessionDataPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);

	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream, headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;


	//sessionId
	r = BfxBufferWriteUInt32(pStream, (pPackage->sessionId), &writeBytes);

	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWriteBytes += writeBytes;
	writeBytes = 0;

	//streamPos
	r = BfxBufferWriteUInt48(pStream, (pPackage->streamPos), &writeBytes);

	if (r != BFX_RESULT_SUCCESS)
	{
		return r;
	}
	totalWriteBytes += writeBytes;
	writeBytes = 0;

	//-------------------------------------------------------------
	if (pPackage->synPart)
	{
		pPackage->cmdflags = pPackage->cmdflags | (BDT_SESSIONDATA_PACKAGE_FLAG_SYN);

		//write synPart.synSeq
		r = BfxBufferWriteUInt32(pStream, (pPackage->synPart->synSeq), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;

		//write synPart.synControl
		r = BfxBufferWriteUInt8(pStream, (pPackage->synPart->synControl), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;

		//write to synPart.ccType
		r = BfxBufferWriteUInt8(pStream, (pPackage->synPart->ccType), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;

		//write to synPart.toVPort
		r = BfxBufferWriteUInt16(pStream, (pPackage->synPart->toVPort), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;

		//write to synPart.fromSessionId
		r = BfxBufferWriteUInt32(pStream, (pPackage->synPart->fromSessionId), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_SYN);
	}


	if (pPackage->packageIDPart)
	{
		pPackage->cmdflags = pPackage->cmdflags | (BDT_SESSIONDATA_PACKAGE_FLAG_PACKAGEID);
		r = BfxBufferWriteUInt32(pStream, (pPackage->packageIDPart->packageId), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;


		r = BfxBufferWriteUInt48(pStream, (pPackage->packageIDPart->totalRecv), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_PACKAGEID);
	}

	//ackStreamPos and sack array
	if (pPackage->ackStreamPos == 0 )
	{
		//pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_ACK);
		
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags | BDT_SESSIONDATA_PACKAGE_FLAG_ACK;
	}
	if (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK)
	{
		//write to buffer
		r = BfxBufferWriteUInt48(pStream, (pPackage->ackStreamPos), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}

	//toSessionId
	if ((pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN) && (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK))
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toSessionId), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}

	//write sack;
	if (pPackage->sackArray)
	{
		pPackage->cmdflags = pPackage->cmdflags | BDT_SESSIONDATA_PACKAGE_FLAG_SACK;
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_SACK);
	}
	if (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SACK)
	{
		size_t countpos = BfxBufferStreamGetPos(pStream);
		BfxBufferStreamSetPos(pStream,countpos + 1);

		BdtStreamRange* pRange = pPackage->sackArray;
		size_t count = 0;
		while (pRange->length)
		{
			r = BfxBufferWriteUInt48(pStream,pRange->pos,&writeBytes);
			if (r != BFX_RESULT_SUCCESS)
			{
				return r;
			}
			totalWriteBytes += writeBytes;
			writeBytes = 0;
			r = BfxBufferWriteUInt32(pStream,pRange->length,&writeBytes);
			if (r != BFX_RESULT_SUCCESS)
			{
				return r;
			}
			totalWriteBytes += writeBytes;
			writeBytes = 0;
			count++;
			pRange++;
			if (count > PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT)
			{
				BLOG_ERROR("SACK max count is %d", PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT);
				return BFX_RESULT_INVALID_FORMAT;
			}
		}
		
		size_t endpos = BfxBufferStreamGetPos(pStream);
		BfxBufferStreamSetPos(pStream, countpos);
		BfxBufferWriteUInt8(pStream, (uint8_t)count, &writeBytes);
		totalWriteBytes += writeBytes;
		writeBytes = 0;
		BfxBufferStreamSetPos(pStream,endpos);
	}

	//recvSpeedlimit
	if (pPackage->recvSpeedlimit == 0)
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_SPEEDLIMIT);
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags | BDT_SESSIONDATA_PACKAGE_FLAG_SPEEDLIMIT;
	}

	if (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SPEEDLIMIT)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->recvSpeedlimit), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}

	//sendtime
	if (pPackage->sendTime == 0)
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_SENDTIME);
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags | BDT_SESSIONDATA_PACKAGE_FLAG_SENDTIME;
	}


	if (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SENDTIME)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime), &writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}

	//TODO:payload 这里可能会通过专用函数来提升encode性能。先实现一个正确的版本
	if (pPackage->payload == NULL)
	{
		pPackage->cmdflags = pPackage->cmdflags & (~BDT_SESSIONDATA_PACKAGE_FLAG_PAYLOAD);
	}
	else
	{
		pPackage->cmdflags = pPackage->cmdflags | BDT_SESSIONDATA_PACKAGE_FLAG_PAYLOAD;
	}
	if (pPackage->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_PAYLOAD)
	{
		r = BfxBufferWriteByteArray(pStream, pPackage->payload, pPackage->payloadLength);
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += pPackage->payloadLength;
	}


	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes, &writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype, &writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags, &writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;

	return BFX_RESULT_SUCCESS;
}


int Bdt_EncodePackage(
	const Bdt_Package** packages, 
	size_t packageCount, 
	const Bdt_Package* refPackage, 
	uint8_t* buffer, 
	size_t length, 
	size_t* pWriteBytes)
{
	*pWriteBytes = 0;
	int r = 0;
	BfxBufferStream stream;
	BfxBufferStreamInit(&stream, buffer, length);
	size_t totalwrite = 0;

	for (size_t i = 0; i < packageCount; ++i)
	{
		// todo: const 
		Bdt_Package* pPackage = (Bdt_Package*)packages[i];
		const Bdt_Package* pRefPackage = refPackage ? refPackage : ( i ? packages[0] : NULL);
		size_t thisWrite = 0;
		if (pPackage->cmdtype == BDT_EXCHANGEKEY_PACKAGE && i != 0)
		{
			BLOG_WARN("Encode Package:BDT_PACKAGE_EXCHANGEKEY must be first package.");
			return BFX_RESULT_INVALID_PARAM;
		}

		r = Bdt_EncodeSinglePackage(pPackage, pRefPackage,&stream, &thisWrite);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalwrite += thisWrite;
	}

	*pWriteBytes = totalwrite;
	return BFX_RESULT_SUCCESS;

}