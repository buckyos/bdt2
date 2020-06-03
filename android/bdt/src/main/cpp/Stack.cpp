//
// Created by tsukasa on 2020/2/17.
//
#include "./Stack.h"
#include "./PeerInfo.h"
#include <BuckyFramework/framework.h>

StackWrapper* StackWrapper::s_instance = NULL;

const StackWrapper* StackWrapper::Instance()
{
    return StackWrapper::s_instance;
}

int StackWrapper::OnLoad(JNIEnv* jenv)
{
    StackWrapper* instance = new StackWrapper();
    int result = instance->Initialize(jenv);
    if (result != BFX_RESULT_SUCCESS)
    {
        delete instance;
        return result;
    }
    s_instance = instance;
    return BFX_RESULT_SUCCESS;
}


StackWrapper::StackWrapper()
: m_listener(new EventListenerWrapper())
{

}

int StackWrapper::Initialize(JNIEnv* jenv)
{
    NativePointorWrapper<BdtStack>::Initialize(jenv);
    this->m_listener->Initialize(jenv);
    return BFX_RESULT_SUCCESS;
}

StackEventCallback StackWrapper::EventListenerWrapper::ToCCallback() const
{
    return CCallback;
}


void BFX_STDCALL StackWrapper::EventListenerWrapper::CCallback(
        BdtStack* stack,
        uint32_t eventId,
        const void* eventParams,
        void* userData)
{
    const EventListenerWrapper* listenerWrapper = StackWrapper::Instance()->EventListener();
    jobject jListener =  listenerWrapper->FromBfxUserData(userData);
    JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
    if (eventId == BDT_STACK_EVENT_CREATE)
    {
        jenv->CallVoidMethod(listenerWrapper->m_class, listenerWrapper->m_onOpen);
    }
    else if (eventId == BDT_STACK_EVENT_ERROR)
    {
        jenv->CallVoidMethod(listenerWrapper->m_class, listenerWrapper->m_onError, *reinterpret_cast<const jint*>(eventParams));
    }
}

StackWrapper::EventListenerWrapper::EventListenerWrapper()
: m_class(0)
, m_onOpen(0)
, m_onError(0)
{

}


int StackWrapper::EventListenerWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/Stack$EventListener");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_onOpen = jenv->GetMethodID(clazz, "onOpen", "()V");
    this->m_onError = jenv->GetMethodID(clazz, "onError", "(I)V");
    return BFX_RESULT_SUCCESS;
}

const StackWrapper::EventListenerWrapper* StackWrapper::EventListener() const
{
    return this->m_listener;
}

/*
 * Class:     com_bucky_bdt_Stack
 * Method:    open
 * Signature: (Lcom/bucky/bdt/PeerInfo;[Lcom/bucky/bdt/PeerInfo;Lcom/bucky/bdt/Stack/EventListener;)I
 */
extern "C" JNIEXPORT jint JNICALL Java_com_bucky_bdt_Stack_open
        (JNIEnv* jenv, jobject jStack, jobject jLocalPeer, jobjectArray jInitialPeers, jobject jListener)
{
    const BdtPeerInfo* localPeer = PeerInfoWrapper::Instance()->FromObject(jenv, jLocalPeer);
    size_t initialCount = jenv->GetArrayLength(jInitialPeers);
    const BdtPeerInfo** initial = new const BdtPeerInfo*[initialCount];
    for (jsize ix = 0; ix < initialCount; ++ix)
    {
        *(initial + ix) = PeerInfoWrapper::Instance()->FromObject(jenv, jenv->GetObjectArrayElement(jInitialPeers, ix));
    }

    BuckyFrameworkOptions options;
    options.size = sizeof(BuckyFrameworkOptions);
    options.threadCount = 1;

    BdtSystemFrameworkEvents events;
    events.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent;
    events.bdtPushTcpSocketData = BdtPushTcpSocketData;
    events.bdtPushUdpSocketData = BdtPushUdpSocketData;
    BdtSystemFramework* framework = createBuckyFramework(&events, &options);

    BfxUserData udListener;
    const StackWrapper* stackWrapper = StackWrapper::Instance();
    const StackWrapper::EventListenerWrapper* listenerWrapper = stackWrapper->EventListener();

    BdtStack* stack = NULL;
    jint result = BdtCreateStack(
            framework,
            localPeer,
            initial,
            initialCount,
            NULL,
            listenerWrapper->ToCCallback(),
            listenerWrapper->ToBfxUserData(jenv, jListener, &udListener),
            &stack);
    stackWrapper->BindObject(jenv, jStack, stack);
    udListener.lpfnReleaseUserData(udListener.userData);
    return result;
}

/*
 * Class:     com_bucky_bdt_Stack
 * Method:    close
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_bucky_bdt_Stack_close
(JNIEnv* jenv, jobject jStack)
{
    BdtStack* stack =  StackWrapper::Instance()->FromObject(jenv, jStack);
    BdtCloseStack(stack);
}

