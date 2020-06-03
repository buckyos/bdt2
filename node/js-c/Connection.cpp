#include "./Connection.h"
#include "./PeerInfo.h"

Napi::FunctionReference Connection::s_constructor;

Napi::Value Connection::Wrap(Napi::Env env, BdtConnection* connection)
{
    return s_constructor.New({ Napi::External<BdtConnection>::New(env, connection) });
}

Napi::Object Connection::Register(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(
        env,
        "Connection",
        {
            InstanceMethod("confirm", &Connection::Confirm),
            InstanceMethod("send", &Connection::Send),
            InstanceMethod("recv", &Connection::Recv),
            InstanceMethod("close", &Connection::Close),
            InstanceMethod("reset", &Connection::Reset),
            InstanceAccessor("name", &Connection::GetName, NULL, napi_enumerable),
            InstanceAccessor("providerName", &Connection::GetProviderName, NULL, napi_enumerable)
        });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("Connection", func);
    return exports;
}

Connection::Connection(const Napi::CallbackInfo& cb)
    : Napi::ObjectWrap<Connection>(cb)
    , m_connection(NULL)
    , m_receiver(NULL)
{
    this->m_connection = cb[0].As<Napi::External<BdtConnection> >().Data();
    BdtAddRefConnection(this->m_connection);
}


Connection::~Connection()
{
    if (this->m_receiver != NULL)
    {
        this->m_receiver->Release();
    }
    BdtReleaseConnection(this->m_connection);
}


OnConnectionConnect::OnConnectionConnect(Napi::Env env, Napi::Function cb, Connection* connection)
    : AsyncCallbackOnce(env, cb, "OnConnectionConnect"),
    m_connection(connection)
{
    connection->Ref();
}

BdtConnectionConnectCallback OnConnectionConnect::ToCFunction()
{
    return CCallback;
}

void BFX_STDCALL OnConnectionConnect::CCallback(BdtConnection* connection, uint32_t result, void* userData)
{
    OnConnectionConnect* cb = reinterpret_cast<OnConnectionConnect*>(userData);
    cb->AddRef();
    cb->m_result = result;
    cb->m_tsfn.NonBlockingCall(cb, JsCallback);
}

void OnConnectionConnect::JsCallback(Napi::Env env, Napi::Function callback, OnConnectionConnect* cb)
{
    cb->m_connection->OnConnected(cb->m_result);
    callback.Call({ cb->m_connection->Value(), BdtResult::ToObject(env, cb->m_result) });
    cb->m_connection->Unref();
    cb->Release();
}


OnConnectionFirstAnswer::OnConnectionFirstAnswer(Napi::Env env, Napi::Function cb)
    : AsyncCallbackOnce(env, cb, "OnConnectionFirstAnswer")
    , m_data(NULL)
    , m_len(0)
{

}

BdtConnectionFirstAnswerCallback OnConnectionFirstAnswer::ToCFunction()
{
    return CCallback;
}

void BFX_STDCALL OnConnectionFirstAnswer::CCallback(
    const uint8_t* data,
    size_t len,
    void* userData)
{
    OnConnectionFirstAnswer* cb = reinterpret_cast<OnConnectionFirstAnswer*>(userData);
    cb->AddRef();
    cb->m_data = data;
    cb->m_len = len;
    cb->m_tsfn.NonBlockingCall(cb, JsCallback);
}

void OnConnectionFirstAnswer::JsCallback(
    Napi::Env env,
    Napi::Function callback,
    OnConnectionFirstAnswer* cb)
{
    Napi::Value fa;
    if (cb->m_len)
    {
        fa = Napi::Buffer<uint8_t>::New(env, cb->m_len);
        ::memcpy(fa.As<Napi::Buffer<uint8_t> >().Data(), cb->m_data, cb->m_len);
    }
    else
    {
        fa = Napi::Buffer<uint8_t>::New(env, 0);
    }
    callback.Call({ fa });
    cb->Release();
}

// BDT_API(uint32_t) BdtConnectionConnect(
//  BdtConnection* connection,
//  uint16_t vport,
//  const BdtBuildTunnelParams* params,
//  const uint8_t* firstQuestion,
//  size_t len,
//  BdtConnectionConnectCallback connectCallback,
//  BdtConnectionFirstAnswerCallback faCallback,
//  const BfxUserData* userData);
Napi::Value Connection::Connect(const Napi::CallbackInfo& cb)
{
    Napi::Object cbParams = cb[0].As<Napi::Object>();

    uint16_t vport = (uint16_t)(static_cast<uint32_t>(cbParams.Get("vport").ToNumber()));

    BdtBuildTunnelParams* params = BuildTunnelParams::FromObject(cbParams);

    Napi::Value objQuestion = cbParams.Get("question");
    const uint8_t* fq = NULL;
    size_t fqLen = 0;
    if (objQuestion.IsBuffer())
    {
        fqLen = objQuestion.As<Napi::Buffer<uint8_t> >().Length();
        fq = objQuestion.As<Napi::Buffer<uint8_t> >().Data();
    }

    OnConnectionConnect* connectCallback = new OnConnectionConnect(cb.Env(), cbParams.Get("onConnect").As<Napi::Function>(), this);
    BfxUserData connectUserData;
    connectCallback->ToBfxUserData(&connectUserData);

    Napi::Value objOnAnswer = cbParams.Get("onAnswer");
    OnConnectionFirstAnswer* faCallback = NULL;
    BfxUserData faUserData;
    if (objOnAnswer.IsFunction())
    {
        faCallback = new OnConnectionFirstAnswer(cb.Env(), objOnAnswer.As<Napi::Function>());
        faCallback->ToBfxUserData(&faUserData);
    }

    uint32_t result = BdtConnectionConnect(
        this->m_connection,
        vport,
        params,
        fq,
        fqLen,
        connectCallback->ToCFunction(),
        &connectUserData,
        faCallback ? faCallback->ToCFunction() : NULL,
        faCallback ? &faUserData : NULL
    );

    connectCallback->Release();

    if (faCallback != NULL)
    {
        faCallback->Release();
    }

    BuildTunnelParams::Destroy(params);
    return BdtResult::ToObject(cb.Env(), result);
}

void Connection::InitReceiver()
{
    this->m_receiver = new ConnectionReceiver(this->m_connection);
}

void Connection::OnConnected(uint32_t result)
{
    if (result != BFX_RESULT_SUCCESS)
    {
        return;
    }

    uint32_t providerType = BdtConnectionProviderType(this->m_connection);
    // if (providerType == BDT_CONNECTION_PROVIDER_TYPE_PACKAGE)
    {
        this->InitReceiver();

        this->m_receiver->OnConnected();
    }
}

// BDT_API(uint32_t) BdtConfirmConnection(
//  BdtConnection* connection,
//  uint8_t* answer,
//  size_t len, 
//  BdtConnectionConnectCallback connectCallback,
//  const BfxUserData* userData
// )

Napi::Value Connection::Confirm(const Napi::CallbackInfo& cb)
{
    Napi::Object cbParams = cb[0].As<Napi::Object>();

    Napi::Value objAnswer = cbParams.Get("answer");
    uint8_t* fa = NULL;
    size_t faLen = 0;
    if (objAnswer.IsBuffer())
    {
        faLen = objAnswer.As<Napi::Buffer<uint8_t> >().Length();
        fa = objAnswer.As<Napi::Buffer<uint8_t> >().Data();
    }

    OnConnectionConnect* connectCallback = new OnConnectionConnect(cb.Env(), cbParams.Get("onConnect").As<Napi::Function>(), this);
    BfxUserData connectUserData;
    connectCallback->ToBfxUserData(&connectUserData);

    uint32_t result = BdtConfirmConnection(
        this->m_connection,
        fa,
        faLen,
        connectCallback->ToCFunction(),
        &connectUserData
    );

    connectCallback->Release();

    return BdtResult::ToObject(cb.Env(), result);
}



OnConnectionSend::OnConnectionSend(Napi::Env env, Napi::Buffer<uint8_t> buffer, Napi::Function cb)
    : AsyncCallbackOnce(env, cb, "OnConnectionSend")
    , m_refBuffer(Napi::Reference<Napi::Buffer<uint8_t> >::New(buffer, 1))
    , m_result(BFX_RESULT_SUCCESS)
    , m_sent(0)
{

}

//OnConnectionSend::~OnConnectionSend()
//{
//    printf("OnConnectionSend release.");
//}

BdtConnectionSendCallback OnConnectionSend::ToCFunction()
{
    return CCallback;
}

void BFX_STDCALL OnConnectionSend::CCallback(uint32_t result, const uint8_t* buffer, size_t length, void* userData)
{
    OnConnectionSend* cb = reinterpret_cast<OnConnectionSend*>(userData);
    cb->AddRef();
    cb->m_result = result;
    cb->m_sent = length;
    cb->m_tsfn.NonBlockingCall(cb, JsCallback);
}

void OnConnectionSend::JsCallback(Napi::Env env, Napi::Function callback, OnConnectionSend* cb)
{
    callback.Call({
        BdtResult::ToObject(env, cb->m_result),
        cb->m_refBuffer.Value(),
        Napi::Number::New(env, (double)cb->m_sent) });
    cb->m_refBuffer.Unref();
    cb->Release();
}

// BDT_API(uint32_t) BdtConnectionSend(
//  BdtConnection* connection, 
//  const uint8_t* buffer, 
//  size_t length, 
//  BdtConnectionSendCallback callback, 
//  const BfxUserData* userData);
Napi::Value Connection::Send(const Napi::CallbackInfo& cb)
{
    Napi::Object cbParams = cb[0].As<Napi::Object>();

    Napi::Value objData = cbParams.Get("data");

    uint8_t* data = NULL;
    size_t length = 0;

    length = objData.As<Napi::Buffer<uint8_t> >().Length();
    data = objData.As<Napi::Buffer<uint8_t> >().Data();

    OnConnectionSend* callback = new OnConnectionSend(cb.Env(), objData.As<Napi::Buffer<uint8_t> >(), cbParams.Get("onSend").As<Napi::Function>());
    BfxUserData userData;
    callback->ToBfxUserData(&userData);

    uint32_t result = BdtConnectionSend(
        this->m_connection,
        data,
        length,
        callback->ToCFunction(),
        &userData
    );

    callback->Release();

    return BdtResult::ToObject(cb.Env(), result);
}

ConnectionReceiver::ConnectionReceiver(BdtConnection* connection) :
    m_ref(1),
    m_waitingRecvCallback(NULL),
    m_connection(connection),
    m_isClosed(false),
    m_pendingSize(0),
    m_freeBufferCount(RECV_BUFFER_COUNT)
{
    for (int i = 0; i < BFX_ARRAYSIZE(this->m_recvbuffer); i++)
    {
        struct RecvBuffer<RECV_BUFFER_SIZE>* buffer = this->m_recvbuffer + i;
        buffer->pos = 0;
        buffer->size = sizeof(buffer->data);
        this->m_freeBuffer[i] = buffer;
    }

    this->m_cacheBuffer.pos = 0;
    this->m_cacheBuffer.size = sizeof(this->m_cacheBuffer.data);

    BfxThreadLockInit(&this->m_lock);
    BdtAddRefConnection(this->m_connection);
}

ConnectionReceiver::~ConnectionReceiver()
{
    assert(this->m_waitingRecvCallback == NULL);
    BdtReleaseConnection(this->m_connection);
    BfxThreadLockDestroy(&this->m_lock);
}

int32_t ConnectionReceiver::AddRef() {
    return BfxAtomicInc32(&this->m_ref);
}

int32_t ConnectionReceiver::Release() {
    int32_t ref = BfxAtomicDec32(&this->m_ref);
    if (ref == 0) {
        delete this;
        return 0;
    }
    return ref;
}

size_t ConnectionReceiver::CacheDataSize() const
{
    return this->m_cacheBuffer.pos;
}

uint8_t* ConnectionReceiver::Fetch(uint8_t* buffer,
    size_t* size,
    struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[RECV_BUFFER_COUNT],
    size_t* freeBufferCount)
{
    BfxThreadLockLock(&this->m_lock);
    uint8_t* fetchBuffer = FetchNoLock(buffer, size, freeBuffer, freeBufferCount);
    BfxThreadLockUnlock(&this->m_lock);
    return fetchBuffer;
}

uint8_t* ConnectionReceiver::FetchNoLock(uint8_t* outBuffer,
    size_t* size,
    struct RecvBuffer<RECV_BUFFER_SIZE>* outFreeBuffer[RECV_BUFFER_COUNT],
    size_t* freeBufferCount)
{
    size_t writePos = 0;
    *freeBufferCount = 0;

    if ((*size) > this->m_cacheBuffer.pos)
    {
        memcpy(outBuffer, this->m_cacheBuffer.data, this->m_cacheBuffer.pos);
        *size = this->m_cacheBuffer.pos;
        this->m_cacheBuffer.pos = 0;
    }
    else
    {
        memcpy(outBuffer, this->m_cacheBuffer.data, *size);
        this->m_cacheBuffer.pos -= *size;
        memcpy(this->m_cacheBuffer.data, this->m_cacheBuffer.data + (*size), this->m_cacheBuffer.pos);
    }

    size_t leftSize = sizeof(this->m_cacheBuffer.data) - this->m_cacheBuffer.pos - this->m_pendingSize;

    int i = 0;
    for (;
        leftSize > 0 && this->m_freeBufferCount > 0 && i < RECV_BUFFER_COUNT && this->m_freeBuffer[i] != NULL;
        i++)
    {
        struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer = this->m_freeBuffer[i];
        assert(freeBuffer->pos == 0);
        this->m_freeBuffer[i] = NULL;
        outFreeBuffer[*freeBufferCount] = freeBuffer;
        (*freeBufferCount)++;
        this->m_freeBufferCount--;
        if (leftSize > freeBuffer->size)
        {
            freeBuffer->pos = freeBuffer->size; // 可以写满
            this->m_pendingSize += freeBuffer->size;
            leftSize -= freeBuffer->size;
        }
        else
        {
            freeBuffer->pos = leftSize; // 只能写部分
            this->m_pendingSize += leftSize;
            break;
        }
    }

    if (i > 0 && i < RECV_BUFFER_COUNT)
    {
        int j = 0;
        for (; i < RECV_BUFFER_COUNT; j++, i++)
        {
            this->m_freeBuffer[j] = this->m_freeBuffer[i];
        }
        for (; j < RECV_BUFFER_COUNT; j++)
        {
            this->m_freeBuffer[j] = NULL;
        }
    }

    return outBuffer;
}

uint32_t ConnectionReceiver::RecvC(struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[], size_t freeBufferCount)
{
    if (freeBufferCount == 0 || this->m_isClosed)
    {
        return BFX_RESULT_SUCCESS;
    }

    BfxUserData userData;
    this->ToUserData(&userData);

    uint32_t ret = BFX_RESULT_SUCCESS;

    for (size_t i = 0; i < freeBufferCount; i++)
    {
        uint32_t result = BdtConnectionRecv(
            this->m_connection,
            freeBuffer[i]->data,
            freeBuffer[i]->pos,
            ConnectionReceiver::CCallback,
            &userData
        );

        if (result == BFX_RESULT_PENDING)
        {
            ret = BFX_RESULT_PENDING;
        }
        else if (result != BFX_RESULT_SUCCESS)
        {
            // assert(false);
            // BLOG_WARN("post recv to native failed: %u", result);
            return result;
        }
    }
    return ret;
}

void ConnectionReceiver::OnConnected()
{
    RecvBuffer<RECV_BUFFER_SIZE>* recvBuffer[RECV_BUFFER_COUNT];

    this->m_freeBufferCount = 0;
    for (int i = 0; i < RECV_BUFFER_COUNT; i++)
    {
        RecvBuffer<RECV_BUFFER_SIZE>* buffer = this->m_freeBuffer[i];
        assert(buffer != NULL);
        buffer->pos = RECV_BUFFER_SIZE;
        this->m_pendingSize += RECV_BUFFER_SIZE;
        recvBuffer[i] = buffer;
    }
    this->RecvC(recvBuffer, RECV_BUFFER_COUNT);
}

Napi::Value ConnectionReceiver::RecvJS(const Napi::CallbackInfo& cb)
{
    assert(this->m_waitingRecvCallback == NULL);

    // 1.尝试读FullBuffer
    // 2.尝试投递recv
    // 3.callback

    Napi::Object cbParams = cb[0].As<Napi::Object>();
    Napi::Value objBuffer = cb.Env().Null();

    size_t lengthJS = 0;

    struct RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[RECV_BUFFER_COUNT];
    size_t freeBufferCount = 0;

    BfxThreadLockLock(&m_lock);
    {
        if (this->m_cacheBuffer.pos > 0 || this->m_isClosed)
        {
            objBuffer = Napi::Buffer<uint8_t>::New(objBuffer.Env(), this->m_cacheBuffer.pos);
            uint8_t* bufferJS = objBuffer.As<Napi::Buffer<uint8_t> >().Data();
            lengthJS = this->m_cacheBuffer.pos;
            bufferJS = this->FetchNoLock(bufferJS, &lengthJS, freeBuffer, &freeBufferCount);
        }
        else
        {
            m_waitingRecvCallback = new OnConnectionRecv(
                cb.Env(),
                cbParams.Get("onRecv").As<Napi::Function>(),
                this);
            lengthJS = 0;
        }
    }
    BfxThreadLockUnlock(&this->m_lock);

    if (freeBufferCount > 0 && !this->m_isClosed)
    {
        this->RecvC(freeBuffer, freeBufferCount);
    }

    if (lengthJS > 0 || this->m_isClosed)
    {
        cbParams.Get("onRecv").As<Napi::Function>().Call({
            objBuffer,
            Napi::Number::New(cb.Env(), (double)lengthJS)
            });
        return BdtResult::ToObject(cb.Env(), BFX_RESULT_SUCCESS);
    }

    return BdtResult::ToObject(cb.Env(), BFX_RESULT_PENDING);
}

const BfxUserData* ConnectionReceiver::ToUserData(BfxUserData* userData)
{
    userData->lpfnAddRefUserData = ConnectionReceiver::AddRefAsUserData;
    userData->lpfnReleaseUserData = ConnectionReceiver::ReleaseAsUserData;
    userData->userData = this;
    return userData;
}

void BFX_STDCALL ConnectionReceiver::AddRefAsUserData(void* ud)
{
    ConnectionReceiver* receiver = (ConnectionReceiver*)ud;
    receiver->AddRef();
}

void BFX_STDCALL ConnectionReceiver::ReleaseAsUserData(void* ud)
{
    ConnectionReceiver* receiver = (ConnectionReceiver*)ud;
    receiver->Release();
}
void BFX_STDCALL ConnectionReceiver::CCallback(uint8_t* buffer, size_t length, void* userData)
{
    ConnectionReceiver* receiver = (ConnectionReceiver*)userData;
    OnConnectionRecv* jsCallback = NULL;
    size_t jsFillSize = 0;

    struct RecvBuffer<RECV_BUFFER_SIZE>* recvBuffer = (struct RecvBuffer<RECV_BUFFER_SIZE>*)buffer;
    assert(recvBuffer->size == sizeof(recvBuffer->data));
    assert(recvBuffer->pos >= length);

    if (length == 0)
    {
        receiver->m_isClosed = true;
    }

    size_t freeCacheSize = 0;

    BfxThreadLockLock(&receiver->m_lock);
    {
        jsCallback = receiver->m_waitingRecvCallback;
        receiver->m_waitingRecvCallback = NULL;

        assert(length <= receiver->m_cacheBuffer.size - receiver->m_cacheBuffer.pos);
        memcpy(receiver->m_cacheBuffer.data + receiver->m_cacheBuffer.pos, buffer, length);
        receiver->m_cacheBuffer.pos += length;
        receiver->m_pendingSize -= recvBuffer->pos;
        freeCacheSize = receiver->m_cacheBuffer.size - receiver->m_cacheBuffer.pos - receiver->m_pendingSize;
        if (freeCacheSize == 0)
        {
            recvBuffer->pos = 0;
            receiver->m_freeBuffer[receiver->m_freeBufferCount] = recvBuffer;
            receiver->m_freeBufferCount++;
        }
        else if (freeCacheSize > RECV_BUFFER_SIZE)
        {
            recvBuffer->pos = RECV_BUFFER_SIZE;
            receiver->m_pendingSize += RECV_BUFFER_SIZE;
        }
        else
        {
            recvBuffer->pos = freeCacheSize;
            receiver->m_pendingSize += freeCacheSize;
        }
    }
    BfxThreadLockUnlock(&receiver->m_lock);

    if (freeCacheSize > 0 && !receiver->m_isClosed)
    {
        receiver->RecvC(&recvBuffer, 1);
    }

    if (jsCallback != NULL)
    {
        jsCallback->CallJS();
        jsCallback->Release();
    }
}

OnConnectionRecv::OnConnectionRecv(Napi::Env env, Napi::Function cb, ConnectionReceiver* receiver)
    : AsyncCallbackOnce(env, cb, "OnConnectionRecv")
{
    this->m_receiver = receiver;
    this->m_receiver->AddRef();
}

//OnConnectionRecv::~OnConnectionRecv()
//{
//    printf("OnConnectionRecv release.");
//}

void OnConnectionRecv::CallJS()
{
    this->AddRef();
    this->m_tsfn.NonBlockingCall(this, JsCallback);
}

void OnConnectionRecv::JsCallback(Napi::Env env, Napi::Function callback, OnConnectionRecv* cb)
{
    ConnectionReceiver* receiver = cb->m_receiver;
    size_t recvSize = receiver->CacheDataSize();

    Napi::Buffer<uint8_t> jsBuffer = Napi::Buffer<uint8_t>::New(env, (size_t)recvSize);

    ConnectionReceiver::RecvBuffer<RECV_BUFFER_SIZE>* freeBuffer[RECV_BUFFER_COUNT];
    size_t freeBufferCount = 0;
    receiver->Fetch(jsBuffer.Data(), &recvSize, freeBuffer, &freeBufferCount);

    receiver->RecvC(freeBuffer, freeBufferCount);

    //callback.Call({
    //  cb->m_refBuffer.Value(),
    //  Napi::Number::New(env, (double)cb->m_recvSize)
    //  });

    callback.Call({
        jsBuffer,
        Napi::Number::New(env, (double)recvSize)
        });

    cb->m_receiver->Release();
    cb->Release();
}

// typedef void (BFX_STDCALL* BdtConnectionRecvCallback)(
//  uint8_t* buffer,
//  size_t recv,
//  void* userData);
// BDT_API(uint32_t) BdtConnectionRecv(
//  BdtConnection* connection,
//  uint8_t* buffer,
//  size_t length,
//  BdtConnectionRecvCallback callback,
//  const BfxUserData* userData);
Napi::Value Connection::Recv(const Napi::CallbackInfo& cb)
{
    return this->m_receiver->RecvJS(cb);
}

Napi::Value Connection::Close(const Napi::CallbackInfo& cb)
{
    uint32_t result = BdtConnectionClose(this->m_connection);
    return BdtResult::ToObject(cb.Env(), result);
}

Napi::Value Connection::Reset(const Napi::CallbackInfo& cb)
{
    uint32_t result = BdtConnectionReset(this->m_connection);
    return BdtResult::ToObject(cb.Env(), result);
}

Napi::Value Connection::GetName(const Napi::CallbackInfo& cb)
{
    return Napi::String::New(cb.Env(), BdtConnectionGetName(this->m_connection));
}


Napi::Value Connection::GetProviderName(const Napi::CallbackInfo& cb)
{
    return Napi::String::New(cb.Env(), BdtConnectionGetProviderName(this->m_connection));
}
