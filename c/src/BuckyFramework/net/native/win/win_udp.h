#include "./win_sock.h"

typedef struct _NativeUDPSocket _NativeUDPSocket;

typedef struct {
    typedef void (*sendTo)(_NativeUDPSocket* socket, int err, void* buffer, size_t len);
    typedef void (*recvFrom)(_NativeUDPSocket* socket, int err, void* buffer, size_t len, const );
    typedef void (*error)(_NativeUDPSocket* socket, int err);
} _WinUDPSocketEvents;

typedef struct _NativeUDPSocket {
    NATIVE_SOCKET_COMMON

    BdtEndpoint local;

    _WinUDPSocketEvents events;

} _NativeUDPSocket;
