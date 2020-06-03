#include <BdtCore.h>
#include "../../src/BuckyFramework/framework.h"

static uv_sem_t sig;
static uv_sem_t serverReady;

static volatile int32_t g_count = 0;

typedef struct BdtStack {
    BdtSystemFramework* framework;
} BdtStack;


static void serverRun(void* udata) {
    BdtSystemFramework* framework = (BdtSystemFramework*)udata;

    BfxUserData userData = { .userData = "server" };
    BDT_SYSTEM_TCP_SOCKET sock = framework->createUdpSocket(framework, &userData);

    BdtEndpoint addr;
    BdtEndpointFromString(&addr, "v4tcp127.0.0.1:6789");

    int ret = framework->udpSocketOpen(sock, &addr);
    assert(ret == 0);

    uv_sem_post(&serverReady);

    BLOG_INFO("udp sever thread running! %d", 100);
}


static void startUdpServer(BdtSystemFramework* framework) {
    BfxThreadOptions opt = {
        .name = "udpserver",
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    BFX_THREAD_HANDLE thandle;
    BfxCreateIOThread(opt, &thandle);

    BfxUserData ud = { .userData = framework };
    BfxPostMessage(thandle, serverRun, ud);
}

static void clientRun(void* udata) {
    BdtSystemFramework* framework = (BdtSystemFramework*)udata;

    BfxUserData userData = { .userData = "client" };
    BDT_SYSTEM_TCP_SOCKET sock = framework->createUdpSocket(framework, &userData);

    BdtEndpoint local;
    BdtEndpointFromString(&local, "v4tcp127.0.0.1:0");
    int ret = framework->udpSocketOpen(sock, &local);
    assert(ret == 0);

    BdtEndpoint addr;
    BdtEndpointFromString(&addr, "v4tcp127.0.0.1:6789");

    const char* value = "a msg from client #_#";
    size_t len = strlen(value) + 1;
    framework->udpSocketSendto(sock, &addr, value, len);

    BLOG_INFO("udp client thread running! %d", 100);
}


static void startUdpClient(BdtSystemFramework* framework) {
    BfxThreadOptions opt = {
        .name = "udpclient",
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    BFX_THREAD_HANDLE thandle;
    BfxCreateIOThread(opt, &thandle);

    BfxUserData ud = { .userData = framework };
    BfxPostMessage(thandle, clientRun, ud);
}


static void _bdtPushUdpSocketData(BdtStack* stack, BDT_SYSTEM_UDP_SOCKET socket, const uint8_t* data, size_t bytes, const BdtEndpoint* remote, const void* userData) {
    BdtSystemFramework* framework = stack->framework;
    assert(framework);

    const char* msg = (char*)data;
    BLOG_INFO("%s", msg);

    const char* type = (char*)userData;
    if (strcmp(type, "client") == 0) {
        msg = "a msg from client #_#";
    } else if (strcmp(type, "server") == 0) {
        msg = "a msg from server @_@";
    } else {
        assert(false);
    }

    int count = BfxAtomicDec32(&g_count);
    if (count <= 0) {
        uv_sem_post(&sig);
        return;
    }

    BfxThreadSleep(1000 * 100);

    size_t len = strlen(msg) + 1;
    framework->udpSocketSendto(socket, remote, msg, len);
}

void testFrameworkUdp() {
    BuckyFrameworkOptions options = {
        .size = sizeof(BuckyFrameworkOptions),
        .tcpThreadCount = 4,
        .udpThreadCount = 4,
        .timerThreadCount = 4,
        .dispatchThreadCount = 4
    };

    BdtSystemFrameworkEvents events = {
        .bdtPushTcpSocketEvent = NULL,
        .bdtPushTcpSocketData = NULL,
        .bdtPushUdpSocketData = _bdtPushUdpSocketData,
    };
    
    // 为了测试需要，构造一个假的BdtStack
    BdtStack* dummy = (BdtStack*)BfxMalloc(sizeof(BdtStack));
    BdtSystemFramework* framework = createBuckyFramework(&events, &options);
    dummy->framework = framework;
    framework->stack = dummy;

    uv_sem_init(&sig, 0);
    g_count = 100;

    uv_sem_init(&serverReady, 0);

    startUdpServer(framework);

    uv_sem_wait(&serverReady);

    startUdpClient(framework);

    // 等待测试结束
    
    uv_sem_wait(&sig);
}