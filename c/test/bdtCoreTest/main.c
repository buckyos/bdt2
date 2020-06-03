#include "./bdtCoreTest.h"

BdtStack* testCreateBdtStack(
	BfxUserData userData,
	const BdtPeerInfo* localPeer,
	const BdtPeerInfo* remotePeer,
	StackEventCallback callback
) {
	BuckyFrameworkOptions options = {
	.size = sizeof(BuckyFrameworkOptions),
	.tcpThreadCount = 1,
	.udpThreadCount = 1,
	.timerThreadCount = 1,
	.dispatchThreadCount = 1,
	};

	BdtSystemFrameworkEvents events = {
		.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent,
		.bdtPushTcpSocketData = BdtPushTcpSocketData,
		.bdtPushUdpSocketData = BdtPushUdpSocketData,
	};

	BdtSystemFramework* framework = createBuckyFramework(&events, &options);

	BdtStack* stack = NULL;
	BdtCreateStack(framework, localPeer, &remotePeer, 1, NULL, callback, &userData, &stack);

	return stack;
}



const BdtPeerInfo* testCreatePeerInfo(const char* deviceid, const char* endpoints[]) 
{
	BdtPeerArea area = {
		.continent = 0,
		.country = 0,
		.carrier = 0,
		.city = 0,
		.inner = 0
	};

	BdtPeerid peerid;
	BdtPeerConstInfo localPeerConst;
	BdtPeerSecret localPeerSecret;
	BdtCreatePeerid(&area, deviceid, BDT_PEER_PUBLIC_KEY_TYPE_RSA1024, &peerid, &localPeerConst, &localPeerSecret);
	
	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET | BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST);
	BdtPeerInfoSetConstInfo(builder, &localPeerConst);
	BdtPeerInfoSetSecret(builder, &localPeerSecret);

	for (size_t ix = 0; endpoints[ix] != NULL; ++ix)
	{
		BdtEndpoint ep;
		BdtEndpointFromString(&ep, endpoints[ix]);
		ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
		BdtPeerInfoAddEndpoint(builder, &ep);
	}
	

	const BdtPeerInfo* peerInfo = BdtPeerInfoBuilderFinish(builder, &localPeerSecret);

	return peerInfo;
}






int main()
{
	//testPeerid();
	//testSenderBuffer();
	testStreamConn();
	//testStreamConnIpv6();
	//testPackageConn();
}

