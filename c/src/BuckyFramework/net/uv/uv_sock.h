#pragma once

#include <BuckyBase/BuckyBase.h>
#include <BDTCore/BdtCore.h>
#include <BDTCore/BdtSystemFramework.h>
#include "../../framework.h"
#include "../../uv_comnon.h"
#include "../common/send_pool.h"


#define UV_SOCKET_COMMON                    \
    struct _BuckyFramework* framework;      \
    volatile int32_t ref;                   \
    volatile int32_t status;                \
    BfxUserData udata;                      \
    BFX_THREAD_HANDLE thread;               \

typedef struct {
    UV_SOCKET_COMMON
}_UVSocketCommon;

typedef struct _UVTCPSocket {
    UV_SOCKET_COMMON

    uv_tcp_t socket;
    BdtEndpoint local;
    BdtEndpoint remote;
    int backlog;

    _TCPSendPool sendPool;
}_UVTCPSocket;


typedef struct _UVUDPSocket{
    UV_SOCKET_COMMON

    uv_udp_t socket;

}_UVUDPSocket;


typedef struct {
    void (*connection)(_UVTCPSocket* socket, _UVTCPSocket* comingSocket);
    void (*established)(_UVTCPSocket* socket);
    void (*drain)(_UVTCPSocket* socket);
    void (*data)(_UVTCPSocket* socket, const char* data, size_t bytes);
    void (*error)(_UVTCPSocket* socket, int errCode, const char* errMsg);
}_UVTCPSocketEvents;

typedef struct {
    void (*data)(_UVUDPSocket* socket, const char* data, size_t bytes, const BdtEndpoint* remote);
    void (*error)(_UVUDPSocket* socket, int errCode, const char* errMsg);
}_UVUDPSocketEvents;

int _uvTranslateError(int status);

void _initSocketCommon(_UVSocketCommon* common, struct _BuckyFramework* framework, BfxUserData udata);
void _uninitSocketCommon(_UVSocketCommon* common);

void _uvSocketInitUserData(_UVSocketCommon* socket, BfxUserData udata);
void _uvSocketGetUserData(_UVSocketCommon* socket, BfxUserData* udata);

int32_t _uvUdpSocketAddref(_UVUDPSocket* sock);
int32_t _uvUdpSocketRelease(_UVUDPSocket* sock);
