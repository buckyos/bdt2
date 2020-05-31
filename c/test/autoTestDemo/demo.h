#ifndef _BDT_AUTO_TEST_STACK_H_
#define _BDT_AUTO_TEST_STACK_H_

#include <BDTCore/BdtCore.h>
#include <BuckyFramework/framework.h>

struct TestDemoConnection;
typedef struct TestDemo TestDemo;

TestDemo* TestDemoCreate(uint16_t port, const char* id);
BdtStack* TestDemoGetBdtStack(TestDemo* stack);
const BdtPeerInfo* TestDemoGetSnPeerInfo(TestDemo* stack);

void TestDemoRun(TestDemo* stack);

void TestDemoOnConnectionFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, uint64_t time);
void TestDemoOnConnectionRecvFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, const char* name, uint64_t totaltime);
void TestDemoOnConnectionSendFinish(TestDemo* demo, struct TestDemoConnection* conn, uint32_t errcode, const char* name, uint64_t totaltime);

#endif