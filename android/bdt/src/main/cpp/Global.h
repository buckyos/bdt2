//
// Created by tsukasa on 2020/2/20.
//

#ifndef BDT2_GLOBAL_H
#define BDT2_GLOBAL_H

#include <jni.h>
#include <BdtCore.h>

template <typename type>
class NativePointorWrapper
{
public:
    jobject BindObject(
            JNIEnv* jenv,
            jobject jobj,
            type* ptr) const
    {
        jenv->SetLongField(jobj, this->m_nativePtr, reinterpret_cast<jlong>(ptr));
        return jobj;
    }

    type* FromObject(
            JNIEnv* jenv,
            jobject jobj) const
    {
        return reinterpret_cast<type*>(jenv->GetLongField(jobj, this->m_nativePtr));
    }

protected:
    virtual ~NativePointorWrapper()
    {

    }
    int Initialize(JNIEnv *jenv)
    {
        jclass cls = jenv->FindClass("com/bucky/bdt/NativePointerObject");
        this->m_nativePtr = jenv->GetFieldID(cls, "nativePtr", "J");
        return BFX_RESULT_SUCCESS;
    }

private:
    jfieldID m_nativePtr;
};

class GlobalEnv
{
public:
    static jint OnLoad(JavaVM* jvm);
    static GlobalEnv* Instance();

    JNIEnv* GetJNIEnv();
private:
    GlobalEnv();
    static GlobalEnv* s_instance;
    JavaVM* m_jvm;
};

class CallbackWrapper
{
public:
    // return 0 ref; should not release once after this call
    BfxUserData* ToBfxUserData(
            JNIEnv* jenv,
            jobject jListener,
            BfxUserData* userData) const;
protected:
    jobject FromBfxUserData(void* userData) const;
private:
    struct RefListener
    {
        jobject jListener;
        volatile int32_t refCount;
    };
    static void BFX_STDCALL AddRef(void* userData);
    static void BFX_STDCALL Release(void* userData);
};


#endif //BDT2_GLOBAL_H
