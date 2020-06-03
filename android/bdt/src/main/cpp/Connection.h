//
// Created by tsukasa on 2020/2/25.
//

#ifndef BDT2_CONNECTION_H
#define BDT2_CONNECTION_H
#include <jni.h>
#include "./Global.h"

class ConnectionWrapper
        : public NativePointorWrapper<BdtConnection>
{
public:
    static int OnLoad(JNIEnv* jenv);
    static const ConnectionWrapper* Instance();
    jobject ToObject(JNIEnv* jenv, BdtConnection* connection) const;

    class ConnectListenerWrapper
            : public CallbackWrapper
    {
        friend class ConnectionWrapper;
    public:
        void ToCCallback(
                BdtConnectionFirstAnswerCallback* cbAnswer,
                BdtConnectionConnectCallback* cbConnect) const;
    private:
        static void BFX_STDCALL AnswerCallback(
                const uint8_t* data,
                size_t len,
                void* userData);

        static void BFX_STDCALL ConnectCallback(
                BdtConnection* connection,
                uint32_t result,
                void* userData);
    private:
        int Initialize(JNIEnv* jenv);
        ConnectListenerWrapper();
        jclass m_class;
        jmethodID m_onEstablish;
        jmethodID m_onAnswer;
        jmethodID m_onError;
    };
    const ConnectListenerWrapper* ConnectListener() const;

    class PreAcceptListenerWrapper
            : public CallbackWrapper
    {
        friend class ConnectionWrapper;
    public:
        BdtAcceptConnectionCallback ToCCallback() const;
    private:
        static void BFX_STDCALL CCallback(
            uint32_t result,
            const uint8_t* buffer,
            size_t recv,
            BdtConnection* connection,
            void* userData);

    private:
        int Initialize(JNIEnv* jenv);
        PreAcceptListenerWrapper();
        jclass m_class;
        jmethodID m_onConnection;
        jmethodID m_onError;
    };
    const PreAcceptListenerWrapper* PreAcceptListener() const;

    class AcceptListenerWrapper
            : public CallbackWrapper
    {
        friend class ConnectionWrapper;
    public:
        BdtConnectionConnectCallback ToCCallback() const;
    private:
        static void BFX_STDCALL CCallback(
                BdtConnection* connection,
                uint32_t result,
                void* userData);

    private:
        int Initialize(JNIEnv* jenv);
        AcceptListenerWrapper();
        jclass m_class;
        jmethodID m_onEstablish;
        jmethodID m_onError;
    };
    const AcceptListenerWrapper* AcceptListener() const;

    class SendListenerWrapper
            : public CallbackWrapper
    {
        friend class ConnectionWrapper;
    public:
        BdtConnectionSendCallback ToCCallback() const;
        void GetBuffer(JNIEnv* jenv, jobject jListener, uint8_t** outBuffer, size_t* outLen) const;
    private:
        static void BFX_STDCALL CCallback(
            uint32_t result,
            const uint8_t* buffer,
            size_t length,
            void* userData);

    private:
        int Initialize(JNIEnv* jenv);
        SendListenerWrapper();
        jclass m_class;
        jmethodID m_onSent;
        jmethodID m_getBuffer;
    };
    const SendListenerWrapper* SendListener() const;

    class RecvListenerWrapper
            : public CallbackWrapper
    {
        friend class ConnectionWrapper;
    public:
        BdtConnectionRecvCallback ToCCallback() const;
        void GetBuffer(JNIEnv* jenv, jobject jListener, uint8_t** outBuffer, size_t* outLen) const;
    private:
        static void BFX_STDCALL CCallback(
                uint8_t* buffer,
                size_t recv,
                void* userData);

    private:
        int Initialize(JNIEnv* jenv);
        RecvListenerWrapper();
        jclass m_class;
        jmethodID m_onRecv;
        jmethodID m_getBuffer;
    };
    const RecvListenerWrapper* RecvListener() const;
private:
    ConnectionWrapper();
    int Initialize(JNIEnv* jenv);

private:
    static ConnectionWrapper* s_instance;
    ConnectListenerWrapper* m_connectListener;
    PreAcceptListenerWrapper* m_preAcceptListener;
    AcceptListenerWrapper* m_acceptListener;
    SendListenerWrapper* m_sendListener;
    RecvListenerWrapper* m_recvListener;

    jclass m_class;
    jmethodID m_constructor;
};


#endif //BDT2_CONNECTION_H
