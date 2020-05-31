#include "./node.h"
#include <libuv-1.29.1/include/uv.h>

//int main()
//{
//	uv_sem_t pause;
//	uv_sem_init(&pause, 0);
//
//	BdtPeerArea area = {
//		.continent = 0,
//		.country = 0,
//		.carrier = 0,
//		.city = 0,
//		.inner = 0
//	};
//
//	BdtPeerid peerid;
//	BdtPeerConstInfo peerConst;
//	BdtPeerSecret peerSecret;
//	BdtCreatePeerid(&area, "remote", BDT_PEER_PUBLIC_KEY_TYPE_RSA1024, &peerid, &peerConst, &peerSecret);
//
//	BdtEndpoint ep;
//	char str[1024] = { 0 };
//	sprintf(str, "v4udp%s:%d", "127.0.0.1", 10001);
//	BdtEndpointFromString(&ep, str);
//	ep.flag |= BDT_ENDPOINT_FLAG_STATIC_WAN;
//
//	
//	BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET | BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO);
//	BdtPeerInfoSetConstInfo(builder, &peerConst);
//	BdtPeerInfoSetSecret(builder, &peerSecret);
//	BdtPeerInfoAddEndpoint(builder, &ep);
//	const BdtPeerInfo* peerInfo = BdtPeerInfoBuilderFinish(builder, &(node->peerSecret));
//
//	char out[1024] = { 0 };
//	BdtPeeridToString(BdtPeerInfoGetPeerid(peerInfo), out);
//	BLOG_DEBUG("local remotepeerid=%s \n", out);
//
//	BdtTestNode* node = BdtTestNodeCreate("local", "127.0.0.1", 10000, NULL);
//	BdtTestNodeSendFile(node, peerInfo, NULL);
//
//	uv_sem_wait(&pause);
//
//	return 0;
//}

int main(int argc, char* argv[])
{	
	/*uint8_t sz[100] = { 0xcd, 0xcd, 0xcd, 0xcd, '\0' };
	FILE* file = fopen("./ttttt", "w");
	int iseek = fseek(file, 2, SEEK_SET);
	fwrite(sz, 1, 100, file);
	fflush(file);
	fclose(file);
	uint8_t cmpStr[5] = { 0xcd, 0xcd, 0xcd, 0xcd, '\0' };
	int i = memcmp(sz, cmpStr, 4);*/
	
	if (argc < 7)
	{
		printf("xxx.exe self_deviceid, self_ip, self_port, other_peer_ip, whether_remote_peer(0|1) send_file_path");
		return 0;
	}
	uint16_t port = atoi(argv[3]);
	bool bRemote = atoi(argv[5]);
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	uv_sem_t pause;
	uv_sem_init(&pause, 0);

	BdtTestNode* nodeLocal = BdtTestNodeCreate(argv[1], argv[2], port, argv[4], bRemote);
	BdtTestNodeAccept(nodeLocal);
	if (strcmp(argv[6], "-"))
	{
		BdtTestNodeSendFile(nodeLocal, argv[6]);
	}
	
	uv_sem_wait(&pause);

	return 0;
}