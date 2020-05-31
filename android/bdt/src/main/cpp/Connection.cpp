//
// Created by tsukasa on 2020/2/17.
//
#include "./Connection.h"
#include "./Stack.h"
#include "./PeerInfo.h"


ConnectionWrapper* ConnectionWrapper::s_instance = NULL;

const ConnectionWrapper* ConnectionWrapper::Instance()
{
    return ConnectionWrapper::s_instance;
}

int ConnectionWrapper::OnLoad(JNIEnv* jenv)
{
    ConnectionWrapper* instance = new ConnectionWrapper();
    int result = instance->Initialize(jenv);
    if (result != BFX_RESULT_SUCCESS)
    {
        delete instance;
        return result;
    }
    s_instance = instance;
    return BFX_RESULT_SUCCESS;
}

const ConnectionWrapper::ConnectListenerWrapper* ConnectionWrapper::ConnectListener() const
{
    return this->m_connectListener;
}


const ConnectionWrapper::PreAcceptListenerWrapper* ConnectionWrapper::PreAcceptListener() const
{
    return this->m_preAcceptListener;
}

const ConnectionWrapper::AcceptListenerWrapper* ConnectionWrapper::AcceptListener() const
{
    return this->m_acceptListener;
}

const ConnectionWrapper::SendListenerWrapper* ConnectionWrapper::SendListener() const
{
    return this->m_sendListener;
}

const ConnectionWrapper::RecvListenerWrapper* ConnectionWrapper::RecvListener() const
{
    return this->m_recvListener;
}

ConnectionWrapper::ConnectionWrapper()
: m_class(0)
, m_constructor(0)
, m_connectListener(new ConnectListenerWrapper())
, m_preAcceptListener(new PreAcceptListenerWrapper())
, m_acceptListener(new AcceptListenerWrapper())
, m_sendListener(new SendListenerWrapper())
, m_recvListener(new RecvListenerWrapper())
{

}


int ConnectionWrapper::Initialize(JNIEnv* jenv)
{
    NativePointorWrapper<BdtConnection>::Initialize(jenv);
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_constructor = jenv->GetMethodID(this->m_class, "<init>", "()V");

    this->m_connectListener->Initialize(jenv);
    this->m_preAcceptListener->Initialize(jenv);
    this->m_acceptListener->Initialize(jenv);
    this->m_sendListener->Initialize(jenv);
    this->m_recvListener->Initialize(jenv);
    return BFX_RESULT_SUCCESS;
}

jobject ConnectionWrapper::ToObject(JNIEnv* jenv, BdtConnection* connection) const
{
    jobject jConnection = jenv->NewObject(this->m_class, this->m_constructor);
    return this->BindObject(jenv, jConnection, connection);
}

void ConnectionWrapper::ConnectListenerWrapper::ToCCallback(
        BdtConnectionFirstAnswerCallback* cbAnswer,
        BdtConnectionConnectCallback* cbConnect) const
{
    *cbAnswer = AnswerCallback;
    *cbConnect = ConnectCallback;
}


void BFX_STDCALL ConnectionWrapper::ConnectListenerWrapper::AnswerCallback(
        const uint8_t* data,
        size_t len,
        void* userData)
{
    const ConnectListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->ConnectListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    jbyteArray jAnswer = jenv->NewByteArray(len);
    jenv->SetByteArrayRegion(jAnswer, 0, len, reinterpret_cast<const jbyte*>(data));
    jenv->CallVoidMethod(jListener, listenerWrapper->m_onAnswer, jAnswer);
}

void BFX_STDCALL ConnectionWrapper::ConnectListenerWrapper::ConnectCallback(
        BdtConnection* connection,
        uint32_t result,
        void* userData)
{
    const ConnectListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->ConnectListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    if (result == BFX_RESULT_SUCCESS)
    {
        jenv->CallVoidMethod(jListener, listenerWrapper->m_onEstablish);
    }
    else
    {
        jenv->CallVoidMethod(jListener, listenerWrapper->m_onError, result);
    }
}

ConnectionWrapper::ConnectListenerWrapper::ConnectListenerWrapper()
: m_class(0)
, m_onEstablish(0)
, m_onAnswer(0)
, m_onError(0)
{

}

int ConnectionWrapper::ConnectListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection$ConnectListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onAnswer = jenv->GetMethodID(this->m_class, "onAnswer", "([B)V");
    this->m_onEstablish = jenv->GetMethodID(this->m_class, "onEstablish", "()V");
    this->m_onError = jenv->GetMethodID(this->m_class, "onError", "(I)V");
    return BFX_RESULT_SUCCESS;
}


BdtAcceptConnectionCallback ConnectionWrapper::PreAcceptListenerWrapper::ToCCallback() const
{
    return CCallback;
}

void BFX_STDCALL ConnectionWrapper::PreAcceptListenerWrapper::CCallback(
        uint32_t result,
        const uint8_t* buffer,
        size_t recv,
        BdtConnection* connection,
        void* userData)
{
    const PreAcceptListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->PreAcceptListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    if (result == BFX_RESULT_SUCCESS)
    {
        jobject jConnection = ConnectionWrapper::Instance()->ToObject(jenv, connection);
        jbyteArray jQuestion = jenv->NewByteArray(recv);
        jenv->SetByteArrayRegion(jQuestion, 0, recv, reinterpret_cast<const jbyte*>(buffer));

        jenv->CallVoidMethod(jListener, listenerWrapper->m_onConnection, jConnection, jQuestion);
    }
    else
    {
        jenv->CallVoidMethod(jListener, listenerWrapper->m_onError, result);
    }
}

ConnectionWrapper::PreAcceptListenerWrapper::PreAcceptListenerWrapper()
: m_class(0)
, m_onConnection(0)
, m_onError(0)
{

}

int ConnectionWrapper::PreAcceptListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection$PreAcceptListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onConnection = jenv->GetMethodID(this->m_class, "onConnection", "(Lcom/bucky/bdt/Connection;[B)V");
    this->m_onError = jenv->GetMethodID(this->m_class, "onError", "(I)V");
    return BFX_RESULT_SUCCESS;
}



BdtConnectionConnectCallback ConnectionWrapper::AcceptListenerWrapper::ToCCallback() const
{
    return CCallback;
}

void BFX_STDCALL ConnectionWrapper::AcceptListenerWrapper::CCallback(
        BdtConnection* connection,
        uint32_t result,
        void* userData)
{
    const AcceptListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->AcceptListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    if (result == BFX_RESULT_SUCCESS)
    {
        jenv->CallVoidMethod(jListener, listenerWrapper->m_onEstablish);
    }
    else
    {
        jenv->CallVoidMethod(jListener, listenerWrapper->m_onError, result);
    }
}

ConnectionWrapper::AcceptListenerWrapper::AcceptListenerWrapper()
: m_class(0)
, m_onEstablish(0)
, m_onError(0)
{

}


int ConnectionWrapper::AcceptListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection$AcceptListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onEstablish = jenv->GetMethodID(this->m_class, "onEstablish", "()V");
    this->m_onError = jenv->GetMethodID(this->m_class, "onError", "(I)V");
    return BFX_RESULT_SUCCESS;
}



BdtConnectionSendCallback ConnectionWrapper::SendListenerWrapper::ToCCallback() const
{
    return CCallback;
}

void BFX_STDCALL ConnectionWrapper::SendListenerWrapper::CCallback(
            uint32_t result,
            const uint8_t* buffer,
            size_t length,
            void* userData)
{
    const SendListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->SendListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    jenv->CallVoidMethod(jListener, listenerWrapper->m_onSent, result, static_cast<jlong>(length));
}

void ConnectionWrapper::SendListenerWrapper::GetBuffer(JNIEnv* jenv, jobject jListener, uint8_t** outBuffer, size_t* outLen) const
{
    jbyteArray jBuffer = static_cast<jbyteArray>(jenv->CallObjectMethod(jListener, this->m_getBuffer));
    *outBuffer = reinterpret_cast<uint8_t*>(jenv->GetByteArrayElements(jBuffer, NULL));
    *outLen = jenv->GetArrayLength(jBuffer);
}

ConnectionWrapper::SendListenerWrapper::SendListenerWrapper()
: m_class(0)
, m_onSent(0)
, m_getBuffer(0)
{

}

int ConnectionWrapper::SendListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection$SendListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onSent = jenv->GetMethodID(this->m_class, "onSent", "(IJ)V");
    this->m_getBuffer = jenv->GetMethodID(this->m_class, "getBuffer", "()[B");
    return BFX_RESULT_SUCCESS;
}


BdtConnectionRecvCallback ConnectionWrapper::RecvListenerWrapper::ToCCallback() const
{
    return CCallback;
}

void BFX_STDCALL ConnectionWrapper::RecvListenerWrapper::CCallback(
        uint8_t* buffer,
        size_t recv,
        void* userData)
{
    const RecvListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->RecvListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    jenv->CallVoidMethod(jListener, listenerWrapper->m_onRecv, static_cast<jlong>(recv));
}

void ConnectionWrapper::RecvListenerWrapper::GetBuffer(JNIEnv* jenv, jobject jListener, uint8_t** outBuffer, size_t* outLen) const
{
    jbyteArray jBuffer = static_cast<jbyteArray>(jenv->CallObjectMethod(jListener, this->m_getBuffer));
    *outBuffer = reinterpret_cast<uint8_t*>(jenv->GetByteArrayElements(jBuffer, NULL));
    *outLen = jenv->GetArrayLength(jBuffer);
}

ConnectionWrapper::RecvListenerWrapper::RecvListenerWrapper()
: m_class(0)
, m_onRecv(0)
, m_getBuffer(0)
{

}

int ConnectionWrapper::RecvListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Connection$RecvListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onRecv = jenv->GetMethodID(this->m_class, "onRecv", "(J)V");
    this->m_getBuffer = jenv->GetMethodID(this->m_class, "getBuffer", "()[B");
    return BFX_RESULT_SUCCESS;
}

/*
 * Class:     com_bucky_bdt_Connection
 * Method:    releaseNative
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_bucky_bdt_Connection_releaseNative
(JNIEnv* jenv, jobject jConnection)
{
    BdtConnection* connection = ConnectionWrapper::Instance()->FromObject(jenv, jConnection);
    if (connection != NULL)
    {
        BdtReleaseConnection(connection);
    }
}

/*
 * Class:     com_bucky_bdt_Connection
 * Method:    connect
 * Signature: (Lcom/bucky/bdt/Stack;Lcom/bucky/bdt/PeerConstInfo;S[[B[BLcom/bucky/bdt/Connection/ConnectEventListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_connect
        (JNIEnv* jenv, jobject jConnection, jobject jStack, jobject jRemote, jshort vport, jobjectArray jSnList, jbyteArray jQuestion, jobject jListener)
{
    BdtStack* stack = StackWrapper::Instance()->FromObject(jenv, jStack);

    const ConnectionWrapper* connectionWrapper = ConnectionWrapper::Instance();
    const ConnectionWrapper::ConnectListenerWrapper* listenerWrapper = connectionWrapper->ConnectListener();

    BdtPeerConstInfo remoteConstInfo;
    PeerConstInfoWrapper::Instance()->FromObject(jenv, jRemote, &remoteConstInfo);
    BdtPeerid remotePeerid;
    BdtGetPeeridFromConstInfo(&remoteConstInfo, &remotePeerid);

    BdtBuildTunnelParams params;
    ::memset(&params, 0, sizeof(BdtBuildTunnelParams));
    params.remoteConstInfo = &remoteConstInfo;
    size_t snListLen = jenv->GetArrayLength(jSnList);
    BdtPeerid* snList = NULL;
    if (snList != 0)
    {
        snList = new BdtPeerid[snListLen];
        for (size_t ix = 0; ix < snListLen; ++ix)
        {
            jbyteArray jPeerid = static_cast<jbyteArray>(jenv->GetObjectArrayElement(jSnList, ix));
            const BdtPeerid* peerid = reinterpret_cast<BdtPeerid*>(jenv->GetByteArrayElements(jPeerid, NULL));
            *(snList + ix) = *peerid;
        }
        params.flags |= BDT_BUILD_TUNNEL_PARAMS_FLAG_SN;
        params.snListLength = snListLen;
        params.snList = snList;
    }

    size_t questionLen = jenv->GetArrayLength(jQuestion);
    const uint8_t* question = reinterpret_cast<uint8_t*>(jenv->GetByteArrayElements(jQuestion, NULL));

    BfxUserData udListener;
    listenerWrapper->ToBfxUserData(jenv, jListener, &udListener);
    BdtConnectionFirstAnswerCallback cbAnswer;
    BdtConnectionConnectCallback cbConnect;
    listenerWrapper->ToCCallback(&cbAnswer, &cbConnect);

    BdtConnection* connection = NULL;
    uint32_t result = BdtCreateConnection(stack, &remotePeerid, true, &connection);
    connectionWrapper->BindObject(jenv, jConnection, connection);

    result = BdtConnectionConnect(
        connection,
        vport,
        &params,
        question,
        questionLen,
        cbConnect,
        &udListener,
        cbAnswer,
        &udListener);
    //tofix: if error, release udListener
    if (params.flags & BDT_BUILD_TUNNEL_PARAMS_FLAG_SN)
    {
        delete snList;
    }
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}

/*
 * Class:     com_bucky_bdt_Connection
 * Method:    listen
 * Signature: (Lcom/bucky/bdt/Stack;S)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_listen
        (JNIEnv* jenv, jclass _, jobject jStack, jshort vport)
{
    BdtStack* stack = StackWrapper::Instance()->FromObject(jenv, jStack);
    return BdtListenConnection(stack, vport, NULL);
}


/*
 * Class:     com_bucky_bdt_Connection
 * Method:    accept
 * Signature: (Lcom/bucky/bdt/Stack;SLcom/bucky/bdt/Connection/AcceptListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_accept
        (JNIEnv* jenv, jclass _, jobject jStack, jshort vport, jobject jListener)
{
    BdtStack* stack = StackWrapper::Instance()->FromObject(jenv, jStack);
    const ConnectionWrapper::PreAcceptListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->PreAcceptListener();

    BfxUserData udListener;
    uint32_t result = BdtAcceptConnection(
            stack,
            vport,
            listenerWrapper->ToCCallback(),
            listenerWrapper->ToBfxUserData(jenv, jListener, &udListener)
            );
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}


/*
 * Class:     com_bucky_bdt_Connection
 * Method:    confirm
 * Signature: ([BLcom/bucky/bdt/Connection/AcceptListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_confirm
        (JNIEnv* jenv, jobject jConnection, jbyteArray jAnswer, jobject jListener)
{
    BdtConnection* connection = ConnectionWrapper::Instance()->FromObject(jenv, jConnection);
    const ConnectionWrapper::AcceptListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->AcceptListener();

    size_t answerLen = jenv->GetArrayLength(jAnswer);
    uint8_t* answer = reinterpret_cast<uint8_t*>(jenv->GetByteArrayElements(jAnswer, NULL));

    BfxUserData udListener;
    uint32_t result = BdtConfirmConnection(
            connection,
            answer,
            answerLen,
            listenerWrapper->ToCCallback(),
            listenerWrapper->ToBfxUserData(jenv, jListener, &udListener)
    );
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}


/*
 * Class:     com_bucky_bdt_Connection
 * Method:    send
 * Signature: (Lcom/bucky/bdt/Connection/SendListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_send
        (JNIEnv* jenv, jobject jConnection, jobject jListener)
{
    BdtConnection* connection = ConnectionWrapper::Instance()->FromObject(jenv, jConnection);
    const ConnectionWrapper::SendListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->SendListener();

    uint8_t* buffer = NULL;
    size_t len = 0;
    listenerWrapper->GetBuffer(jenv, jListener, &buffer, &len);

    BfxUserData udListener;
    uint32_t result = BdtConnectionSend(
            connection,
            buffer,
            len,
            listenerWrapper->ToCCallback(),
            listenerWrapper->ToBfxUserData(jenv, jListener, &udListener)
            );
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}

/*
 * Class:     com_bucky_bdt_Connection
 * Method:    recv
 * Signature: (Lcom/bucky/bdt/Connection/RecvListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Connection_recv
        (JNIEnv* jenv, jobject jConnection, jobject jListener)
{
    BdtConnection* connection = ConnectionWrapper::Instance()->FromObject(jenv, jConnection);
    const ConnectionWrapper::RecvListenerWrapper* listenerWrapper = ConnectionWrapper::Instance()->RecvListener();

    uint8_t* buffer = NULL;
    size_t len = 0;
    listenerWrapper->GetBuffer(jenv, jListener, &buffer, &len);

    BfxUserData udListener;
    uint32_t result = BdtConnectionRecv(
            connection,
            buffer,
            len,
            listenerWrapper->ToCCallback(),
            listenerWrapper->ToBfxUserData(jenv, jListener, &udListener)
    );
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}