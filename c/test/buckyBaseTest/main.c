extern void testThread();
extern void testRandom();
void testThreadBuffer();
void testThreadBuffer2();
extern void testBuffer();
extern void testStackBuffer();
int testBLog();
extern void testDebug();
void testPath();
void testEvents();
extern void testRef();

void testFrameworkTcp();
void testFrameworkUdp();
void testFrameworkTimer();
void testLog();
void testLock();
void testByteOrder();
void testString();
void testList();

#include "../../src/BuckyBase/BuckyBase.h"

static BFX_THREAD_LOCAL BfxThreadID id;

static int userTargetOutput(void* ud, const char* log, size_t len) {
    return 0;
}

int main() {

    BfxUserData userData = { 0 };
    BLogAddTarget(BLogTargetType_File, userTargetOutput, userData);

    testList();

    testFrameworkTimer();

    testFrameworkTcp();
    testFrameworkUdp();

    testLock();

    testLog();
    
    testThread();

    testByteOrder();

    testString();

    testRandom();

    testThreadBuffer2();

    testRef();

    testEvents();

    testPath();

    testDebug();

    testBuffer();
    testStackBuffer();

    return 0;
}