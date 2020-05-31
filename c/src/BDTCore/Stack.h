#ifndef _BDT_STACK_H_
#define _BDT_STACK_H_
#include "./BdtCore.h"
#include "./BdtSystemFramework.h"
#include "./Global/Module.h"
#include "./Interface/NetManager.h"
#include "./PeerFinder.h"
#include "./Tunnel/TunnelManager.h"
#include "./Connection/ConnectionManager.h"
#include "./SnClient/Client.h"
#include "./History/Updater.h"

// add ref

const BdtPeerInfo* BdtStackGetConstLocalPeer(BdtStack* stack);
const BdtPeerConstInfo* BdtStackGetLocalPeerConstInfo(BdtStack* stack);
const BdtPeerid* BdtStackGetLocalPeerid(BdtStack* stack);
const BdtPeerSecret* BdtStackGetSecret(BdtStack* stack);

BdtSystemFramework* BdtStackGetFramework(BdtStack* stack);
const BdtStackConfig* BdtStackGetConfig(BdtStack* stack);
Bdt_EventCenter* BdtStackGetEventCenter(BdtStack* stack);
Bdt_Uint32IdCreator* BdtStackGetIdCreator(BdtStack* stack);
BdtPeerFinder* BdtStackGetPeerFinder(BdtStack* stack);
Bdt_TunnelContainerManager* BdtStackGetTunnelManager(BdtStack* stack);
Bdt_ConnectionManager* BdtStackGetConnectionManager(BdtStack* stack);
BdtSuperNodeClient* BdtStackGetSnClient(BdtStack* stack);
BdtHistory_KeyManager* BdtStackGetKeyManager(BdtStack* stack);
Bdt_NetManager* BdtStackGetNetManager(BdtStack* stack);
Bdt_StackTls* BdtStackGetTls(BdtStack* stack);
BdtStorage* BdtStackGetStorage(BdtStack* stack);
BdtHistory_PeerUpdater* BdtStackGetHistoryPeerUpdater(BdtStack* stack);

#endif //_BDT_STACK_H_