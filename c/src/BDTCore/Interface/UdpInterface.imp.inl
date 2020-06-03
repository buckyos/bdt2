#ifndef __BDT_UDP_INTERFACE_IMP_INL__
#define __BDT_UDP_INTERFACE_IMP_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__

#include "./UdpInterface.inl"
#include <assert.h>
#include "../Stack.h"
#include "../Protocol/PackageDecoder.h"
#include "../Protocol/PackageEncoder.h"

int32_t Bdt_UdpInterfaceAddRef(const UdpInterface* ui)
{
	UdpInterface* i = (UdpInterface*)ui;
	return BfxAtomicInc32(&i->refCount);
}

int32_t Bdt_UdpInterfaceRelease(const UdpInterface* ui)
{
	UdpInterface* i = (UdpInterface*)ui;
	int32_t ref = BfxAtomicDec32(&i->refCount);
	if (ref <= 0)
	{
		BdtDestroyUdpSocket(i->framework, i->socket);
		BFX_FREE(i);
	}
	return ref;
}

BDT_SYSTEM_UDP_SOCKET Bdt_UdpInterfaceGetSocket(const UdpInterface* ui)
{
	return ui->socket;
}

const BdtEndpoint* Bdt_UdpInterfaceGetLocal(const UdpInterface* ui)
{
	return &ui->localEndpoint;
}


uint32_t Bdt_BoxUdpPackage(
	const Bdt_Package** packages,
	size_t count, 
	const Bdt_Package* refPackage, 
	uint8_t* buffer,
	size_t* inoutLength
)
{
	size_t outLen = 0;
	uint32_t result = Bdt_EncodePackage(packages, count, refPackage, buffer + 2, *inoutLength - 2, &outLen);
	if (result)
	{
		return result;
	}
	buffer[0] = BDT_PROTOCOL_UDP_MAGIC_NUM_0;
	buffer[1] = BDT_PROTOCOL_UDP_MAGIC_NUM_1;
	*inoutLength = outLen + 2;
	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_BoxEncryptedUdpPackage(
	const Bdt_Package** packages,
	size_t count, 
	const Bdt_Package* refPackage,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	uint8_t* buffer,
	size_t* inoutLength
)
{
    // 前边填充key hash
	BdtHistory_KeyManagerCalcKeyHash(aesKey, buffer);

	size_t dataLen = 0;
    size_t remainLen = *inoutLength - BDT_MIX_HASH_LENGTH;
	uint32_t result = Bdt_EncodePackage(packages, count, refPackage, buffer + BDT_MIX_HASH_LENGTH, remainLen, &dataLen);
	if (result)
	{
		return result;
	}

    int targetLen = Bdt_AesEncryptDataWithPadding(aesKey, buffer + BDT_MIX_HASH_LENGTH, dataLen, remainLen, buffer + BDT_MIX_HASH_LENGTH, remainLen);
    if (targetLen < 0)
    {
        return BFX_RESULT_FAILED;
    }
    
    *inoutLength = (size_t)targetLen + BDT_MIX_HASH_LENGTH;
	return BFX_RESULT_SUCCESS;
}

uint32_t Bdt_BoxEncryptedUdpPackageStartWithExchange(
	const Bdt_Package** packages,
	size_t count, 
	const Bdt_Package* refPackage, 
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
    uint8_t remotePublicType, 
	const uint8_t* remotePublic,
	uint16_t secretLength, 
    const uint8_t* secret,
	uint8_t* buffer,
	size_t* inoutLength
)
{
    size_t olen = 0;
    int ret = Bdt_RsaEncrypt(
		aesKey, 
		BDT_AES_KEY_LENGTH, 
		remotePublic, 
		BdtGetPublicKeyLength(remotePublicType), 
		buffer, 
		&olen, 
		*inoutLength);
    if (ret)
    {
        return ret;
    }
	size_t used = olen;
	//write key hash:
	Bdt_HashAesKey(aesKey, buffer + used);
	used += 8;
	size_t headlen = used;

    // 第一个包一定是ExchangeKeyPackage, 这里用secret签名
    Bdt_ExchangeKeyPackage* pkg = (Bdt_ExchangeKeyPackage*)packages[0];
    uint8_t buf4sign[sizeof(pkg->seq) + BDT_AES_KEY_LENGTH];
    memcpy(buf4sign, (const uint8_t *)(&(pkg->seq)), sizeof(pkg->seq));
    memcpy(buf4sign + sizeof(pkg->seq), aesKey, BDT_AES_KEY_LENGTH);

    uint8_t sign[BDT_MAX_SIGN_LENGTH];
    size_t signlen = BDT_MAX_SIGN_LENGTH;
    ret = Bdt_RsaSignMd5(buf4sign, sizeof(buf4sign), secret, secretLength, sign, &signlen);
    if (ret)
    {
        return ret;
    }

    memcpy(pkg->seqAndKeySign, sign, sizeof(pkg->seqAndKeySign));
    
    size_t outLen = 0;
    size_t remainLen = *inoutLength - headlen;
    uint32_t result = Bdt_EncodePackage(packages, count, refPackage, buffer + used, remainLen, &outLen);
    if (result)
    {
        return result;
    }

    int targetLen = Bdt_AesEncryptDataWithPadding(aesKey, buffer + headlen, outLen, remainLen, buffer + headlen, remainLen);
    if (targetLen < 0)
    {
        return BFX_RESULT_FAILED;
    }
    
    *inoutLength = headlen + targetLen;
	return BFX_RESULT_SUCCESS;
}

static bool UnboxUdpPackage(
	const uint8_t* data, 
	size_t bytes, 
	const Bdt_Package* refPackage, 
	Bdt_Package** resultPackages)
{
	if (data[0] == BDT_PROTOCOL_UDP_MAGIC_NUM_0 && data[1] == BDT_PROTOCOL_UDP_MAGIC_NUM_1)
	{
		int r = Bdt_DecodePackage(
			data + BDT_PROTOCOL_UDP_MAGIC_LENGTH,
			bytes - BDT_PROTOCOL_UDP_MAGIC_LENGTH, 
			refPackage, 
			resultPackages, 
			false);

		if (r == 0)
		{
			return true;
		}
	}
	return false;
}

static bool UnboxEncryptedUdpPackage(
	BdtStack* stack, 
	const uint8_t* data, 
	size_t bytes, 
	uint8_t* aesKey,
	bool isStartWithExchangeKey,
	const uint8_t* secret,
	size_t secretlen, 
	const Bdt_Package* refPackage, 
	Bdt_Package** resultPackages)
{
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	int r = 0;
	size_t headerPos = BDT_MIX_HASH_LENGTH;
	if (isStartWithExchangeKey)
	{
		assert(secret);
		// 解出AESKey
		size_t olen = 0;
		size_t inLen = 0;
		r = Bdt_RsaDecrypt(data, secret, secretlen, aesKey, &olen, BDT_AES_KEY_LENGTH, &inLen);
		if (r == 0)
		{
			headerPos = inLen;
		}
		else
		{
			BLOG_WARN("process udp package:rsa decrypto aeskey failed!\n");
			return false;
		}

		//先读keyhash
		const uint8_t* keyHash = data + inLen;
		headerPos += 8;
		//验证keyhash.
		if (!Bdt_CheckAesKeyHash(aesKey, keyHash))
		{
			BLOG_WARN("check aes key hash failed.");
			return false;
		}
	}

	int datalen = Bdt_AesDecryptDataWithPadding(
		aesKey,
		data + headerPos,
		bytes - headerPos,
		tlsData->udpDecryptBuffer,
		sizeof(tlsData->udpDecryptBuffer));

	if (datalen > 0)
	{
		r = Bdt_DecodePackage(
			tlsData->udpDecryptBuffer,
			datalen,
			refPackage, 
			resultPackages,
			isStartWithExchangeKey);

		if (r == 0)
		{
			return true;
		}
		else
		{
			BLOG_WARN("unbox udp package failed!:decrypto data ok but decode pakcage error!");
			return false;
		}
	}
	else
	{
		BLOG_WARN("unbox udp package failed!:decrypto data error!");
		return false;
	}

	return false;
}

uint32_t Bdt_UnboxUdpPackage(
	BdtStack* stack, 
	const uint8_t* data, 
	size_t bytes, 
	Bdt_Package** packages, 
	size_t count, 
	const Bdt_Package* refPackage, 
	uint8_t outKey[BDT_AES_KEY_LENGTH], 
	BdtPeerid* outKeyPeerid,
	bool* outIsEncrypto,
	bool* outHasExchange)
{
	assert(count >= BDT_MAX_PACKAGE_MERGE_LENGTH);
	*outIsEncrypto = true;
	*outHasExchange = false;
	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(BdtStackGetTls(stack));
	//todo: do independent verify every package
	if (bytes < BDT_PROTOCOL_MIN_PACKAGE_LENGTH)
	{
		BLOG_WARN("process udp package: too small!\n");
		return BFX_RESULT_FAILED;
	}
	if (UnboxUdpPackage(data, bytes, refPackage, packages))
	{
		*outIsEncrypto = false;
		return BFX_RESULT_SUCCESS;
	}

	//read mix hash
	if (bytes < BDT_PROTOCOL_MIN_CRYPTO_PACKAGE_LENGTH)
	{
		return BFX_RESULT_FAILED;
	}
	if (!BdtHistory_KeyManagerFindByMixHash(BdtStackGetKeyManager(stack), data, outKey, outKeyPeerid, true, true))
	{
		if (UnboxEncryptedUdpPackage(stack, data, bytes, outKey, false, NULL, 0, refPackage, packages))
		{
			return BFX_RESULT_SUCCESS;
		}
	}
	
	// 这是exchange key package
	const BdtPeerSecret* secret = BdtStackGetSecret(stack);
	if (UnboxEncryptedUdpPackage(stack, data, bytes, outKey, true, (const uint8_t*)secret, secret->secretLength, refPackage, packages))
	{
		Bdt_ExchangeKeyPackage* pExchangeKey = (Bdt_ExchangeKeyPackage*)packages[0];  
		if (packages[0]->cmdtype == BDT_EXCHANGEKEY_PACKAGE)
		{
			// TODO: 检查一下seqkeysign
			//将key加入keymananger,TODO:这里key在栈上
			BdtHistory_KeyManagerAddKey(BdtStackGetKeyManager(stack), outKey, BdtPeerInfoGetPeerid(pExchangeKey->peerInfo), true);
			*outKeyPeerid = *BdtPeerInfoGetPeerid(pExchangeKey->peerInfo);
			*outHasExchange = true;
			return BFX_RESULT_SUCCESS;
		}
		else
		{
			return BFX_RESULT_FAILED;
		}
	}
	return BFX_RESULT_FAILED;
}

BDT_API(void) BdtPushUdpSocketData(
	BdtStack* stack, 
	BDT_SYSTEM_UDP_SOCKET socket, 
	const uint8_t* data, 
	size_t bytes, 
	const BdtEndpoint* remote, 
	const void* userData)
{
	Bdt_Package* resultPackages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	memset(resultPackages, 0, sizeof(Bdt_Package*) * BDT_MAX_PACKAGE_MERGE_LENGTH);
	bool isEncrypto = true;
	bool hasExchange = false;
	uint8_t key[BDT_AES_KEY_LENGTH];
	BdtPeerid keyPeerid;
	uint32_t result = Bdt_UnboxUdpPackage(stack,
		data,
		bytes, 
		resultPackages,
		BDT_MAX_PACKAGE_MERGE_LENGTH, 
		NULL, 
		key,
		&keyPeerid,
		&isEncrypto,
		&hasExchange);
	if (result)
	{
		return;
	}
	const size_t firstIndex = hasExchange ? 1 : 0;
	const Bdt_Package* firstPackage = resultPackages[firstIndex];
	if (!firstPackage)
	{
		return;
	}
	Bdt_UdpInterface* udpInterface = (Bdt_UdpInterface*)userData;
	assert(udpInterface);
	
	result = BFX_RESULT_SUCCESS;
	//TODO：优先分发sessiondata包（和其他数据包）

	if (Bdt_IsSnPackage(firstPackage))
	{
		for (size_t ix = firstIndex; ix < BDT_PROTOCOL_MIN_PACKAGE_LENGTH; ++ix)
		{
			Bdt_Package* package = resultPackages[ix];
			if (!package)
			{
				break;
			}
			bool handled = false;
			result = BdtSnClient_PushUdpPackage(BdtStackGetSnClient(stack),
				package,
				remote,
				udpInterface,
				isEncrypto? key : NULL,
				&handled);
		}
	}
	else
	{
		//TODO:通过Key来确定包的归属最好是做一次查询
		//     现在没有处理明文逻辑么？
		Bdt_TunnelContainer* container = BdtTunnel_GetContainerByRemotePeerid(BdtStackGetTunnelManager(stack), &keyPeerid);
		if (!container)
		{
			if (firstPackage->cmdtype == BDT_SYN_TUNNEL_PACKAGE)
			{
				container = BdtTunnel_CreateContainer(
					BdtStackGetTunnelManager(stack),
					&keyPeerid);
			}
		}
		assert(container);
		for (size_t ix = firstIndex; ix < BDT_PROTOCOL_MIN_PACKAGE_LENGTH; ++ix)
		{
			Bdt_Package* package = resultPackages[ix];
			if (!package)
			{
				break;
			}
			bool handled = false;
			result = BdtTunnel_PushUdpPackage(container, package, udpInterface, remote, &handled);
			// if handled set, ignore rest packages
			if (result != BFX_RESULT_SUCCESS || handled)
			{
				break;
			}
			BdtTunnel_PushSessionPackage(container, package, &handled);
		}
		Bdt_TunnelContainerRelease(container);
	}

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

#endif //__BDT_UDP_INTERFACE_IMP_INL__