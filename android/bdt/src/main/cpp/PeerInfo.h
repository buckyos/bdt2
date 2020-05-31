//
// Created by tsukasa on 2020/2/17.
//
#ifndef BDT2_PEERINFO_H
#define BDT2_PEERINFO_H
#include <jni.h>
#include <BdtCore.h>
#include "./Global.h"

//public class PeerConstInfo {
    //public byte continent;
    //public byte country;
    //public byte carrier;
    //public short city;
    //public byte inner;
    //public String deviceId;
    //public byte[] publicKey;
    //public native byte[] peerid();
//}

class PeerConstInfoWrapper
{
public:
    static int OnLoad(JNIEnv* jenv);
    static const PeerConstInfoWrapper* Instance();

public:
    const BdtPeerConstInfo* FromObject(
            JNIEnv* jenv,
            jobject jobj,
            BdtPeerConstInfo* info) const;
    jobject ToObject(
            JNIEnv* jenv,
            const BdtPeerConstInfo* info) const;
private:
    PeerConstInfoWrapper();
    int Initialize(JNIEnv* jenv);
private:
    static PeerConstInfoWrapper* s_instance;
    jclass m_class;
    jmethodID m_constructor;
    jfieldID m_continent;
    jfieldID m_country;
    jfieldID m_carrier;
    jfieldID m_city;
    jfieldID m_inner;
    jfieldID m_deviceId;
    jfieldID m_publicKey;
};


class PeerInfoWrapper
        : public NativePointorWrapper<const BdtPeerInfo>
{
    class BuilderWrapper
            : public NativePointorWrapper<BdtPeerInfoBuilder>
    {
        friend class PeerInfoWrapper;
    };

public:
    static int OnLoad(JNIEnv* jenv);
    static const PeerInfoWrapper* Instance();
    const BuilderWrapper* Builder() const;
    jobject ToObject(JNIEnv* jenv, const BdtPeerInfo* peer) const;
private:
    PeerInfoWrapper();
    int Initialize(JNIEnv* jenv);

private:
    static PeerInfoWrapper* s_instance;
    BuilderWrapper m_builder;
    jclass m_class;
    jmethodID m_constructor;
};


#endif //BDT2_PEERINFO_H
