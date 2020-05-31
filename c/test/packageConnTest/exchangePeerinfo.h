#ifndef _BDT_PACKAGE_CONNECTION_TEST_EXCHANGE_PEER_INFO_H_
#define _BDT_PACKAGE_CONNECTION_TEST_EXCHANGE_PEER_INFO_H_

#include <BDTCore/BdtCore.h>
#include <BuckyFramework/thread/bucky_thread.h>

#define  TEST_NODE_PEERINFO_LEN 1024 * 3

struct ExchangePeerInfo;
typedef struct ExchangePeerInfo
{
	void (*sendPeerInfoTo)(struct ExchangePeerInfo* exchange, const BdtPeerInfo* peerinfo);
	const BdtPeerInfo* (*getPeerInfoFrom)(struct ExchangePeerInfo* exchange);
}ExchangePeerInfo;

typedef struct ShareMemoryExchangePeerInfo
{
	ExchangePeerInfo base;
	HANDLE hRemoteEvent;
	HANDLE hMap;
}ShareMemoryExchangePeerInfo;
ShareMemoryExchangePeerInfo* ShareMemoryExchangePeerInfoCreate();


typedef struct NetworkExchangePeerInfo
{
	ExchangePeerInfo base;
	int port;
	char ip[64];
	char remoteIp[64];
}NetworkExchangePeerInfo;
NetworkExchangePeerInfo* NetworkExchangePeerInfoCreate(const char* ip, const char* remoteip);

#endif