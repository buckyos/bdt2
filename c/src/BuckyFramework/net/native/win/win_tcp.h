#pragma once

#include "./win_sock.h"

typedef struct {
    typedef void (*connect)(_NativeTCPSocket* socket, int err);
    typedef void (*connection)(_NativeTCPSocket* socket, int err, _NativeTCPSocket* comingSocket);
    typedef void (*send)(_NativeTCPSocket* socket, int err, void* buffer, size_t len);
    typedef void (*recv)(_NativeTCPSocket* socket, int err, void* buffer, size_t len);
    typedef void (*error)(_NativeTCPSocket* socket, int err);
} _WinTCPSocketEvents;

typedef struct {
    NATIVE_SOCKET_COMMON

    BdtEndpoint local;
    BdtEndpoint remote;

    int backlog;

    _WinTCPSocketEvents events;

    // ∑¢ÀÕª∫≥Â
    _TCPSendPool sendPool;
    
    // Ω” ’ª∫≥Â
    void* recvBuffer;
    int recvBufferSize;

    volatile int32_t reading;

} _NativeTCPSocket;

