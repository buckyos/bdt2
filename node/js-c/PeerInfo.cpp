#include "./PeerInfo.h"

// const BdtEndpoint* Endpoint::FromObject(const Napi::Value value, BdtEndpoint* ep)
// {
//     assert(value.isObject());
//     Napi::Object objValue = value.As<Napi::Object>();
// }

const BdtEndpoint* Endpoint::FromString(const Napi::Value value, BdtEndpoint* ep)
{
    assert(value.IsString());
    std::string str = static_cast<std::string>(value.As<Napi::String>());
    uint32_t result = BdtEndpointFromString(ep, str.c_str());
    if (result != BFX_RESULT_SUCCESS)
    {
        return NULL;
    }
    return ep;
}


const BdtPeerid* Peerid::FromObject(const Napi::Value value, BdtPeerid* peerid)
{
    if (!value.IsArrayBuffer()
        || value.As<Napi::ArrayBuffer>().ByteLength() != BDT_PEERID_LENGTH)
    {
        return NULL;
    }
    if (peerid)
    {
        *peerid = *reinterpret_cast<BdtPeerid*>(value.As<Napi::ArrayBuffer>().Data());
        return peerid;
    }
    else
    {
        return reinterpret_cast<BdtPeerid*>(value.As<Napi::ArrayBuffer>().Data());
    }
}


Napi::Value Peerid::ToObject(Napi::Env env, const BdtPeerid* peerid)
{
    Napi::ArrayBuffer value = Napi::ArrayBuffer::New(env, sizeof(BdtPeerid));
    ::memcpy(value.Data(), peerid, sizeof(BdtPeerid));
    return value;
}

Napi::FunctionReference PeerConstInfo::s_constructor;

PeerConstInfo::PeerConstInfo(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<PeerConstInfo>(info)
{

}

Napi::Object PeerConstInfo::Register(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(
        env,
        "PeerConstInfo",
        {
            StaticMethod("toPeerid", &PeerConstInfo::ToPeerid),
        });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("PeerConstInfo", func);
    return exports;
}

const BdtPeerid* PeerConstInfo::ToPeerid(const Napi::Value& jsValue, BdtPeerid* peerid)
{
    BdtPeerConstInfo insInfo;
    const BdtPeerConstInfo* constInfo = PeerConstInfo::FromObject(jsValue, &insInfo);
    if (constInfo == NULL)
    {
        return NULL;
    }
    uint32_t result = BdtGetPeeridFromConstInfo(constInfo, peerid);
    return peerid;
}

Napi::Value PeerConstInfo::ToPeerid(const Napi::CallbackInfo& cb)
{
    BdtPeerid insPeerid;
    const BdtPeerid* peerid = PeerConstInfo::ToPeerid(cb[0], &insPeerid);
    if (peerid != NULL)
    {
        return Peerid::ToObject(cb.Env(), &insPeerid);
    }
    else
    {
        return cb.Env().Null();
    }
}

const BdtPeerArea* PeerArea::FromObject(const Napi::Value value, BdtPeerArea* area)
{

    assert(value.IsObject());
    Napi::Object obj = value.As<Napi::Object>();

    area->continent = (uint8_t)obj.Get("continent").ToNumber().Uint32Value();
    area->country = (uint8_t)obj.Get("country").ToNumber().Uint32Value();
    area->carrier = (uint8_t)obj.Get("carrier").ToNumber().Uint32Value();
    area->city = (uint8_t)obj.Get("city").ToNumber().Uint32Value();
    area->inner = (uint8_t)obj.Get("inner").ToNumber().Uint32Value();

    return area;
}

Napi::Value PeerArea::ToObject(Napi::Env env, const BdtPeerArea* area)
{
    Napi::Object areaJs = Napi::Object::New(env);

    areaJs.Set("continent", Napi::Number::New(env, area->continent));
    areaJs.Set("country", Napi::Number::New(env, area->country));
    areaJs.Set("carrier", Napi::Number::New(env, area->carrier));
    areaJs.Set("city", Napi::Number::New(env, area->city));
    areaJs.Set("inner", Napi::Number::New(env, area->inner));

    return areaJs;
}

const BdtPeerConstInfo* PeerConstInfo::FromObject(const Napi::Value value, BdtPeerConstInfo* info)
{
    if (value.IsNull() || value.IsUndefined())
    {
        return NULL;
    }
    ::memset(info, 0, sizeof(BdtPeerConstInfo));
    assert(value.IsObject());
    Napi::Object obj = value.As<Napi::Object>();

    Napi::Value objDeviceId = obj.Get("deviceId");
    if (objDeviceId.IsString())
    {
        std::string strDeviceId = static_cast<std::string>(objDeviceId.As<Napi::String>());
        if (strDeviceId.size() >= BDT_MAX_DEVICE_ID_LENGTH)
        {
            return NULL;
        }
        ::memcpy(info->deviceId, strDeviceId.c_str(), strDeviceId.length());
    }
    else if (objDeviceId.IsArrayBuffer())
    {
        Napi::Buffer<uint8_t> bufDeviceId = objDeviceId.As<Napi::Buffer<uint8_t> >();
        if (bufDeviceId.Length() >= BDT_MAX_DEVICE_ID_LENGTH) {
            return NULL;
        }
        ::memcpy(&info->deviceId, bufDeviceId.Data(), bufDeviceId.Length());
    }
    else
    {
        return NULL;
    }

    //TODO: enum publicKeyType
    Napi::Value objPublicKey = obj.Get("publicKey");
    if (objPublicKey.IsNull() || !objPublicKey.IsBuffer())
    {
        return NULL;
    }
    Napi::Buffer<uint8_t> bufPublicKey = objPublicKey.As<Napi::Buffer<uint8_t> >();
    if (bufPublicKey.Length() != BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024)
    {
        return NULL;
    }
    info->publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;
    ::memcpy(&info->publicKey, bufPublicKey.Data(), bufPublicKey.Length());
    return info;
}

Napi::Value PeerConstInfo::ToObject(Napi::Env env, const BdtPeerConstInfo* constInfo)
{
    Napi::Object constInfoJs = Napi::Object::New(env);

    constInfoJs.Set("areaCode", PeerArea::ToObject(env, &constInfo->areaCode));
    constInfoJs.Set("deviceId", Napi::Buffer<uint8_t>::New(env,
        (uint8_t*)constInfo->deviceId, BDT_MAX_DEVICE_ID_LENGTH));
    constInfoJs.Set("publicKeyType", Napi::Number::New(env, constInfo->publicKeyType));
    constInfoJs.Set("publicKey", Napi::Buffer<uint8_t>::New(env,
        (uint8_t*)constInfo->publicKey.rsa1024,
        BdtGetPublicKeyLength(constInfo->publicKeyType)));

    return constInfoJs;
}

const BdtPeerInfo* PeerInfo::FromObject(const Napi::Value value)
{
    assert(value.IsObject());
    Napi::Object obj = value.As<Napi::Object>();

    BdtPeerConstInfo insConstInfo;
    const BdtPeerConstInfo* constInfo = PeerConstInfo::FromObject(obj.Get("constInfo"), &insConstInfo);
    if (constInfo == NULL)
    {
        return NULL;
    }

    uint32_t infoFlags = 0;
    Napi::Value objSecret = obj.Get("secret");
    BdtPeerSecret insSecret;
    const BdtPeerSecret* secret = NULL;
    if (!objSecret.IsNull() && !objSecret.IsUndefined())
    {
        if (!objSecret.IsBuffer())
        {
            return NULL;
        }
        //TODO: enum publicKeyType
        Napi::Buffer<uint8_t> bufSecret = objSecret.As<Napi::Buffer<uint8_t> >();
        if (bufSecret.Length() > BdtGetSecretKeyMaxLength(BDT_PEER_PUBLIC_KEY_TYPE_RSA1024))
        {
            return NULL;
        }
        insSecret.secretLength = (uint16_t)bufSecret.Length();
        ::memcpy(&insSecret.secret, bufSecret.Data(), bufSecret.Length());
        infoFlags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET;
        secret = &insSecret;
    }

    Napi::Value objEpList = obj.Get("endpoints");
    if (!objEpList.IsArray())
    {
        return NULL;
    }
    Napi::Array arrEpList = objEpList.As<Napi::Array>();

    Napi::Value objSnList = obj.Get("supernodes");
    if (objSnList.IsArray())
    {
        infoFlags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST;
        infoFlags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO;
    }

    BdtPeerInfoBuilder* builder = BdtPeerInfoBuilderBegin(infoFlags);
    BdtPeerInfoSetConstInfo(builder, constInfo);
    if (secret != NULL)
    {
        BdtPeerInfoSetSecret(builder, secret);
    }
    for (uint32_t ix = 0; ix < arrEpList.Length(); ++ix)
    {
        BdtEndpoint insEp;
        const BdtEndpoint* ep = Endpoint::FromString(arrEpList[ix], &insEp);
        BdtPeerInfoAddEndpoint(builder, ep);
    }
    if (infoFlags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST
        || infoFlags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO)
    {
        for (uint32_t ix = 0; ix < objSnList.As<Napi::Array>().Length(); ++ix)
        {
            Napi::Value elem = objSnList.As<Napi::Object>()[ix];
            if (elem.IsArrayBuffer())
            {
                BdtPeerInfoAddSn(builder, Peerid::FromObject(elem));
            }
            else if (elem.IsObject())
            {
                const BdtPeerInfo* snInfo = PeerInfo::FromObject(elem);
                BdtPeerInfoAddSnInfo(builder, snInfo);
                BdtReleasePeerInfo(snInfo);
            }
        }
    }
    return BdtPeerInfoBuilderFinish(builder, NULL);
}

Napi::Value PeerInfo::ToObject(Napi::Env env, const BdtPeerInfo* peerinfo)
{
    Napi::Object peerInfoJs = Napi::Object::New(env);

    //uint32_t flags;
    //BdtPeerid peerid;
    //BdtPeerConstInfo constInfo;

    //BdtEndpoint* endpoints;
    //size_t endpointLength;
    //BdtPeerid* snList;
    //size_t snListLength;
    //const BdtPeerInfo** snInfoList;
    //size_t snInfoListLength;
    //BdtPeerSecret* secret;
    //uint8_t signature[BDT_PEER_LENGTH_SIGNATRUE];
    //uint64_t createTime;

    Napi::Value peeridJs = Peerid::ToObject(env, BdtPeerInfoGetPeerid(peerinfo));
    peerInfoJs.Set("peerid", peeridJs);
    peerInfoJs.Set("constInfo", PeerConstInfo::ToObject(env, BdtPeerInfoGetConstInfo(peerinfo)));

    size_t count = 0;
    const BdtEndpoint* endpoints = BdtPeerInfoListEndpoint(peerinfo, &count);
    Napi::Array endpointsJs = Napi::Array::New(env, count);
    char ep[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
    for (size_t i = 0; i < count; i++)
    {
        BdtEndpointToString(endpoints + i, ep);
        endpointsJs.Set((uint32_t)i, Napi::String::New(env, ep));
    }
    peerInfoJs.Set("endpoints", endpointsJs);

    const BdtPeerid* snList = BdtPeerInfoListSn(peerinfo, &count);
    size_t snInfoCount = 0;
    const BdtPeerInfo** snInfoList = BdtPeerInfoListSnInfo(peerinfo, &snInfoCount);
    Napi::Array snListJs = Napi::Array::New(env, count + snInfoCount);
    for (size_t i = 0; i < count; i++)
    {
        snListJs.Set((uint32_t)i, Peerid::ToObject(env, snList + i));
    }
    for (size_t i = 0; i < snInfoCount; i++)
    {
        snListJs.Set((uint32_t)(count + i), PeerInfo::ToObject(env, snInfoList[i]));
    }
    peerInfoJs.Set("supernodes", snListJs);

    const BdtPeerSecret* secret = BdtPeerInfoGetSecret(peerinfo);
    if (secret != NULL)
    {
        peerInfoJs.Set("secret", Napi::Buffer<uint8_t>::New(env, (uint8_t*)&secret->secret, secret->secretLength));
    }

    //const uint8_t* signature = BdtPeerInfoGetSignature(peerinfo, &count);
    //if (signature != NULL)
    //{
    //    peerInfoJs.Set("signature", Napi::Buffer<uint8_t>::New(env, signature, count));
    //}

    //uint64_t createTime = BdtPeerInfoGetCreateTime(peerinfo);
    //peerInfoJs.Set("createTime", Napi::BigInt::New(env, createTime));

    return peerInfoJs;
}

BdtBuildTunnelParams* BuildTunnelParams::FromObject(const Napi::Value value)
{
    assert(value.IsObject());
    BdtBuildTunnelParams* params = new BdtBuildTunnelParams();
    ::memset(params, 0, sizeof(BdtBuildTunnelParams));
    if (value.IsObject())
    {
        BdtPeerConstInfo* constInfo = new BdtPeerConstInfo();
        if (PeerConstInfo::FromObject(value.As<Napi::Object>().Get("constInfo"), constInfo) != NULL)
        {
            params->remoteConstInfo = constInfo;
        }
        else
        {
            delete constInfo;
        }

        Napi::Value objSnList = value.As<Napi::Object>().Get("supernodes");
        if (objSnList.IsArray())
        {
            uint32_t snListLen = objSnList.As<Napi::Array>().Length();
            BdtPeerid* snList = new BdtPeerid[snListLen];
            for (uint32_t ix = 0; ix < snListLen; ++ix)
            {
                if (Peerid::FromObject(objSnList.As<Napi::Object>()[ix], snList + ix))
                {
                    ++params->snListLength;
                }
            }
            if (params->snListLength)
            {
                params->flags |= BDT_BUILD_TUNNEL_PARAMS_FLAG_SN;
                params->snList = snList;
            }
            else
            {
                delete snList;
            }
        }
    }
    return params;
}

void BuildTunnelParams::Destroy(BdtBuildTunnelParams* params)
{
    if (params->remoteConstInfo)
    {
        delete params->remoteConstInfo;
    }
    if (params->snList)
    {
        delete params->snList;
    }
    delete params;
}