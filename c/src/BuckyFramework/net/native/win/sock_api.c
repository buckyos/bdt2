#include "./sock_api.h"


// ipv4相关api
static void _initV4APIList(_WinSocketAPI* api) {
    DWORD bytesRet = 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BLOG_CHECK(sock != INVALID_SOCKET);

    GUID wsaidConnectEx = WSAID_CONNECTEX;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidConnectEx, sizeof(wsaidConnectEx)
        , (void*)& api->connectEx, sizeof(api->connectEx), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);

    GUID wsaidAcceptEx = WSAID_ACCEPTEX;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidAcceptEx, sizeof(wsaidAcceptEx)
        , (void*)& api->acceptEx, sizeof(api->acceptEx), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);


    GUID wsaidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidGetAcceptExSockaddrs, sizeof(wsaidGetAcceptExSockaddrs)
        , (void*)& api->getAcceptExSockaddrs, sizeof(api->getAcceptExSockaddrs), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);

    ret = closesocket(sock);
    BLOG_CHECK(ret == 0);
}

static void _initV6APIList(_WinSocketAPI* api) {
    SOCKET sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (!BFX_NATIVE_SOCKET_IS_VALID(sock)) {
        
        // 创建ipv6失败，可能是不支持，也可能是其它错误
        DWORD err = ::GetLastError();
        if (err == WSAEOPNOTSUPP || err == WSAEPROTONOSUPPORT) {
            BLOG_WARN("ipv6 not support! err=%d", err);
        } else {
            BLOG_ERROR("create ipv6 socket got error, err=%d", err);
        }

        return;
    }

    DWORD bytesRet = 0;

    GUID wsaidConnectEx = WSAID_CONNECTEX;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidConnectEx, sizeof(wsaidConnectEx)
        , (void*)& api->connectEx6, sizeof(api->connectEx6), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);

    GUID wsaidAcceptEx = WSAID_ACCEPTEX;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidAcceptEx, sizeof(wsaidAcceptEx)
        , (void*)& api->acceptEx6, sizeof(api->acceptEx6), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);


    GUID wsaidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &wsaidGetAcceptExSockaddrs, sizeof(wsaidGetAcceptExSockaddrs)
        , (void*)& api->getAcceptExSockaddrs6, sizeof(api->getAcceptExSockaddrs6), &bytesRet, NULL, NULL);
    BLOG_CHECK(ret == 0);

    ret = closesocket(sock);
    BLOG_CHECK(ret == 0);
}

static void _initAPIList(_WinSocketAPI* api) {
    _initV4APIList(api);

    _initV6APIList(api);
}

void _initSocketProcess(_WinSocketAPI* api) {

    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int ret = WSAStartup(wVersionRequested, &wsaData);
    BLOG_CHECK(ret == 0);
    if (ret != 0) {
        BLOG_ERROR("WSAStartup failed!");
    }

    _initAPIList(api);
}

void _uninitSocketProcess(_WinSocketAPI* api) {
    WSACleanup();
}

// from libuv
extern int uv_ntstatus_to_winsock_error(NTSTATUS status);

int _nativeTranslateErr(int err) {
    if (err == 0) {
        return 0;
    }

    // TODO 完善

    int ret = err;
    switch (err) {
    case WSAEWOULDBLOCK:
        ret = BFX_RESULT_PENDING;
        break;
    default:

    }
    return err;
}

int _winSocketStatusTranslateToError(NTSTATUS status) {
    int err = uv_ntstatus_to_winsock_error(status);

    return _nativeTranslateErr(err);
}

int _winSocketDealWithBOOLResult(BOOL ret) {
    if (ret) {
        return 0;
    }

    int sysErr = WSAGetLastError();
    BLOG_CHECK(sysErr);
    if (sysErr == ERROR_IO_PENDING || sysErr == WSA_IO_PENDING) {
        return 0;
    } else {
        return _nativeTranslateErr(sysErr);
    }
}
