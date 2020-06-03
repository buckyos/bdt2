#pragma once

typedef struct _PackageDispatchThread _PackageDispatchThread;
typedef struct _BuckyFramework _BuckyFramework;

typedef struct _PackageDispatchManager {
    _PackageDispatchThread* threads;
    size_t count;

    volatile int32_t index;

}_PackageDispatchManager;

int _initPackageDispatchManager(_BuckyFramework* framework, _PackageDispatchManager* manager, const BuckyFrameworkOptions* options);

// 收到了udp包，会接管data
void _uvUdpSocketOnData(_UVUDPSocket* socket, const char* data, size_t bytes, const BdtEndpoint* remote);