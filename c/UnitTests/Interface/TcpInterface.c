//
//
//static const BdtPeerInfo* InitPeerInfo()
//{
//	BdtPeerConstInfo constInfo;
//	constInfo.areaCode.continent = 0x01;
//	constInfo.areaCode.country = 0x02;
//	constInfo.areaCode.carrier = 0x03;
//	constInfo.areaCode.city = 0x4444;
//	constInfo.areaCode.inner = 0x05;
//
//	uint8_t deviceId[6] = { 'd','e','v','i','c','e' };
//	memcpy(constInfo.deviceId, _deviceId, 6);
//	//pInfo->constInfo.deviceId = { 'd','e','v','i','c','e' };
//	constInfo.publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;
//	for (int i = 0; i < BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024; ++i)
//	{
//		constInfo.publicKey.rsa1024[i] = i;
//	}
//	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(0);
//	BdtPeerInfoSetConstInfo(builder, &constInfo);
//	return BdtPeerInfoBuilderFinish(builder, NULL);
//}
//
//BdtStack* InitStack()
//{
//	BuckyFrameworkOptions options = {
//		.size = sizeof(BuckyFrameworkOptions),
//		.threadCount = 4,
//	};
//
//	BdtSystemFrameworkEvents events = {
//		.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent,
//		.bdtPushTcpSocketData = BdtPushTcpSocketData,
//		.bdtPushUdpSocketData = BdtPushUdpSocketData,
//	};
//
//	BdtPeerArea area = {
//		.continent = 0,
//		.country = 0,
//		.carrier = 0,
//		.city = 0,
//		.inner = 0
//	};
//	BdtPeerid peerid;
//	BdtPeerConstInfo localPeerConst;
//	BdtPeerSecret localPeerSecret;
//	BdtCreatePeerid(&area, "local", BDT_PEER_PUBLIC_KEY_TYPE_RSA1024, &peerid, &localPeerConst, &localPeerSecret);
//	BdtEndpoint ep;
//	BdtEndpointFromString(&ep, "v4udp127.0.0.1:10000");
//	ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
//	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET | BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
//	BdtPeerInfoSetConstInfo(builder, &localPeerConst);
//	BdtPeerInfoSetSecret(builder, &localPeerSecret);
//	BdtPeerInfoAddEndpoint(builder, &ep);
//	const BdtPeerInfo* localPeer = BdtPeerInfoBuilderFinish(builder, &localPeerSecret);
//
//	BdtSystemFramework* framework = createBuckyFramework(&events, &options);
//	BfxUserData owner = {
//		.userData = NULL,
//		.lpfnAddRefUserData = NULL,
//		.lpfnReleaseUserData = NULL
//	};
//	BdtStack* lstack = NULL;
//	BdtCreateStack(framework, localPeer, NULL, 0, NULL, OnStackEvent, &userData, &lstack);
//	framework->stack = lstack;
//	return lstack;
//}
//
//bool BdtTestTcpBoxFirstResp(BdtStack* pStack)
//{
//	int r = 0;
//	Bdt_TcpInterface* pInterface = BFX_MALLOC_OBJ(Bdt_TcpInterface);
//
//	//设置pInterface状态
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP;
//	pInterface->isCrypto = true;
//
//	//设置pInterface中的aesKey
//	Bdt_AesGenerateKey(pInterface->aesKey);
//
//	BdtPackageTcpAckConnection* pTcpAck = Bdt_TcpAckConnectionPackageCreate();
//	pTcpAck->result = 0;
//	pTcpAck->seq = 0x100;
//	pTcpAck->toSessionId = 0x01000100;
//
//	size_t len = 0;
//	BdtPackage* firstPackages[1] = { (BdtPackage*)pTcpAck };
//	uint8_t* tcpbuffer = malloc(1024 * 64);
//	Bdt_TcpInterfaceBoxPackage(pInterface, firstPackages, 1, tcpbuffer, 64 * 1024, &len);
//	//Box CryptoData
//	uint8_t* dataBuffer = tcpbuffer + len + 2;
//	memset(dataBuffer, 0x88, 1024 * 16);
//	size_t headerLen;
//	size_t boxlen;
//	pInterface->state = BDT_TCP_INTERFACE_STATE_STREAM;
//	r = BdtTcpBoxCryptoData(pInterface, dataBuffer, 1024 * 16, 1024 * 32,&boxlen, &headerLen);
//	
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP;
//
//	pInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
//	pInterface->cacheLength = 0;
//	pInterface->boxSize = 0;
//	pInterface->boxCount = 0;
//	//for (int i = 0; i < len; i++)
//	{
//		BdtPushTcpSocketData(pStack, 0, tcpbuffer, len + 2 + boxlen, pInterface);
//	}
//
//	return true;
//}
//
//bool BdtTestTcpBoxFirst(BdtStack* pStack)
//{
//	Bdt_TcpInterface* pInterface = BFX_MALLOC_OBJ(Bdt_TcpInterface);
//
//	//设置pInterface状态
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE;
//	pInterface->isCrypto = false;
//	pInterface->firstBlockSize = 6;
//
//
//
//	int r;
//	uint8_t* tcpbuffer = malloc(1024 * 64);
//
//	//BoxFirstPackage
//	BdtPackageTcpSynConnection* pTcpSyn1 = Bdt_TcpSynConnectionPackageCreate();
//	pTcpSyn1->seq = 112;
//	pTcpSyn1->toVPort = 112;
//	pTcpSyn1->fromSessionId = 0x112112;
//	uint8_t _peerid[6] = { 'f','r','o','m','i','d' };
//	memcpy(pTcpSyn1->fromPeerid.pid, _peerid, 6);
//	uint8_t _peerid2[6] = { 'f','r','o','m','i','d' };
//	memcpy(pTcpSyn1->toPeerid.pid, _peerid, 6);
//
//	size_t len;
//	BdtPackage* firstPackages[1] = { (BdtPackage*)pTcpSyn1 };
//
//	//测试代码是和自己交换秘钥，用自己的公钥加密
//	const BdtPeerSecret* secret = BdtStackGetSecret(pStack);
//	const BdtPeerConstInfo* selfConstInfo = BdtStackGetLocalPeerConstInfo(pStack);
//	r = Bdt_TcpInterfaceBoxFirstPackage(pInterface, firstPackages, 1, tcpbuffer, 1024 * 64, &len, selfConstInfo->publicKeyType, selfConstInfo->publicKey.rsa1024,
//		secret->secretLength, secret->secret.rsa1024);
//
//
//	//Unbox;
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE;
//	pInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
//	pInterface->cacheLength = 0;
//	pInterface->boxSize = 0;
//	pInterface->boxCount = 0;
//	for (int i = 0; i < len; i++)
//	{
//		BdtPushTcpSocketData(pStack, 0, tcpbuffer + i, 1, pInterface);
//	}
//
//	return true;
//}
//
//bool BdtTestTcpBoxFirstCrypto(BdtStack* pStack)
//{
//
//	Bdt_TcpInterface* pInterface = BFX_MALLOC_OBJ(Bdt_TcpInterface);
//	
//	//设置pInterface状态
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE;
//	pInterface->isCrypto = true;
//	pInterface->firstBlockSize = 6;
//	//设置pInterface中的aesKey
//	Bdt_AesGenerateKey(pInterface->aesKey);
//
//
//	int r;
//	uint8_t* tcpbuffer = malloc(1024 * 64);
//
//	//BoxFirstPackage
//	BdtPackageExchangeKey* pEx1 = Bdt_ExchangeKeyPackageCreate();
//	BdtPackageTcpSynConnection* pTcpSyn1 = Bdt_TcpSynConnectionPackageCreate();
//	pEx1->seq = 100;
//	uint8_t seqAndKeySign[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
//	memcpy(pEx1->seqAndKeySign, seqAndKeySign, 16);
//	uint8_t _peerid[6] = { 'f','r','o','m','i','d' };
//	memcpy(pEx1->fromPeerid.pid, _peerid, 6);
//	pEx1->peerInfo = _InitPeerInfo();
//	pEx1->sendTime = (uint32_t)time(NULL);
//
//	pTcpSyn1->seq = 112;
//	pTcpSyn1->toVPort = 112;
//
//
//	size_t len;
//	BdtPackage* firstPackages[2] = { (BdtPackage*)pEx1,(BdtPackage*)pTcpSyn1 };
//
//	//测试代码是和自己交换秘钥，用自己的公钥加密
//	const BdtPeerSecret* secret = BdtStackGetSecret(pStack);
//	const BdtPeerConstInfo* selfConstInfo = BdtStackGetLocalPeerConstInfo(pStack);
//	r = Bdt_TcpInterfaceBoxFirstPackage(pInterface, firstPackages, 2, tcpbuffer,1024*64,&len, selfConstInfo->publicKeyType,selfConstInfo->publicKey.rsa1024,
//		secret->secretLength, secret->secret.rsa1024);
//
//
//	//Unbox;
//	pInterface->state = BDT_TCP_INTERFACE_STATE_WAIT_FIRST_PACKAGE;
//	pInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
//	pInterface->cacheLength = 0;
//	pInterface->boxSize = 0;
//	pInterface->boxCount = 0;
//	for (int i = 0; i < len; i++)
//	{
//		BdtPushTcpSocketData(pStack, 0, tcpbuffer, len, pInterface);
//	}
//
//	free(tcpbuffer);
//
//	return false;
//}
//
//
//bool BdtTestTcpBoxData(BdtStack* pStack)
//{
//	int r = 0;
//	Bdt_TcpInterface* pInterface = BFX_MALLOC_OBJ(Bdt_TcpInterface);
//	
//	pInterface->state = BDT_TCP_INTERFACE_STATE_STREAM;
//	pInterface->unboxState = BDT_TCP_UNBOX_STATE_READING_LENGTH;
//	pInterface->isCrypto = true;
//	//设置pInterface中的aesKey
//	Bdt_AesGenerateKey(pInterface->aesKey);
//	//Box CryptoData
//	uint8_t* dataBuffer = malloc(1024 * 16 + 32 + 32);
//	memset(dataBuffer + 32, 0x88, 1024 * 16);
//	size_t headerLen;
//	pInterface->state = BDT_TCP_INTERFACE_STATE_STREAM;
//	size_t boxlen;
//	r = BdtTcpBoxCryptoData(pInterface, dataBuffer + 32, 1024 * 16, 1024 * 16 + 32,&boxlen, &headerLen);
//	//UnBox;
//	uint16_t* pheader = (uint16_t*)(dataBuffer + 32 - headerLen);
//	
//	pInterface->cacheLength = 0;
//	pInterface->boxSize = 0;
//	pInterface->boxCount = 0;
//	//for (int i = 0; i < len; i++)
//	{
//		BdtPushTcpSocketData(pStack, 0, dataBuffer + 32 - headerLen, boxlen+2, pInterface);
//	}
//	free(dataBuffer);
//
//	return true;
//}