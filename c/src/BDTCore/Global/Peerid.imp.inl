#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "../BdtCore.h"
#include "./CryptoToolKit.h"


BDT_API(int32_t) BdtPeeridCompare(const BdtPeerid* left, const BdtPeerid* right)
{
	return memcmp(left, right, BDT_PEERID_LENGTH);
}

BDT_API(bool) BdtPeeridIsGroup(const BdtPeerid* peerid)
{
	//TODO:
	return false;
}

BDT_API(uint32_t) BdtCreatePeerid(
	const BdtPeerArea* areaCode,
	const char* deviceId,
	uint8_t publicKeyType,
	BdtPeerid* outPeerid,
	BdtPeerConstInfo* outConstInfo,
	BdtPeerSecret* outSecret
)
{
	memset(outConstInfo, 0, sizeof(BdtPeerConstInfo));
	outConstInfo->areaCode = *areaCode;
	memcpy(outConstInfo->deviceId, deviceId, strlen(deviceId));
	outConstInfo->publicKeyType = publicKeyType;

    size_t publicLen = BdtGetPublicKeyLength(publicKeyType), secretLen = BdtGetSecretKeyMaxLength(publicKeyType);
    uint32_t secretSize = BdtGetKeyBitLength(publicKeyType);
	// 生成rsa对钥
	int ret = Bdt_RsaGenerateToBuffer(secretSize, (uint8_t*)outConstInfo->publicKey.rsa1024, &publicLen, (uint8_t*)outSecret, &secretLen);
	if (ret) {
		return ret;
	}

    outSecret->secretLength = (uint16_t)secretLen;

	return BFX_RESULT_SUCCESS;
}

static void _encode(const BdtPeerArea* areaCode, uint8_t* hash, uint8_t* peerid) {
	// 区域编码的长度为32bits + 8bits(机构内部编码）
	// 大区编码(6bits)+(国家编码8bits)+(运营商编码4bits)+城市编码(14bits)  
	// 第一个byte放continent的全6bit和country的前2bit
	// 第二个byte放country的后6bit和carrier的前2bit
	// 第三个byte放carrier的后2bit和city的前6bit
	// 第四个byte放city的后8bit
	// 第五个byte放inner的全8bit
	peerid[0] = areaCode->continent << 2 | areaCode->country >> 6;
	peerid[1] = areaCode->country << 2 | areaCode->carrier >> 2;
	peerid[2] = areaCode->carrier << 6 | areaCode->city >> 8;
	peerid[3] = areaCode->city << 8 >> 8;
	peerid[4] = areaCode->inner;

	for (int i = 0; i < 27; i++) {
		// 从第5个byte开始是hash部分
		peerid[5 + i] = hash[i];
	}
}

BDT_API(uint32_t) BdtGetPeeridFromConstInfo(const BdtPeerConstInfo* constInfo, BdtPeerid* outPeerid)
{
	uint8_t hash[32];
	Bdt_Sha256Hash((const uint8_t*)&constInfo->publicKey, BdtGetPublicKeyLength(constInfo->publicKeyType), hash);
	_encode(&(constInfo->areaCode), hash, (uint8_t*)outPeerid);

	return BFX_RESULT_SUCCESS;
}

BDT_API(uint32_t) BdtPeeridVerifyConstInfo(const BdtPeerid* peerid, const BdtPeerConstInfo* constInfo)
{

	BdtPeerid pid;
	BdtGetPeeridFromConstInfo(constInfo, &pid);
	if (!BdtPeeridCompare(peerid, &pid))
	{
		return BFX_RESULT_SUCCESS;
	}
	return BFX_RESULT_INVALID_PARAM;
}


BDT_API(void) BdtPeeridToString(const BdtPeerid* peerid, char out[BDT_PEERID_STRING_LENGTH])
{
	for (size_t ix = 0; ix < BDT_PEERID_LENGTH; ++ix)
	{
		sprintf(out + ix * 2, "%02x", *((uint8_t*)peerid + ix));
	}
}

