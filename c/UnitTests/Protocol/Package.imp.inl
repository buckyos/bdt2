#include "../TestCase.h"
#include <Protocol/PackageDecoder.inl>
#include <Protocol/PackageEncoder.inl>

static const BdtPeerInfo* InitPeerInfo()
{
	BdtPeerConstInfo constInfo;
	constInfo.areaCode.continent = 0x01;
	constInfo.areaCode.country = 0x02;
	constInfo.areaCode.carrier = 0x03;
	constInfo.areaCode.city = 0x4444;
	constInfo.areaCode.inner = 0x05;

	uint8_t deviceId[6] = { 'd','e','v','i','c','e' };
	memcpy(constInfo.deviceId, deviceId, 6);
	//pInfo->constInfo.deviceId = { 'd','e','v','i','c','e' };
	constInfo.publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;
	for (int i = 0; i < BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024; ++i)
	{
		constInfo.publicKey.rsa1024[i] = i;
	}
	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(0);
	BdtPeerInfoSetConstInfo(builder, &constInfo);
	return BdtPeerInfoBuilderFinish(builder, NULL);
}

static int TestPackageEncodeDecode(Bdt_TestCase* testCase)
{
	printf("%s\n", ">>>>");
	BLOG_INFO("%s","start package encode decode test.\n");

	Bdt_ExchangeKeyPackage* pEx1 = Bdt_ExchangeKeyPackageCreate();
	Bdt_SynTunnelPackage* pSy1 = Bdt_SynTunnelPackageCreate();
	Bdt_AckTunnelPackage* pAck1 = Bdt_AckTunnelPackageCreate();
	Bdt_AckAckTunnelPackage* pAckAck1 = Bdt_AckAckTunnelPackageCreate();
	Bdt_PingTunnelPackage* pPing1 = Bdt_PingTunnelPackageCreate();
	Bdt_PingTunnelRespPackage* pPingResp1 = Bdt_PingTunnelRespPackageCreate();
	Bdt_SnCallPackage* pSNCall1 = Bdt_SnCallPackageCreate();
	Bdt_SnCallRespPackage* pSNCallResp1 = Bdt_SnCallRespPackageCreate();
	Bdt_SnCalledPackage* pCalled1 = Bdt_SnCalledPackageCreate();
	Bdt_SnCalledRespPackage* pCalledResp1 = Bdt_SnCalledRespPackageCreate();
	Bdt_SnPingPackage* pSNPing1 = Bdt_SnPingPackageCreate();
	Bdt_SnPingRespPackage* pSNPingResp1 = Bdt_SnPingRespPackageCreate();
	Bdt_DataGramPackage* pDataGram1 = Bdt_DataGramPackageCreate();
	Bdt_TcpSynConnectionPackage* pTcpSyn1 = Bdt_TcpSynConnectionPackageCreate();
	Bdt_TcpAckConnectionPackage* pTcpAck1 = Bdt_TcpAckConnectionPackageCreate();
	Bdt_TcpAckAckConnectionPackage* pTcpAckAck1 = Bdt_TcpAckAckConnectionPackageCreate();

	pEx1->seq = 100;
	uint8_t seqAndKeySign[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
	memcpy(pEx1->seqAndKeySign, seqAndKeySign, 16);
	uint8_t _peerid[6] = { 'f','r','o','m','i','d' };
	memcpy(pEx1->fromPeerid.pid, _peerid, 6);
	pEx1->peerInfo = InitPeerInfo();
	pEx1->sendTime = 0;

	//pSy1->fromPeerid = { 'p','e','e','r','i','d' };
	memcpy(pSy1->fromPeerid.pid, _peerid, 6);
	uint8_t _peerid2[8] = { 'p','e','e','r','i','d','2','3' };
	memcpy(pSy1->toPeerid.pid, _peerid2, 8);
	pSy1->fromContainerId = 0x22334455;
	uint8_t _peerid3[8] = { 'p','e','e','r','i','d','p','p' };
	//pSy1->proxyPeerid = { 'p','e','e','r','i','d','p','p' };
	memcpy(pSy1->proxyPeerid.pid, _peerid3, 8);
	pSy1->proxyPeerInfo = NULL;
	pSy1->seq = 101;
	pSy1->peerInfo = pEx1->peerInfo;
	BdtAddRefPeerInfo(pSy1->peerInfo);
	pSy1->sendTime = pEx1->sendTime;

	pAck1->aesKeyHash = 0x99887766;
	pAck1->seq = 102;
	//pAck1->sessionId = 0xee;
	pAck1->result = 0;
	pAck1->peerInfo = pEx1->peerInfo;
	BdtAddRefPeerInfo(pAck1->peerInfo);
	pAck1->sendTime = pEx1->sendTime;
	pAck1->mtu = 1500;

	pAckAck1->seq = 103;
	pAckAck1->toContainerId = 103103;

	pPing1->packageId = 104104;
	pPing1->sendTime = 104104104;
	pPing1->recvData = 104104104;

	pPingResp1->ackPackageId = 105105;
	pPingResp1->sendTime = 105105105;
	pPingResp1->recvData = 105105105;

	pSNCall1->seq = 106;
	uint8_t _peeridSN[8] = { 's','n','t','o','p','e','e','r' };
	memcpy(pSNCall1->toPeerid.pid, _peeridSN, 8);
	pSNCall1->payload = BfxCreateBuffer(106);
	
	pSNCallResp1->seq = 107;
	pSNCallResp1->result = 107;
	pSNCallResp1->toPeerInfo = pEx1->peerInfo;
	BdtAddRefPeerInfo(pSNCallResp1->toPeerInfo);

	pCalled1->seq = 108;
	uint8_t _peerid4[8] = { 's','n','c','a','l','l','e','d' };
	memcpy(pCalled1->toPeerid.pid, _peerid4, 8);
	pCalled1->payload = BfxCreateBuffer(108);

	pSNPing1->seq = 109;
	uint8_t _peerid5[8] = { 's','n','p','i','n','g','i','d' };
	memcpy(pSNPing1->fromPeerid.pid, _peerid5, 8);

	pSNPingResp1->seq = 110;
	pSNPingResp1->result = 110;
	pSNPingResp1->peerInfo = pEx1->peerInfo;
	BdtAddRefPeerInfo(pSNPingResp1->peerInfo);

	pDataGram1->seq = 111;
	pDataGram1->destZone = 111111;
	pDataGram1->hopLimit = 111;
	pDataGram1->piece = 111;
	pDataGram1->ackSeq = 111;
	pDataGram1->sendTime = 111111111;
	pDataGram1->innerCmdType = 111;
	pDataGram1->data = BfxCreateBuffer(111);
	size_t datalen1;
	uint8_t* pdata1 = BfxBufferGetData(pDataGram1->data, &datalen1);
	memset(pdata1, 0x11, datalen1);

	pTcpSyn1->seq = 112;
	pTcpSyn1->toVPort = 112;
	
	pTcpAck1->seq = 113;
	pTcpAck1->result = 113;

	pTcpAckAck1->seq = 114;
	pTcpAckAck1->result = 114;

	Bdt_Package* allPackage[16] = { (Bdt_Package*)pEx1 ,(Bdt_Package*)pSy1, (Bdt_Package*)pAck1,(Bdt_Package*)pAckAck1,
		(Bdt_Package*)pPing1,(Bdt_Package*)pPingResp1,(Bdt_Package*)pSNCall1,(Bdt_Package*)pSNCallResp1,(Bdt_Package*)pCalled1,(Bdt_Package*)pCalledResp1,
		(Bdt_Package*)pSNPing1,(Bdt_Package*)pSNPingResp1,(Bdt_Package*)pDataGram1,(Bdt_Package*)pTcpSyn1,(Bdt_Package*)pTcpAck1,(Bdt_Package*)pTcpAckAck1};

	Bdt_Package* resultPackage[16];
	uint8_t* test_buffer = malloc(1024*64);
	size_t len;
	int r = 0;

	r = Bdt_EncodePackage(allPackage, 16, NULL, test_buffer, 1024*64, &len);
	r = Bdt_DecodePackage(test_buffer, len, NULL, resultPackage, true);

	Bdt_ExchangeKeyPackage* pEx2 = (Bdt_ExchangeKeyPackage*)resultPackage[0];
	Bdt_SynTunnelPackage* pSy2 = (Bdt_SynTunnelPackage*)resultPackage[1];
	Bdt_AckTunnelPackage* pAck2 = (Bdt_AckTunnelPackage*)resultPackage[2];
	Bdt_AckAckTunnelPackage* pAckAck2 = (Bdt_AckAckTunnelPackage*)resultPackage[3];
	Bdt_PingTunnelPackage* pPing2 = (Bdt_PingTunnelPackage*)resultPackage[4];
	Bdt_PingTunnelRespPackage* pPingResp2 = (Bdt_PingTunnelRespPackage*) resultPackage[5];
	Bdt_SnCallPackage* pSNCall2 = (Bdt_SnCallPackage*) resultPackage[6];
	Bdt_SnCallRespPackage* pSNCallResp2 = (Bdt_SnCallRespPackage*) resultPackage[7];
	Bdt_SnCalledPackage* pCalled2 = (Bdt_SnCalledPackage*) resultPackage[8];
	Bdt_SnCalledRespPackage* pCalledResp2 = (Bdt_SnCalledRespPackage*) resultPackage[9];
	Bdt_SnPingPackage* pSNPing2 = (Bdt_SnPingPackage*) resultPackage[10];
	Bdt_SnPingRespPackage* pSNPingResp2 = (Bdt_SnPingRespPackage*) resultPackage[11];
	Bdt_DataGramPackage* pDataGram2 = (Bdt_DataGramPackage*) resultPackage[12];
	Bdt_TcpSynConnectionPackage* pTcpSyn2 = (Bdt_TcpSynConnectionPackage*) resultPackage[13];
	Bdt_TcpAckConnectionPackage* pTcpAck2 = (Bdt_TcpAckConnectionPackage*) resultPackage[14];
	Bdt_TcpAckAckConnectionPackage* pTcpAckAck2 = (Bdt_TcpAckAckConnectionPackage*) resultPackage[15];

	//assert(BdtProtocol_IsEqualBdtPackage((BdtPackage*)pEx1, (BdtPackage*)pEx2));
	//assert(IsEqualBdtPackage((BdtPackage*)pSy1, (BdtPackage*)pSy2));
	//assert(IsEqualBdtPackage((BdtPackage*)pAck1, (BdtPackage*)pAck2));
	//assert(IsEqualBdtPackage((BdtPackage*)pAckAck1, (BdtPackage*)pAckAck2));
	//assert(IsEqualBdtPackage((BdtPackage*)pPing1, (BdtPackage*)pPing2));
	//assert(IsEqualBdtPackage((BdtPackage*)pPingResp1, (BdtPackage*)pPingResp2));
	//assert(IsEqualBdtPackage((BdtPackage*)pSNCall1, (BdtPackage*)pSNCall2));
	//assert(IsEqualBdtPackage((BdtPackage*)pSNCallResp1, (BdtPackage*)pSNCallResp2));
	//assert(IsEqualBdtPackage((BdtPackage*)pSNPing1, (BdtPackage*)pSNPing2));
	//assert(IsEqualBdtPackage((BdtPackage*)pSNPingResp1, (BdtPackage*)pSNPingResp2));
	//assert(IsEqualBdtPackage((BdtPackage*)pDataGram1, (BdtPackage*)pDataGram2));
	//assert(IsEqualBdtPackage((BdtPackage*)pTcpSyn1, (BdtPackage*)pTcpSyn2));
	//assert(IsEqualBdtPackage((BdtPackage*)pTcpAck1, (BdtPackage*)pTcpAck2));
	//assert(IsEqualBdtPackage((BdtPackage*)pTcpAckAck1, (BdtPackage*)pTcpAckAck2));



	Bdt_ExchangeKeyPackageFree(pEx2);
	Bdt_SynTunnelPackageFree(pSy2);
	Bdt_AckTunnelPackageFree(pAck2);
	Bdt_AckAckTunnelPackageFree(pAckAck2);
	Bdt_PingTunnelPackageFree(pPing2);
	Bdt_PingTunnelRespPackageFree(pPingResp2);
	Bdt_SnCallPackageFree(pSNCall2);
	Bdt_SnCallRespPackageFree(pSNCallResp2);
	Bdt_SnCalledPackageFree(pCalled2);
	Bdt_SnCalledRespPackageFree(pCalledResp2);
	Bdt_SnPingPackageFree(pSNPing2);
	Bdt_SnPingRespPackageFree(pSNPingResp2);
	Bdt_DataGramPackageFree(pDataGram2);
	Bdt_TcpSynConnectionPackageFree(pTcpSyn2);
	Bdt_TcpAckConnectionPackageFree(pTcpAck2);
	Bdt_TcpAckAckConnectionPackageFree(pTcpAckAck2);

	//-----test sessiondata
	Bdt_SessionDataPackage* pSessionData1 = Bdt_SessionDataPackageCreate();
	pSessionData1->sessionId = 0x40;
	pSessionData1->streamPos = 0x404040;
	pSessionData1->ackStreamPos = 0x404040;
	pSessionData1->toSessionId = 0x4040;
	pSessionData1->recvSpeedlimit = 0x404040;
	pSessionData1->sendTime = 0x404040;
	pSessionData1->payload = malloc(0x40);
	pSessionData1->payloadLength = 0x40;
	pSessionData1->synPart = malloc(sizeof(Bdt_SessionDataSynPart));
	pSessionData1->synPart->ccType = 0x40;
	pSessionData1->synPart->fromSessionId = 0x404040;
	pSessionData1->synPart->synControl = 0x40;
	pSessionData1->synPart->synSeq = 0x4040;
	pSessionData1->synPart->toVPort = 0x40;
	pSessionData1->packageIDPart = malloc(sizeof(Bdt_SessionDataPackageIdPart));
	pSessionData1->packageIDPart->totalRecv = 0x404040;
	pSessionData1->packageIDPart->packageId = 0x404040;

	Bdt_Package* sessionDataPackages[3] = { (Bdt_Package*)pEx1 ,(Bdt_Package*)pSy1,(Bdt_Package*)pSessionData1 };
	Bdt_Package* resultPackages2[16] = { 0 };
	
	r = Bdt_EncodePackage(sessionDataPackages, 3, NULL, test_buffer, 1024 * 64, &len);
	r = Bdt_DecodePackage(test_buffer, len, NULL, resultPackages2, true);

	Bdt_ExchangeKeyPackage* pEx3 = (Bdt_ExchangeKeyPackage*)resultPackages2[0];
	Bdt_SynTunnelPackage* pSy3 = (Bdt_SynTunnelPackage*)resultPackages2[1];
	Bdt_SessionDataPackage* pSessionData3 = (Bdt_SessionDataPackage*)resultPackages2[2];

	//assert(IsEqualBdtPackage((BdtPackage*)pEx1, (BdtPackage*)pEx3));
	//assert(IsEqualBdtPackage((BdtPackage*)pSy1, (BdtPackage*)pSy3));
	assert(Bdt_IsEqualPackage((Bdt_Package*)pSessionData1, (Bdt_Package*)pSessionData3));

	Bdt_ExchangeKeyPackageFree(pEx3);
	Bdt_SynTunnelPackageFree(pSy3);
	Bdt_SessionDataPackageFree(pSessionData3);

	Bdt_ExchangeKeyPackageFree(pEx1);
	Bdt_SynTunnelPackageFree(pSy1);
	Bdt_AckTunnelPackageFree(pAck1);
	Bdt_AckAckTunnelPackageFree(pAckAck1);
	Bdt_PingTunnelPackageFree(pPing1);
	Bdt_PingTunnelRespPackageFree(pPingResp1);
	Bdt_SnCallPackageFree(pSNCall1);
	Bdt_SnCallRespPackageFree(pSNCallResp1);
	Bdt_SnCalledPackageFree(pCalled1);
	Bdt_SnCalledRespPackageFree(pCalledResp1);
	Bdt_SnPingPackageFree(pSNPing1);
	Bdt_SnPingRespPackageFree(pSNPingResp1);
	Bdt_DataGramPackageFree(pDataGram1);
	Bdt_TcpSynConnectionPackageFree(pTcpSyn1);
	Bdt_TcpAckConnectionPackageFree(pTcpAck1);
	Bdt_TcpAckAckConnectionPackageFree(pTcpAckAck1);

	free(test_buffer);
	return true;
}


