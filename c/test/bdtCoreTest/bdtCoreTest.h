#include <BDTCore/BdtCore.h>
#include "../../src/BuckyFramework/framework.h"
#include "../../src/BDTCore/Tunnel/TunnelModule.h"

extern void testPeerid();
extern void testPackageConn();
extern void testStreamConn();
extern void testStreamConnIpv6();
void testSenderBuffer();

BdtStack* testCreateBdtStack(
	BfxUserData userData,
	const BdtPeerInfo* localPeer,
	const BdtPeerInfo* remotePeer,
	StackEventCallback callback
);

const BdtPeerInfo* testCreatePeerInfo(const char* deviceid, const char* endpoint[]);
