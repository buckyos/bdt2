//
// Created by tsukasa on 2020/2/24.
//

#ifndef BDT2_STACK_H
#define BDT2_STACK_H
#include <jni.h>
#include <BdtCore.h>
#include "./Global.h"

class StackWrapper
        : public NativePointorWrapper<BdtStack>
{
public:
    static int OnLoad(JNIEnv* jenv);
    static const StackWrapper* Instance();

    class EventListenerWrapper
            : public CallbackWrapper
    {
        friend class StackWrapper;
    public:
        StackEventCallback ToCCallback() const;
    private:
        static void BFX_STDCALL CCallback(
                BdtStack* stack,
                uint32_t eventId,
                const void* eventParams,
                void* userData);
        int Initialize(JNIEnv* jenv);
        EventListenerWrapper();
        jclass m_class;
        jmethodID m_onOpen;
        jmethodID m_onError;
    };
    const EventListenerWrapper* EventListener() const;
private:
    StackWrapper();
    int Initialize(JNIEnv* jenv);

private:
    static StackWrapper* s_instance;
    EventListenerWrapper* m_listener;
};

#endif //BDT2_STACK_H
