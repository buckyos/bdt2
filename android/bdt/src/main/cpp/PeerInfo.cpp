//
// Created by tsukasa on 2020/2/17.
//
#include "./PeerInfo.h"

PeerConstInfoWrapper* PeerConstInfoWrapper::s_instance = NULL;

const PeerConstInfoWrapper* PeerConstInfoWrapper::Instance()
{
    return PeerConstInfoWrapper::s_instance;
}

int PeerConstInfoWrapper::OnLoad(JNIEnv* jenv)
{
    PeerConstInfoWrapper* instance = new PeerConstInfoWrapper();
    int result = instance->Initialize(jenv);
    if (result != BFX_RESULT_SUCCESS)
    {
        delete instance;
        return result;
    }
    s_instance = instance;
    return BFX_RESULT_SUCCESS;
}

PeerConstInfoWrapper::PeerConstInfoWrapper()
    : m_class(0)
    , m_constructor(0)
    , m_continent(0)
    , m_country(0)
    , m_carrier(0)
    , m_city(0)
    , m_inner(0)
    , m_deviceId(0)
    , m_publicKey(0)
{

}

int PeerConstInfoWrapper::Initialize(JNIEnv* jenv)
{
    jclass clazz = jenv->FindClass("com/bucky/bdt/PeerConstInfo");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_constructor = jenv->GetMethodID(this->m_class, "<init>", "()V");
    this->m_continent = jenv->GetFieldID(this->m_class, "continent", "B");
    this->m_country = jenv->GetFieldID(this->m_class, "country", "B");
    this->m_carrier = jenv->GetFieldID(this->m_class, "carrier", "B");
    this->m_city = jenv->GetFieldID(this->m_class, "city", "S");
    this->m_inner = jenv->GetFieldID(this->m_class, "inner", "B");
    this->m_deviceId = jenv->GetFieldID(this->m_class, "deviceId", "Ljava/lang/String;");
    this->m_publicKey = jenv->GetFieldID(this->m_class, "publicKey", "[B");
    return BFX_RESULT_SUCCESS;
}

const BdtPeerConstInfo* PeerConstInfoWrapper::FromObject(
    JNIEnv* jenv,
    jobject jobj,
    BdtPeerConstInfo* info) const
{
    ::memset(info, 0, sizeof(BdtPeerConstInfo));
    info->areaCode.continent = jenv->GetByteField(jobj, this->m_continent);
    info->areaCode.country = jenv->GetByteField(jobj, this->m_country);
    info->areaCode.carrier = jenv->GetByteField(jobj, this->m_carrier);
    info->areaCode.city = jenv->GetShortField(jobj, this->m_city);
    info->areaCode.inner = jenv->GetByteField(jobj, this->m_inner);

    jstring jDeviceId = static_cast<jstring>(jenv->GetObjectField(jobj, this->m_deviceId));
    size_t lenDeviceId = jenv->GetStringLength(jDeviceId);
    if (lenDeviceId >= BDT_MAX_DEVICE_ID_LENGTH)
    {
        return NULL;
    }
    const jchar* deviceId16 = jenv->GetStringChars(jDeviceId, NULL);
    size_t lenDeviceId16 = BDT_MAX_DEVICE_ID_LENGTH;
    BfxTranscodeUTF16ToUTF8(reinterpret_cast<const char16_t*>(deviceId16), &lenDeviceId, reinterpret_cast<char*>(info->deviceId), &lenDeviceId16);
    jenv->ReleaseStringChars(jDeviceId, deviceId16);

    jbyteArray jPublicKey = static_cast<jbyteArray>(jenv->GetObjectField(jobj, this->m_publicKey));
    if (jenv->GetArrayLength(jPublicKey) != BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024)
    {
        return NULL;
    }
    ::memcpy(&info->publicKey, jenv->GetByteArrayElements(jPublicKey, NULL), BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024);
    info->publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;

    return info;
}

jobject PeerConstInfoWrapper::ToObject(
    JNIEnv* jenv,
    const BdtPeerConstInfo* info) const
{
    jobject jInfo = jenv->NewObject(this->m_class, this->m_constructor);

    jenv->SetByteField(jInfo, this->m_continent, info->areaCode.country);
    jenv->SetByteField(jInfo, this->m_country, info->areaCode.country);
    jenv->SetShortField(jInfo, this->m_carrier, info->areaCode.country);
    jenv->SetByteField(jInfo, this->m_city, info->areaCode.country);
    jenv->SetByteField(jInfo, this->m_inner, info->areaCode.country);

    size_t lenDeviceId = ::strlen(reinterpret_cast<const char*>(info->deviceId));
    char16_t deviceId16[BDT_MAX_DEVICE_ID_LENGTH];
    memset(deviceId16, 0, sizeof(char16_t) * BDT_MAX_DEVICE_ID_LENGTH);
    size_t lenDeviceId16 = BDT_MAX_DEVICE_ID_LENGTH;
    BfxTranscodeUTF8ToUTF16(reinterpret_cast<const char*>(info->deviceId), &lenDeviceId, deviceId16, &lenDeviceId16);
    jstring jDeviceId = jenv->NewStringUTF(reinterpret_cast<char*>(deviceId16));
    jenv->SetObjectField(jInfo, this->m_deviceId, jDeviceId);

    jbyteArray jPublicKey = jenv->NewByteArray(BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024);
    jenv->SetByteArrayRegion(jPublicKey, 0, BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024, reinterpret_cast<const jbyte*>(&info->publicKey));
    jenv->SetObjectField(jInfo, this->m_publicKey, jPublicKey);

    return jInfo;
}

/*
 * Class:     com_bucky_bdt_PeerConstInfo
 * Method:    peerid
 * Signature: ()[B
 */
extern "C" JNIEXPORT jbyteArray JNICALL Java_com_bucky_bdt_PeerConstInfo_peerid
(JNIEnv * jenv, jobject jobj)
{
    BdtPeerConstInfo insInfo;
    const BdtPeerConstInfo* info = PeerConstInfoWrapper::Instance()->FromObject(jenv, jobj, &insInfo);
    BdtPeerid insPeerid;
    uint32_t result = BdtGetPeeridFromConstInfo(info, &insPeerid);
    jbyteArray jpeerid = jenv->NewByteArray(sizeof(insPeerid));
    jenv->SetByteArrayRegion(jpeerid, 0, sizeof(insPeerid), reinterpret_cast<jbyte*>(&insPeerid));
    return jpeerid;
}




PeerInfoWrapper* PeerInfoWrapper::s_instance = NULL;

int PeerInfoWrapper::OnLoad(JNIEnv* jenv)
{
    PeerInfoWrapper* instance = new PeerInfoWrapper();
    int result = instance->Initialize(jenv);
    if (result != BFX_RESULT_SUCCESS)
    {
        delete instance;
        return result;
    }
    s_instance = instance;
    return BFX_RESULT_SUCCESS;
}

const PeerInfoWrapper* PeerInfoWrapper::Instance()
{
    return s_instance;
}

int PeerInfoWrapper::Initialize(JNIEnv* jenv)
{
    NativePointorWrapper<const BdtPeerInfo>::Initialize(jenv);

    jclass clazz = jenv->FindClass("com/bucky/bdt/PeerInfo");
    this->m_class = static_cast<jclass>(jenv->NewGlobalRef(clazz));
    this->m_constructor = jenv->GetMethodID(clazz, "<init>", "()V");

    this->m_builder.Initialize(jenv);
    return BFX_RESULT_SUCCESS;
}

PeerInfoWrapper::PeerInfoWrapper()
    : m_class(0)
    , m_constructor(0)
{

}

const PeerInfoWrapper::BuilderWrapper* PeerInfoWrapper::Builder() const
{
    return &this->m_builder;
}

jobject PeerInfoWrapper::ToObject(
    JNIEnv* jenv,
    const BdtPeerInfo* peer) const
{
    jobject jPeer = jenv->NewObject(this->m_class, this->m_constructor);
    return this->BindObject(jenv, jPeer, peer);
}


/*
 * Class:     com_bucky_bdt_PeerInfo
 * Method:    releaseNative
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_bucky_bdt_PeerInfo_releaseNative
(JNIEnv * jenv, jobject jpeer)
{
    const BdtPeerInfo* peer = PeerInfoWrapper::Instance()->FromObject(jenv, jpeer);
    if (peer != NULL)
    {
        BdtReleasePeerInfo(peer);
    }
}



/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    init
 * Signature: (I)V
 */
extern "C" JNIEXPORT void JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_init
(JNIEnv * jenv, jobject jBuilder, jint flags)
{
    BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(flags);
    PeerInfoWrapper::Instance()->Builder()->BindObject(jenv, jBuilder, builder);
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    setConstInfo
 * Signature: (Lcom/bucky/bdt/PeerConstInfo;)Lcom/bucky/bdt/PeerInfo/Builder;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_setConstInfo
(JNIEnv * jenv, jobject jBuilder, jobject jConstInfo)
{
    BdtPeerConstInfo insConstInfo;
    const BdtPeerConstInfo* constInfo = PeerConstInfoWrapper::Instance()->FromObject(jenv, jConstInfo, &insConstInfo);
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    BdtPeerInfoSetConstInfo(builder, constInfo);
    return jBuilder;
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    setSecret
 * Signature: ([B)Lcom/bucky/bdt/PeerInfo/Builder;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_setSecret
(JNIEnv * jenv, jobject jBuilder, jbyteArray jSecret)
{
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    BdtPeerSecret insSecret;

    if (jenv->GetArrayLength(jSecret) <= BdtGetSecretKeyMaxLength(BDT_PEER_PUBLIC_KEY_TYPE_RSA1024))
    {
        insSecret.secretLength = jenv->GetArrayLength(jSecret);
        ::memcpy(&insSecret.secret, jenv->GetByteArrayElements(jSecret, NULL), insSecret.secretLength);
        BdtPeerInfoSetSecret(builder, &insSecret);
    }

    return jBuilder;
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    addEndpoint
 * Signature: (Ljava/lang/String;)Lcom/bucky/bdt/PeerInfo/Builder;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_addEndpoint
(JNIEnv * jenv, jobject jBuilder, jstring jstrEp)
{
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    BdtEndpoint ep;
    const jchar* ep16 = jenv->GetStringChars(jstrEp, NULL);
    size_t lenEp16 = jenv->GetStringLength(jstrEp);
    char ep8[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
    size_t lenEp8 = BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
    ::memset(ep8, 0, lenEp8);
    BfxTranscodeUTF16ToUTF8(reinterpret_cast<const char16_t*>(ep16), &lenEp16, ep8, &lenEp8);
    BdtEndpointFromString(&ep, reinterpret_cast<const char*>(ep8));
    jenv->ReleaseStringChars(jstrEp, ep16);
    BdtPeerInfoAddEndpoint(builder, &ep);
    return jBuilder;
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    addSn
 * Signature: ([B)Lcom/bucky/bdt/PeerInfo/Builder;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_addSn
(JNIEnv * jenv, jobject jBuilder, jbyteArray jPeerid)
{
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    const BdtPeerid* peerid = reinterpret_cast<BdtPeerid*>(jenv->GetByteArrayElements(jPeerid, NULL));
    BdtPeerInfoAddSn(builder, peerid);
    return jBuilder;
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    addSnInfo
 * Signature: (Lcom/bucky/bdt/PeerInfo;)Lcom/bucky/bdt/PeerInfo/Builder;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_addSnInfo
(JNIEnv * jenv, jobject jBuilder, jobject jPeer)
{
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    const BdtPeerInfo* peer = PeerInfoWrapper::Instance()->FromObject(jenv, jPeer);
    BdtPeerInfoAddSnInfo(builder, peer);
    return jBuilder;
}

/*
 * Class:     com_bucky_bdt_PeerInfo_Builder
 * Method:    finish
 * Signature: ()Lcom/bucky/bdt/PeerInfo;
 */
extern "C" JNIEXPORT jobject JNICALL Java_com_bucky_bdt_PeerInfo_00024Builder_finish
(JNIEnv * jenv, jobject jBuilder)
{
    BdtPeerInfoBuilder* builder = PeerInfoWrapper::Instance()->Builder()->FromObject(jenv, jBuilder);
    const BdtPeerInfo* peer = BdtPeerInfoBuilderFinish(builder, NULL);
    return PeerInfoWrapper::Instance()->ToObject(jenv, peer);
}