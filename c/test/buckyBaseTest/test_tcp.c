#include <BdtCore.h>
#include "../../src/BuckyFramework/framework.h"

static uv_sem_t sig;
static uv_sem_t serverReady;

static volatile int32_t g_count = 0;

typedef struct BdtStack {
    BdtSystemFramework* framework;
} BdtStack;

void testTcp(BdtSystemFramework* framework) {
    BfxUserData udata = { 0 };
    BDT_SYSTEM_TCP_SOCKET sock = framework->createTcpSocket(framework, &udata);
}

static void serverRun(void* udata) {
    BdtSystemFramework* framework = (BdtSystemFramework*)udata;

    BfxUserData userData = { 0 };
    BDT_SYSTEM_TCP_SOCKET sock = framework->createTcpSocket(framework, &userData);

    BdtEndpoint addr;
    BdtEndpointFromString(&addr, "v4tcp127.0.0.1:4567");

    framework->tcpSocketListen(sock, &addr);

    uv_sem_post(&serverReady);

    BLOG_INFO("tcp sever thread running! %d", 100);
}


static void startTcpServer(BdtSystemFramework* framework) {
    BfxThreadOptions opt = {
        .name = "tcpserver",
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

    BfxUserData userData = { 0 };
    BDT_SYSTEM_TCP_SOCKET sock = framework->createTcpSocket(framework, &userData);

    BdtEndpoint addr;
    BdtEndpointFromString(&addr, "v4tcp127.0.0.1:4567");

    framework->tcpSocketConnect(sock, NULL, &addr);

    BLOG_INFO("tcp client thread running! %d", 100);
}


static void startTcpClient(BdtSystemFramework* framework) {
    BfxThreadOptions opt = {
        .name = "tcpclient",
        .stackSize = 0,
        .ssize = sizeof(BfxThreadOptions),
    };

    BFX_THREAD_HANDLE thandle;
    BfxCreateIOThread(opt, &thandle);

    BfxUserData ud = { .userData = framework };
    BfxPostMessage(thandle, clientRun, ud);
}


static void _bdtPushTcpSocketEvent(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, uint32_t eventId, void* eventParam, const void* userData) {
    BdtSystemFramework* framework = stack->framework;
    assert(framework);

    if (eventId == BDT_TCP_SOCKET_EVENT_CONNECTION) {
        BdtTcpSocketConnectionEvent* event = (BdtTcpSocketConnectionEvent*)eventParam;

        framework->tcpSocketResume(event->socket);

        const char* value = "a msg from server @_@";
        size_t len = strlen(value) + 1;
        framework->tcpSocketSend(event->socket, value, &len);

    } else if (eventId == BDT_TCP_SOCKET_EVENT_ESTABLISH) {
        const char* value = "a msg from client #_#";
        size_t len = strlen(value) + 1;
        framework->tcpSocketResume(socket);
        framework->tcpSocketSend(socket, value, &len);
    }
}

static void _bdtPushTcpSocketData(BdtStack* stack, BDT_SYSTEM_TCP_SOCKET socket, const uint8_t* data, size_t bytes, const void* userData) {
    BdtSystemFramework* framework = stack->framework;
    assert(framework);

    const char* msg = data;
    BLOG_INFO("recv msg %s", msg);

    int count = BfxAtomicDec32(&g_count);
    if (count <= 0) {
        uv_sem_post(&sig);
        return;
    }

    if (strncmp(msg, "a msg from server", strlen("a msg from server")) == 0) {
        const char* value = "a msg from client #_#";
        size_t len = strlen(value) + 1;
        framework->tcpSocketSend(socket, value, &len);
    } else {
        const char* value = "a msg from server @_@";
        size_t len = strlen(value) + 1;
        framework->tcpSocketSend(socket, value, &len);
    }
}

void testFrameworkTcp() {
    BuckyFrameworkOptions options = {
        .size = sizeof(BuckyFrameworkOptions),
        .tcpThreadCount = 4,
        .udpThreadCount = 4,
        .timerThreadCount = 4,
        .dispatchThreadCount = 4
    };

    BdtSystemFrameworkEvents events = {
        .bdtPushTcpSocketEvent = _bdtPushTcpSocketEvent,
        .bdtPushTcpSocketData = _bdtPushTcpSocketData,
        .bdtPushUdpSocketData = NULL,
    };

    BdtStack* dummy = (BdtStack*)BfxMalloc(sizeof(BdtStack));
    BdtSystemFramework* framework = createBuckyFramework(&events, &options);
    dummy->framework = framework;
    framework->stack = dummy;


    uv_sem_init(&serverReady, 0);

    startTcpServer(framework);
    
    uv_sem_wait(&serverReady);

    startTcpClient(framework);

    // µÈ´ý²âÊÔ½áÊø
    uv_sem_init(&sig, 0);
    uv_sem_wait(&sig);
}