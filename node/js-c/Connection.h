#pragma once
#include <napi.h>
#include <BdtCore.h>
#include "./Global.h"

class Connection;

class OnConnectionConnect
    : public AsyncCallbackOnce
{
public:
    OnConnectionConnect(Napi::Env env, Napi::Function cb, Connection* connection);
    BdtConnectionConnectCallback ToCFunction();
private:
    static void BFX_STDCALL CCallback(BdtConnection* connection, uint32_t result, void* userData);
    static void JsCallback(Napi::Env env, Napi::Function callback, OnConnectionConnect* cb);

private:
    uint32_t m_result;
    Connection* m_connection;
};

class OnConnectionFirstAnswer
    : public AsyncCallbackOnce
{
public:
    OnConnectionFirstAnswer(Napi::Env env, Napi::Function cb);
    BdtConnectionFirstAnswerCallback ToCFunction();
private:
    const uint8_t* m_data;
    size_t m_len;
    static void BFX_STDCALL CCallback(const uint8_t* data, size_t len, void* userData);
    static void JsCallback(Napi::Env env, Napi::Function callback, OnConnectionFirstAnswer* cb);
};

class OnConnectionSend
    : public AsyncCallbackOnce
{
public:
    OnConnectionSend(Napi::Env env, Napi::Buffer<uint8_t> buffer, Napi::Function cb);
    // virtual ~OnConnectionSend();
    BdtConnectionSendCallback ToCFunction();
private:
    static void BFX_STDCALL CCallback(uint32_t result, const uint8_t* buffer, size_t length, void* userData);
    static void JsCallback(Napi::Env env, Napi::Function callback, OnConnectionSend* cb);
private:
    uint32_t m_result;
    size_t m_sent;
    Napi::Reference<Napi::Buffer<uint8_t> > m_refBuffer;
};

class OnConnectionRecv;

class ConnectionReceiver
{
public:
    template <int BUFFER_SIZE>
    struct RecvBuffer
    {
        uint8_t data[BUFFER_SIZE];
        size_t size;
        size_t pos;
    };

#define RECV_BUFFER_COUNT 8
#define RECV_BUFFER_SIZE (1024 * 8)
#define CACHE_BUFFER_SIZE (2 * 1024 * 1024)

public:
    ConnectionReceiver(BdtConnection* connection);

private:
    ~ConnectionReceiver();

public:
    int32_t AddRef();
    int32_t Release();

    void OnConnected();
    Napi::Value RecvJS(const Napi::CallbackInfo& cb);

    size_t CacheDataSize() const;
    uint8_t* Fetch(uint8_t* buffer,
        size_t* size,
        struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[RECV_BUFFER_COUNT],
        size_t* freeBufferCount
    );
    uint32_t RecvC(struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[], size_t freeBufferCount);

private:
    uint8_t* FetchNoLock(uint8_t* buffer,
        size_t* size,
        struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[RECV_BUFFER_COUNT],
        size_t* freeBufferCount);
    static void BFX_STDCALL CCallback(uint8_t* buffer, size_t length, void* userData);
    const BfxUserData* ToUserData(BfxUserData* userData);
    static void BFX_STDCALL AddRefAsUserData(void* ud);
    static void BFX_STDCALL ReleaseAsUserData(void* ud);

private:
    volatile int32_t m_ref;

    BdtConnection* m_connection;
    bool m_isClosed;

    BfxThreadLock m_lock;

    OnConnectionRecv* m_waitingRecvCallback; // ͬʱֻ����һ��

    // buffer
    struct RecvBuffer<RECV_BUFFER_SIZE> m_recvbuffer[RECV_BUFFER_COUNT];
    struct RecvBuffer<RECV_BUFFER_SIZE>* m_freeBuffer[RECV_BUFFER_COUNT];
    int m_freeBufferCount;
    struct RecvBuffer<CACHE_BUFFER_SIZE> m_cacheBuffer;
    size_t m_pendingSize;
};

class OnConnectionRecv
    : public AsyncCallbackOnce
{
public:
    OnConnectionRecv(Napi::Env env, Napi::Function cb, ConnectionReceiver* receiver);
    // virtual ~OnConnectionRecv();
    void CallJS();

private:
    static void JsCallback(Napi::Env env, Napi::Function callback, OnConnectionRecv* cb);
private:
    ConnectionReceiver* m_receiver;
};


class Connection
    : public Napi::ObjectWrap<Connection>
{
public:
    static Napi::Object Register(Napi::Env env, Napi::Object exports);
    static Napi::Value Wrap(Napi::Env env, BdtConnection* connection);
    Connection(const Napi::CallbackInfo& cb);
    virtual ~Connection();

    Napi::Value Connect(const Napi::CallbackInfo& cb);
    void OnConnected(uint32_t result);

private:
    Napi::Value Confirm(const Napi::CallbackInfo& cb);
    Napi::Value Send(const Napi::CallbackInfo& cb);
    Napi::Value Recv(const Napi::CallbackInfo& cb);
    Napi::Value Close(const Napi::CallbackInfo& cb);
    Napi::Value Reset(const Napi::CallbackInfo& cb);
    Napi::Value GetName(const Napi::CallbackInfo& cb);
    Napi::Value GetProviderName(const Napi::CallbackInfo& cb);
    static Napi::FunctionReference s_constructor;

private:
    void InitReceiver();

private:
    BdtConnection* m_connection;

    ConnectionReceiver* m_receiver;
};
