#ifndef _BDT_AUTO_TEST_CONNECTION_H_
#define _BDT_AUTO_TEST_CONNECTION_H_
#include "./demo.h"

typedef struct TestDemoConnection TestDemoConnection;

TestDemoConnection* TestDemoConnectionCreate(TestDemo* stack, const char* name, const BdtPeerInfo* remotePeerInfo, bool peerKnowRemotePeerInfo);
TestDemoConnection* TestDemoConnectionCreateFromBdtConnection(TestDemo* stack, BdtConnection* bdtConn, const char* name);

bool TestDemoConnectionIsEqual(TestDemoConnection* conn, BdtConnection* bdtConn);

void TestDemoConnectionDestory(TestDemoConnection* conn);

const char* TestDemoConnectionGetName(TestDemoConnection* conn);

uint32_t TestDemoConnectionBeginRecv(TestDemoConnection* conn);
uint32_t TestDemoConnectionConnect(TestDemoConnection* conn);
uint32_t TestDemoConnectionSendFile(TestDemoConnection* conn, uint64_t filesize, const char* name);
uint32_t TestDemoConnectionClose(TestDemoConnection* conn);
uint32_t TestDemoConnectionReset(TestDemoConnection* conn);
double TestDemoConnectionGetSendSpeed(TestDemoConnection* conn);

#endif

