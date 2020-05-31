//
// Created by tsukasa on 2020/2/20.
//
#include "./Global.h"
#include "./PeerInfo.h"
#include "./Stack.h"
#include "./Connection.h"


extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    GlobalEnv::OnLoad(vm);
    JNIEnv* env = GlobalEnv::Instance()->GetJNIEnv();
    PeerConstInfoWrapper::OnLoad(env);
    PeerInfoWrapper::OnLoad(env);
    StackWrapper::OnLoad(env);
    ConnectionWrapper::OnLoad(env);
    return JNI_VERSION_1_4;
}

jint GlobalEnv::OnLoad(JavaVM* jvm)
{
    GlobalEnv* instance = new GlobalEnv();
    instance->m_jvm = jvm;
    s_instance = instance;
    return BFX_RESULT_SUCCESS;
}

GlobalEnv* GlobalEnv::Instance()
{
    return s_instance;
}

GlobalEnv::GlobalEnv()
    : m_jvm(NULL)
{

}

JNIEnv* GlobalEnv::GetJNIEnv()
{
    //TO FIX: when to detach thread
    JNIEnv* jenv = NULL;
    jint result = this->m_jvm->GetEnv(reinterpret_cast<void**>(&jenv), JNI_VERSION_1_4);
    if (result == JNI_OK)
    {
        return jenv;
    }
    if (result == JNI_EDETACHED)
    {
        JavaVMAttachArgs args = { JNI_VERSION_1_4, NULL, NULL};
        result = this->m_jvm->AttachCurrentThread(&jenv, &args);
        if (result == JNI_OK)
        {
            return jenv;
        }
    }
    return NULL;
}

GlobalEnv* GlobalEnv::s_instance = NULL;

void BFX_STDCALL CallbackWrapper::AddRef(void* userData)
{
    RefListener* rl = reinterpret_cast<RefListener*>(userData);
    BfxAtomicInc32(&rl->refCount);
}

void BFX_STDCALL CallbackWrapper::Release(void* userData)
{
    RefListener* rl = reinterpret_cast<RefListener*>(userData);
    int32_t refCount = BfxAtomicDec32(&rl->refCount);
    if (refCount <= 0)
    {
        JNIEnv* jenv = GlobalEnv::Instance()->GetJNIEnv();
        jenv->DeleteGlobalRef(rl->jListener);
        delete rl;
    }
}

BfxUserData* CallbackWrapper::ToBfxUserData(
        JNIEnv* jenv, jobject jListener, BfxUserData* userData) const
{
    RefListener* rl = new RefListener();
    rl->refCount = 1;
    rl->jListener = jenv->NewGlobalRef(jListener);
    userData->userData = rl;
    userData->lpfnAddRefUserData = AddRef;
    userData->lpfnReleaseUserData = Release;
    return userData;
}

jobject CallbackWrapper::FromBfxUserData(void* userData) const
{
    RefListener* rl = reinterpret_cast<RefListener*>(userData);
    return rl->jListener;
}

