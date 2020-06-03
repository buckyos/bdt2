#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./PeerInfo.h"
#include <assert.h>
#include <BuckyBase/BuckyBase.h>
#include "../BdtCore.h"
#include "../Protocol/Package.h"
#include "./CryptoToolKit.h"

#define BDT_PEER_INFO_FLAG_SIGN_UPDATE			1

static uint32_t s_bdtKeyLengthMap[][3] = {
    {BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024, sizeof(((BdtPeerSecret*)0)->secret), 1024},
    {BDT_PEER_PUBLIC_KEY_LENTGH_RSA2048, sizeof(((BdtPeerSecret*)0)->secret), 2048},
    {BDT_PEER_PUBLIC_KEY_LENTGH_RSA3072, sizeof(((BdtPeerSecret*)0)->secret), 3072},
};

struct BdtPeerInfo
{
    int refCount;
    uint32_t flags;
    BdtPeerid peerid;
    BdtPeerConstInfo constInfo;

    BdtEndpoint* endpoints;
    size_t endpointLength;

    BdtPeerid* snList;
    size_t snListLength;

    const BdtPeerInfo** snInfoList; //TODO:不应该在这里保存PeerInfo,而是通过Peerid->Peerinfo的cache
    size_t snInfoListLength;

    BdtPeerSecret* secret;
    uint8_t signature[BDT_PEER_LENGTH_SIGNATRUE];
    uint64_t createTime;
};

static BdtPeerInfo* CreatePeerInfo()
{
    BdtPeerInfo* result = BFX_MALLOC_OBJ(BdtPeerInfo);
    memset(result, 0, sizeof(BdtPeerInfo));
    result->refCount = 1;
    result->constInfo.publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;
    return result;
}

static uint32_t BdtPeerInfoWriteSignFieldToBuffer(const BdtPeerInfo* peerInfo,
    BOOL isWillResign,
    BfxBufferStream* stream,
    size_t* writeBytes,
    size_t* signStartPos);
static uint32_t BdtPeerInfoReadSignFieldFromBuffer(BdtPeerInfo* peerInfo,
    uint32_t flags,
    BfxBufferStream* stream,
    size_t* readBytes,
    size_t* signStartPos);
static uint32_t BdtPeerInfoSign(
    const BdtPeerSecret* signSecret,
    BdtPeerInfo* peerInfo,
    const uint8_t* buffer,
    size_t bufferBytes);
static uint32_t BdtPeerInfoVerify(const BdtPeerInfo* peerInfo,
    const uint8_t* buffer,
    size_t bufferBytes);
static uint32_t BdtPeerInfoWriteEndpoint(const BdtEndpoint* endpoint,
    uint32_t flag,
    BfxBufferStream* stream,
    size_t* writeBytes);
static uint32_t BdtPeerInfoWriteEndpointsWithoutFlag(const BdtEndpoint* endpoints,
    uint8_t endpointCount,
    uint32_t flag,
    BfxBufferStream* stream,
    size_t* writeBytes);

uint32_t BufferWritePeerInfo(BfxBufferStream* bufferStream, const BdtPeerInfo* peerInfo, size_t* pWriteBytes)
{
    uint8_t flags = BDT_PEER_INFO_BUILD_FLAG_NULL;
    size_t writeBytes = 0;
    size_t startPos = BfxBufferStreamGetPos(bufferStream);
    *pWriteBytes = 0;
    int r = 0;

    if (peerInfo == NULL)
    {
        r = BfxBufferWriteUInt8(bufferStream, flags, &writeBytes);
        if (r != BFX_RESULT_SUCCESS)
        {
            return r;
        }
        *pWriteBytes = 1;
        return BFX_RESULT_SUCCESS;
    }
    else
    {
        flags = 0;
    }

    if (peerInfo->snInfoList)
    {
        flags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO;
    }
    else if (peerInfo->snList)
    {
        flags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST;
    }
    if ((peerInfo->flags & BDT_PEER_INFO_FLAG_SIGNED) || peerInfo->secret != NULL)
    {
        flags |= BDT_PEER_INFO_FLAG_SIGNED;
    }

    r = BfxBufferWriteUInt8(bufferStream, flags, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt8(bufferStream, peerInfo->constInfo.areaCode.continent, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt8(bufferStream, peerInfo->constInfo.areaCode.country, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt8(bufferStream, peerInfo->constInfo.areaCode.carrier, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt16(bufferStream, peerInfo->constInfo.areaCode.city, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt8(bufferStream, peerInfo->constInfo.areaCode.inner, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    //====================================================================
    r = BfxBufferWriteByteArray(bufferStream, peerInfo->constInfo.deviceId, BDT_MAX_DEVICE_ID_LENGTH);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    r = BfxBufferWriteUInt8(bufferStream, peerInfo->constInfo.publicKeyType, &writeBytes);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }
    writeBytes = 0;

    switch (peerInfo->constInfo.publicKeyType)
    {
    case BDT_PEER_PUBLIC_KEY_TYPE_RSA1024:
        r = BfxBufferWriteByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa1024, BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024);
        if (r != BFX_RESULT_SUCCESS)
        {
            return r;
        }
        writeBytes = 0;
        break;
    case BDT_PEER_PUBLIC_KEY_TYPE_RSA2048:
        r = BfxBufferWriteByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa2048, BDT_PEER_PUBLIC_KEY_LENTGH_RSA2048);
        if (r != BFX_RESULT_SUCCESS)
        {
            return r;
        }
        writeBytes = 0;
        break;
    case BDT_PEER_PUBLIC_KEY_TYPE_RSA3072:
        r = BfxBufferWriteByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa3072, BDT_PEER_PUBLIC_KEY_LENTGH_RSA3072);
        if (r != BFX_RESULT_SUCCESS)
        {
            return r;
        }
        writeBytes = 0;
        break;
    default:
        BLOG_WARN("encode peerinfo error,unknown public_key_type.\n");
        return BFX_RESULT_UNKNOWN_TYPE;
    }

    size_t signStartPos = 0;
    r = BdtPeerInfoWriteSignFieldToBuffer(peerInfo, FALSE, bufferStream, &writeBytes, &signStartPos);
    if (r != BFX_RESULT_SUCCESS)
    {
        return r;
    }

    if (flags & BDT_PEER_INFO_FLAG_SIGNED)
    {
        r = BfxBufferWriteByteArray(bufferStream, peerInfo->signature, sizeof(peerInfo->signature));
        if (r != BFX_RESULT_SUCCESS)
        {
            return r;
        }
    }
    *pWriteBytes = BfxBufferStreamGetPos(bufferStream) - startPos;
    return BFX_RESULT_SUCCESS;
}

typedef struct EndpointVector
{
    BdtEndpoint* list;
    size_t size;
    size_t allocSize;
} EndpointVector, endpoint_vector;
BFX_VECTOR_DEFINE_FUNCTIONS(endpoint_vector, BdtEndpoint, list, size, allocSize)

typedef struct PeeridVector
{
    BdtPeerid* list;
    size_t size;
    size_t allocSize;
} PeeridVector, peerid_vector;
BFX_VECTOR_DEFINE_FUNCTIONS(peerid_vector, BdtPeerid, list, size, allocSize)

typedef struct PeerInfoVector
{
    BdtPeerInfo** list;
    size_t size;
    size_t allocSize;
} PeerInfoVector, peerinfo_vector;
BFX_VECTOR_DEFINE_FUNCTIONS(peerinfo_vector, BdtPeerInfo*, list, size, allocSize)

int BufferReadPeerInfo(BfxBufferStream* bufferStream, const BdtPeerInfo** pResult)
{
    BdtPeerInfo* peerInfo = NULL;
    int readlen = 0;
    uint32_t ret = BFX_RESULT_PARSE_FAILED;
    size_t startPos = BfxBufferStreamGetPos(bufferStream);

    do
    {
        uint8_t flags = 0;
        readlen = BfxBufferReadUInt8(bufferStream, &(flags));
        if (readlen <= 0)
        {
            break;
        }

        if (flags == BDT_PEER_INFO_BUILD_FLAG_NULL)
        {
            ret = BFX_RESULT_SUCCESS;
            break;
        }
        peerInfo = CreatePeerInfo();
        readlen = BfxBufferReadUInt8(bufferStream, &(peerInfo->constInfo.areaCode.continent));
        if (readlen <= 0)
        {
            break;
        }

        readlen = BfxBufferReadUInt8(bufferStream, &(peerInfo->constInfo.areaCode.country));
        if (readlen <= 0)
        {
            break;
        }

        readlen = BfxBufferReadUInt8(bufferStream, &(peerInfo->constInfo.areaCode.carrier));
        if (readlen <= 0)
        {
            break;
        }

        readlen = BfxBufferReadUInt16(bufferStream, &(peerInfo->constInfo.areaCode.city));
        if (readlen <= 0)
        {
            break;
        }

        readlen = BfxBufferReadUInt8(bufferStream, &(peerInfo->constInfo.areaCode.inner));
        if (readlen <= 0)
        {
            break;
        }
        //----------------------------------------------------------------------------
        readlen = BfxBufferReadByteArray(bufferStream, peerInfo->constInfo.deviceId, BDT_MAX_DEVICE_ID_LENGTH);
        if (readlen <= 0)
        {
            break;
        }

        readlen = BfxBufferReadUInt8(bufferStream, &(peerInfo->constInfo.publicKeyType));
        if (readlen <= 0)
        {
            break;
        }

        if (peerInfo->constInfo.publicKeyType == BDT_PEER_PUBLIC_KEY_TYPE_RSA1024)
        {
            readlen = BfxBufferReadByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa1024, BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024);
            if (readlen <= 0)
            {
                break;
            }
        }
        else if (peerInfo->constInfo.publicKeyType == BDT_PEER_PUBLIC_KEY_TYPE_RSA2048)
        {
            readlen = BfxBufferReadByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa2048, BDT_PEER_PUBLIC_KEY_LENTGH_RSA2048);
            if (readlen <= 0)
            {
                break;
            }
        }
        else if (peerInfo->constInfo.publicKeyType == BDT_PEER_PUBLIC_KEY_TYPE_RSA3072)
        {
            readlen = BfxBufferReadByteArray(bufferStream, peerInfo->constInfo.publicKey.rsa3072, BDT_PEER_PUBLIC_KEY_LENTGH_RSA3072);
            if (readlen <= 0)
            {
                break;
            }
        }
        else
        {
            BLOG_WARN("read peerinfo error,unknown public_key_type.");
            ret = BFX_RESULT_UNKNOWN_TYPE;
            break;
        }

        size_t readSize = 0;
        size_t signStartPos = 0;
        ret = BdtPeerInfoReadSignFieldFromBuffer(peerInfo, flags, bufferStream, &readSize, &signStartPos);
        if (ret != BFX_RESULT_SUCCESS)
        {
            break;
        }
        ret = BFX_RESULT_PARSE_FAILED;
        if (flags & BDT_PEER_INFO_FLAG_SIGNED)
        {
            assert(signStartPos > 0);
            size_t signSize = BfxBufferStreamGetPos(bufferStream) - signStartPos;
            readlen = BfxBufferReadByteArray(bufferStream, peerInfo->signature, sizeof(peerInfo->signature));
            if (readlen <= 0)
            {
                break;
            }
            ret = BdtPeerInfoVerify(peerInfo, bufferStream->buffer + signStartPos, signSize);
        }
        else
        {
            ret = BFX_RESULT_SUCCESS;
        }
    } while (false);

    if (ret)
    {
        if (peerInfo)
        {
            BdtReleasePeerInfo(peerInfo);
        }
        *pResult = NULL;
        return -1;
    }
    else
    {
        if (peerInfo)
        {
            BdtGetPeeridFromConstInfo(&peerInfo->constInfo, &peerInfo->peerid);
        }
        *pResult = peerInfo;
    }
    return (int)(BfxBufferStreamGetPos(bufferStream) - startPos);
}

struct BdtPeerInfoBuilder
{
    uint32_t flags;
    BdtPeerInfo* result;
    EndpointVector endpoints;
    PeeridVector snList;
    PeerInfoVector snInfoList;
    uint64_t createTime;
};

BDT_API(BdtPeerInfoBuilder*) BdtPeerInfoBuilderBegin(uint32_t flags)
{
    BdtPeerInfoBuilder* builder = BFX_MALLOC_OBJ(BdtPeerInfoBuilder);
    memset(builder, 0, sizeof(BdtPeerInfoBuilder));
    builder->flags = flags;
    bfx_vector_endpoint_vector_init(&builder->endpoints);
    bfx_vector_peerid_vector_init(&builder->snList);
    bfx_vector_peerinfo_vector_init(&builder->snInfoList);

    builder->result = CreatePeerInfo();

    return builder;
}

BDT_API(void) BdtPeerInfoClone(BdtPeerInfoBuilder* builder, const BdtPeerInfo* proto)
{
    builder->result->peerid = proto->peerid;
    builder->result->constInfo = proto->constInfo;
    builder->result->flags = proto->flags;
    memcpy(builder->result->signature, proto->signature, sizeof(proto->signature));

    builder->endpoints.list = BFX_MALLOC_ARRAY(BdtEndpoint, proto->endpointLength);
    memcpy(builder->endpoints.list,
        proto->endpoints,
        sizeof(BdtEndpoint) * proto->endpointLength);
    builder->endpoints.size = builder->endpoints.allocSize = proto->endpointLength;

    if ((builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET) &&
        proto->secret != NULL)
    {
        builder->result->secret = BFX_MALLOC_OBJ(BdtPeerSecret);
        *builder->result->secret = *proto->secret;
    }

    if ((builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST) &&
        proto->snList != NULL)
    {
        builder->snList.list = BFX_MALLOC_ARRAY(BdtPeerid, proto->snListLength);
        memcpy(builder->snList.list,
            proto->snList,
            sizeof(BdtPeerid) * proto->snListLength);
        builder->snList.size = builder->snList.allocSize = proto->snListLength;
    }

    if ((builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO) &&
        proto->snInfoList != NULL)
    {
        builder->snInfoList.list = BFX_MALLOC_ARRAY(BdtPeerInfo*, proto->snInfoListLength);
        memcpy(builder->snInfoList.list,
            proto->snInfoList,
            sizeof(BdtPeerInfo*) * proto->snInfoListLength);
        builder->snInfoList.size = builder->snInfoList.allocSize = proto->snInfoListLength;
    }
    builder->result->createTime = proto->createTime;
}

BDT_API(void) BdtPeerInfoSetConstInfo(BdtPeerInfoBuilder* builder, const BdtPeerConstInfo* constInfo)
{
    memcpy(&builder->result->constInfo, constInfo, sizeof(BdtPeerConstInfo));
    BdtGetPeeridFromConstInfo(&builder->result->constInfo, &builder->result->peerid);
}

BDT_API(void) BdtPeerInfoAddEndpoint(BdtPeerInfoBuilder* builder, const BdtEndpoint* endpoint)
{
    if (endpoint == NULL)
    {
        return;
    }

    for (size_t i = 0; i < builder->endpoints.size; i++)
    {
        // 去掉signed/wan标记去重
        BdtEndpoint* existEndpoint = builder->endpoints.list + i;
        uint8_t existFlag = existEndpoint->flag & (~(BDT_ENDPOINT_FLAG_STATIC_WAN | BDT_ENDPOINT_FLAG_SIGNED));
        uint8_t newFlag = endpoint->flag & (~(BDT_ENDPOINT_FLAG_STATIC_WAN | BDT_ENDPOINT_FLAG_SIGNED));

        if (existFlag == newFlag &&
            (existEndpoint->port == endpoint->port) &&
            (((existFlag & BDT_ENDPOINT_IP_VERSION_4) && memcmp(existEndpoint->address, endpoint->address, sizeof(endpoint->address)) == 0) ||
            (memcmp(existEndpoint->addressV6, endpoint->addressV6, sizeof(endpoint->addressV6)) == 0)))
        {
            // 标记改变要重新签名
            if (existEndpoint->flag != endpoint->flag)
            {
                existEndpoint->flag |= newFlag;
                existEndpoint->flag &= ~(BDT_ENDPOINT_FLAG_STATIC_WAN);
                existEndpoint->flag |= (endpoint->flag & BDT_ENDPOINT_FLAG_STATIC_WAN);
                builder->result->flags |= BDT_PEER_INFO_FLAG_SIGN_UPDATE;
                builder->result->createTime = 0;
            }
            return;
        }
    }
    bfx_vector_endpoint_vector_push_back(&builder->endpoints, *endpoint);
    builder->result->flags |= BDT_PEER_INFO_FLAG_SIGN_UPDATE;
    builder->result->createTime = 0;
}

BDT_API(void) BdtPeerInfoClearEndpoint(BdtPeerInfoBuilder* builder)
{
    if (builder->endpoints.size > 0)
    {
        builder->result->flags &= ~BDT_PEER_INFO_FLAG_SIGNED;
        builder->result->createTime = 0;
    }
    bfx_vector_endpoint_vector_cleanup(&builder->endpoints);
}

BDT_API(void) BdtPeerInfoAddSn(BdtPeerInfoBuilder* builder, const BdtPeerid* peerid)
{
    if (!(builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST))
    {
        return;
    }
    if (peerid == NULL)
    {
        return;
    }

    for (size_t i = 0; i < builder->snList.size; i++)
    {
        if (memcmp(&builder->snList.list[i], peerid, sizeof(BdtEndpoint)) == 0)
        {
            return;
        }
    }
    bfx_vector_peerid_vector_push_back(&builder->snList, *peerid);
    builder->result->flags &= ~BDT_PEER_INFO_FLAG_SIGNED;
    builder->result->createTime = 0;
}

BDT_API(void) BdtPeerInfoAddSnInfo(BdtPeerInfoBuilder* builder, const BdtPeerInfo* peerInfo)
{
    if (!(builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO))
    {
        return;
    }
    if (peerInfo == NULL)
    {
        return;
    }
    for (size_t i = 0; i < builder->snList.size; i++)
    {
        if (memcmp(&builder->snList.list[i], &peerInfo->peerid, sizeof(BdtEndpoint)) == 0)
        {
            return;
        }
    }
    bfx_vector_peerid_vector_push_back(&builder->snList, peerInfo->peerid);
    bfx_vector_peerinfo_vector_push_back(&builder->snInfoList, (BdtPeerInfo*)peerInfo);
    BdtAddRefPeerInfo(peerInfo);
    builder->result->flags &= ~BDT_PEER_INFO_FLAG_SIGNED;
    builder->result->createTime = 0;
}

BDT_API(void) BdtPeerInfoSetSecret(BdtPeerInfoBuilder* builder, const BdtPeerSecret* secret)
{
    assert(secret != NULL);
    if ((builder->flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET))
    {
        if (builder->result->secret == NULL ||
            secret->secretLength != builder->result->secret->secretLength ||
            memcmp(secret, &builder->result->secret->secret, secret->secretLength) != 0)
        {
            if (builder->result->secret == NULL)
            {
                builder->result->secret = BFX_MALLOC_OBJ(BdtPeerSecret);
            }
            builder->result->secret->secretLength = secret->secretLength;
            memcpy(&builder->result->secret->secret, secret, secret->secretLength);
            builder->result->flags &= ~BDT_PEER_INFO_FLAG_SIGNED;
            builder->result->createTime = 0;
        }
    }
}

BDT_API(const BdtPeerInfo*) BdtPeerInfoBuilderFinish(BdtPeerInfoBuilder* builder, const BdtPeerSecret* signSecret)
{
    if (signSecret)
    {
        BdtPeerInfoSetSecret(builder, signSecret);
    }

    BdtPeerInfo* peerInfo = builder->result;
    peerInfo->endpoints = builder->endpoints.list;
    peerInfo->endpointLength = builder->endpoints.size;
    peerInfo->snList = builder->snList.list;
    peerInfo->snListLength = builder->snList.size;
    peerInfo->snInfoList = builder->snInfoList.list;
    peerInfo->snInfoListLength = builder->snInfoList.size;
    if (builder->createTime != 0)
    {
        peerInfo->createTime = builder->createTime;
    }
    else if (peerInfo->createTime == 0)
    {
        peerInfo->createTime = BfxTimeGetNow(false);
        peerInfo->flags &= ~BDT_PEER_INFO_FLAG_SIGNED;
    }

    BFX_FREE(builder);

    uint32_t errorCode = BFX_RESULT_SUCCESS;
    if (signSecret &&
        (((peerInfo->flags & BDT_PEER_INFO_FLAG_SIGNED) == 0) ||
        ((peerInfo->flags & BDT_PEER_INFO_FLAG_SIGN_UPDATE) != 0))
        )
    {
        do
        {
            // 重新签名
            BfxBufferStream bufferStream;
            uint8_t signBuffer[2048];
            BfxBufferStreamInit(&bufferStream, signBuffer, 2048);
            size_t pos = 0;
            size_t writeBytes = 0;
            size_t signStartPos;
            errorCode = BdtPeerInfoWriteSignFieldToBuffer(peerInfo, TRUE, &bufferStream, &writeBytes, &signStartPos);
            if (errorCode != BFX_RESULT_SUCCESS)
            {
                break;
            }
            errorCode = BdtPeerInfoSign(signSecret, peerInfo, bufferStream.buffer + signStartPos, writeBytes - (signStartPos - pos));
            if (errorCode != BFX_RESULT_SUCCESS)
            {
                break;
            }

            for (size_t i = 0; i < peerInfo->endpointLength; i++)
            {
                peerInfo->endpoints[i].flag |= BDT_ENDPOINT_FLAG_SIGNED;
            }
            peerInfo->flags |= BDT_PEER_INFO_FLAG_SIGNED;
            peerInfo->flags &= ~BDT_PEER_INFO_FLAG_SIGN_UPDATE;
        } while (FALSE);
    }

    if (errorCode != BFX_RESULT_SUCCESS)
    {
        BdtReleasePeerInfo(peerInfo);
        return NULL;
    }
    return peerInfo;
}

BDT_API(int32_t) BdtAddRefPeerInfo(const BdtPeerInfo* peerInfo)
{
    BdtPeerInfo* p = (BdtPeerInfo*)peerInfo;
    return BfxAtomicInc32(&p->refCount);
}

BDT_API(int32_t) BdtReleasePeerInfo(const BdtPeerInfo* peerInfo)
{
    BdtPeerInfo* p = (BdtPeerInfo*)peerInfo;
    int32_t refCount = BfxAtomicDec32(&p->refCount);
    if (refCount <= 0)
    {
        if (peerInfo->endpoints)
        {
            BFX_FREE(p->endpoints);
        }
        if (peerInfo->snList)
        {
            BFX_FREE(p->snList);
        }
        if (peerInfo->snInfoListLength)
        {
            for (size_t ix = 0; ix < peerInfo->snInfoListLength; ++ix)
            {
                BdtReleasePeerInfo(p->snInfoList[ix]);
            }
            BFX_FREE((void*)p->snInfoList);
        }
        if (peerInfo->secret != NULL)
        {
            BFX_FREE(peerInfo->secret);
        }
        BFX_FREE(p);
        return 0;
    }
    return peerInfo->refCount;
}

BDT_API(const BdtPeerConstInfo*) BdtPeerInfoGetConstInfo(const BdtPeerInfo* peerInfo)
{
    return &(peerInfo->constInfo);
}

BDT_API(const BdtPeerid*) BdtPeerInfoGetPeerid(const BdtPeerInfo* peerInfo)
{
    return &(peerInfo->peerid);
}

BDT_API(const BdtEndpoint*) BdtPeerInfoListEndpoint(const BdtPeerInfo* peerInfo, size_t* outCount)
{
    *outCount = peerInfo->endpointLength;
    return peerInfo->endpoints;
}

BDT_API(const BdtPeerid*) BdtPeerInfoListSn(const BdtPeerInfo* peerInfo, size_t* outCount)
{
    *outCount = peerInfo->snListLength;
    return peerInfo->snList;
}

BDT_API(const BdtPeerInfo**) BdtPeerInfoListSnInfo(const BdtPeerInfo* peerInfo, size_t* outCount)
{
    *outCount = peerInfo->snInfoListLength;
    return peerInfo->snInfoList;
}

BDT_API(const BdtPeerSecret*) BdtPeerInfoGetSecret(const BdtPeerInfo* peerInfo)
{
    return peerInfo->secret;
}

bool BdtPeerInfoIsSigned(const BdtPeerInfo* peerInfo)
{
    return (peerInfo->flags & BDT_PEER_INFO_FLAG_SIGNED) != 0;
}

BDT_API(uint32_t) BdtGetPublicKeyLength(uint8_t keyType)
{
    return s_bdtKeyLengthMap[keyType][0];
}

BDT_API(uint32_t) BdtGetSecretKeyMaxLength(uint8_t keyType)
{
    return s_bdtKeyLengthMap[keyType][1];
}

BDT_API(uint32_t) BdtGetKeyBitLength(uint8_t keyType)
{
    return s_bdtKeyLengthMap[keyType][2];
}

static uint32_t BdtPeerInfoWriteEndpoint(const BdtEndpoint* endpoint, uint32_t flag, BfxBufferStream* stream, size_t* writeBytes)
{
    size_t pos = BfxBufferStreamGetPos(stream);

    BfxBufferWriteUInt8(stream, flag, writeBytes);
    BfxBufferWriteUInt16(stream, endpoint->port, writeBytes);
    if ((endpoint->flag & BDT_ENDPOINT_IP_VERSION_4) != 0)
    {
        int ret = BfxBufferWriteByteArray(stream, endpoint->address, sizeof(endpoint->address));
        if (ret != BFX_RESULT_SUCCESS)
        {
            return (uint32_t)ret;
        }
    }
    else if ((endpoint->flag & BDT_ENDPOINT_IP_VERSION_6) != 0)
    {
        int ret = BfxBufferWriteByteArray(stream, (uint8_t*)endpoint->addressV6, sizeof(endpoint->addressV6));
        if (ret != BFX_RESULT_SUCCESS)
        {
            return (uint32_t)ret;
        }
    }

    *writeBytes = BfxBufferStreamGetPos(stream) - pos;
    return BFX_RESULT_SUCCESS;
}

static uint32_t BdtPeerInfoReadEndpoint(BdtEndpoint* endpoint, BfxBufferStream* stream, size_t* readBytes)
{
    size_t startPos = BfxBufferStreamGetPos(stream);
    int readSize = BfxBufferReadUInt8(stream, &endpoint->flag);
    if (readSize <= 0)
    {
        return BFX_RESULT_PARSE_FAILED;
    }

    readSize = BfxBufferReadUInt16(stream, &endpoint->port);
    if (readSize <= 0)
    {
        return BFX_RESULT_PARSE_FAILED;
    }

    if ((endpoint->flag & BDT_ENDPOINT_IP_VERSION_4) != 0)
    {
        readSize = BfxBufferReadByteArray(stream, endpoint->address, sizeof(endpoint->address));
        if (readSize <= 0)
        {
            return BFX_RESULT_PARSE_FAILED;
        }
    }
    else if ((endpoint->flag & BDT_ENDPOINT_IP_VERSION_6) != 0)
    {
        readSize = BfxBufferReadByteArray(stream, (uint8_t*)endpoint->addressV6, sizeof(endpoint->addressV6));
        if (readSize <= 0)
        {
            return BFX_RESULT_PARSE_FAILED;
        }
    }
    *readBytes = BfxBufferStreamGetPos(stream) - startPos;
    return BFX_RESULT_SUCCESS;
}

// 写入不包含指定标记的endpoint
static uint32_t BdtPeerInfoWriteEndpointsWithoutFlag(const BdtEndpoint* endpoints,
    uint8_t endpointCount,
    uint32_t flag,
    BfxBufferStream* stream,
    size_t* writeBytes)
{
    size_t pos = BfxBufferStreamGetPos(stream);
    uint32_t r = BFX_RESULT_SUCCESS;
    for (uint8_t i = 0; i < endpointCount; i++)
    {
        const BdtEndpoint* ep = endpoints + i;
        if ((ep->flag & flag) == 0)
        {
            r = BdtPeerInfoWriteEndpoint(ep, ep->flag, stream, writeBytes);
            if (r != BFX_RESULT_SUCCESS)
            {
                return r;
            }
        }
    }
    *writeBytes = BfxBufferStreamGetPos(stream) - pos;
    return BFX_RESULT_SUCCESS;
}

// 写入要签名的字段<endpoint/sn/sninfo>
static uint32_t BdtPeerInfoWriteSignFieldToBuffer(const BdtPeerInfo* peerInfo,
    BOOL isWillResign,
    BfxBufferStream* stream,
    size_t* writeBytes,
    size_t* signStartPos)
{
    uint32_t ret = BFX_RESULT_SUCCESS;
    size_t startPos = BfxBufferStreamGetPos(stream);
    uint8_t writeEndpointCount = (uint8_t)peerInfo->endpointLength;
    BfxBufferWriteUInt8(stream, writeEndpointCount, writeBytes);
    if (!isWillResign)
    {
        // 不重签名，就可能有部分endpoint不被签名，要先写入
        ret = BdtPeerInfoWriteEndpointsWithoutFlag(peerInfo->endpoints, writeEndpointCount, BDT_ENDPOINT_FLAG_SIGNED, stream, writeBytes);
        if (ret != BFX_RESULT_SUCCESS)
        {
            return ret;
        }
    }

    *signStartPos = BfxBufferStreamGetPos(stream);
    // 再写入签名的endpointlist
    for (uint8_t epI = 0; epI < writeEndpointCount; epI++)
    {
        const BdtEndpoint* ep = peerInfo->endpoints + epI;
        if (isWillResign || (ep->flag & BDT_ENDPOINT_FLAG_SIGNED))
        {
            ret = BdtPeerInfoWriteEndpoint(ep, (ep->flag | BDT_ENDPOINT_FLAG_SIGNED), stream, writeBytes);
            if (ret != BFX_RESULT_SUCCESS)
            {
                return ret;
            }
        }
    }

    if (peerInfo->snInfoList != NULL)
    {
        uint8_t writeSnInfoCount = (uint8_t)peerInfo->snInfoListLength;
        ret = (uint32_t)BfxBufferWriteUInt8(stream, writeSnInfoCount, writeBytes);
        if (ret != BFX_RESULT_SUCCESS)
        {
            return ret;
        }
        for (size_t ix = 0; ix < writeSnInfoCount; ++ix)
        {
            ret = BufferWritePeerInfo(stream, peerInfo->snInfoList[ix], writeBytes);
            if (ret != BFX_RESULT_SUCCESS)
            {
                return ret;
            }
        }
    }
    else if (peerInfo->snList)
    {
        uint8_t writeSnListCount = (uint8_t)peerInfo->snListLength;
        ret = (uint32_t)BfxBufferWriteUInt8(stream, writeSnListCount, writeBytes);
        if (ret != BFX_RESULT_SUCCESS)
        {
            return ret;
        }
        for (size_t ix = 0; ix < writeSnListCount; ++ix)
        {
            ret = (uint32_t)Bdt_BufferWritePeerid(stream, peerInfo->snList + ix, writeBytes);
            if (ret != BFX_RESULT_SUCCESS)
            {
                return ret;
            }
        }
    }
    BfxBufferWriteUInt64(stream, peerInfo->createTime, writeBytes);
    *writeBytes = BfxBufferStreamGetPos(stream) - startPos;
    return BFX_RESULT_SUCCESS;
}

static uint32_t BdtPeerInfoReadSignFieldFromBuffer(BdtPeerInfo* peerInfo, uint32_t flags, BfxBufferStream* stream, size_t* readBytes, size_t* signStartPos)
{
    uint32_t ret = BFX_RESULT_SUCCESS;
    int readSize = 0;
    uint8_t epCount = 0;
    size_t startPos = BfxBufferStreamGetPos(stream);
    *signStartPos = 0;
    readSize = BfxBufferReadUInt8(stream, &epCount);
    if (readSize <= 0)
    {
        return BFX_RESULT_PARSE_FAILED;
    }
    peerInfo->endpointLength = (size_t)epCount;
    if (epCount > 0)
    {
        peerInfo->endpoints = BFX_MALLOC_ARRAY(BdtEndpoint, peerInfo->endpointLength);
        for (uint8_t epI = 0; epI < epCount; epI++)
        {
            BdtEndpoint* endpoint = peerInfo->endpoints + epI;
            size_t endpointReadSize = 0;
            ret = BdtPeerInfoReadEndpoint(endpoint, stream, &endpointReadSize);
            if (ret != BFX_RESULT_SUCCESS)
            {
                return BFX_RESULT_PARSE_FAILED;
            }
            if (*signStartPos == 0 && (endpoint->flag & BDT_ENDPOINT_FLAG_SIGNED))
            {
                *signStartPos = BfxBufferStreamGetPos(stream) - endpointReadSize;
            }
        }
    }

    if (*signStartPos == 0)
    {
        *signStartPos = BfxBufferStreamGetPos(stream);
    }

    if (flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO)
    {
        uint8_t snCount = 0;
        readSize = BfxBufferReadUInt8(stream, &snCount);
        if (readSize <= 0)
        {
            return BFX_RESULT_PARSE_FAILED;
        }
        if (snCount > 0)
        {
            peerInfo->snInfoListLength = snCount;
            peerInfo->snInfoList = BFX_MALLOC_ARRAY(BdtPeerInfo*, snCount);
            for (uint8_t ix = 0; ix < snCount; ++ix)
            {
                const BdtPeerInfo* snInfo = NULL;
                BufferReadPeerInfo(stream, &snInfo);
                if (!snInfo)
                {
                    return BFX_RESULT_PARSE_FAILED;
                }
                peerInfo->snInfoList[ix] = snInfo;
            }
        }
    }
    else if (flags & BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST)
    {
        uint8_t snCount = 0;
        readSize = BfxBufferReadUInt8(stream, &snCount);
        if (readSize <= 0)
        {
            return BFX_RESULT_PARSE_FAILED;
        }
        if (snCount > 0)
        {
            peerInfo->snListLength = snCount;
            peerInfo->snList = BFX_MALLOC_ARRAY(BdtPeerid, snCount);
            for (uint8_t ix = 0; ix < snCount; ++ix)
            {
                readSize = Bdt_BufferReadPeerid(stream, peerInfo->snList + ix);
                if (readSize <= 0)
                {
                    return BFX_RESULT_PARSE_FAILED;
                }
            }
        }
    }

    readSize = BfxBufferReadUInt64(stream, &peerInfo->createTime);
    if (readSize <= 0)
    {
        return BFX_RESULT_PARSE_FAILED;
    }
    *readBytes = BfxBufferStreamGetPos(stream) - startPos;

    return BFX_RESULT_SUCCESS;
}

static uint32_t BdtPeerInfoSign(const BdtPeerSecret* signSecret, BdtPeerInfo* peerInfo, const uint8_t* buffer, size_t bufferBytes)
{
    if (peerInfo->flags & BDT_PEER_INFO_FLAG_SIGNED)
    {
        return BFX_RESULT_SUCCESS;
    }

    size_t signBytes = sizeof(peerInfo->signature);
    int signResult = Bdt_RsaSignMd5(buffer,
        bufferBytes,
        (uint8_t*)&signSecret->secret,
        signSecret->secretLength,
        peerInfo->signature,
        &signBytes);

    if (signResult != 0)
    {
        return BFX_RESULT_INVALID_SIGNATURE;
    }
    return BFX_RESULT_SUCCESS;
}

static uint32_t BdtPeerInfoVerify(const BdtPeerInfo* peerInfo, const uint8_t* buffer, size_t bufferBytes)
{
    int verifyResult = Bdt_RsaVerifyMd5(peerInfo->signature,
        sizeof(peerInfo->signature),
        (uint8_t*)&peerInfo->constInfo.publicKey,
        BdtGetPublicKeyLength(peerInfo->constInfo.publicKeyType),
        buffer,
        bufferBytes);
    if (verifyResult != 0)
    {
        return BFX_RESULT_INVALID_SIGNATURE;
    }
    return BFX_RESULT_SUCCESS;
}

BDT_API(bool) BdtPeerInfoIsEqual(const BdtPeerInfo* lhs, const BdtPeerInfo* rhs)
{
    //TODO:
    if (lhs == rhs)
    {
        return true;
    }
    return false;
}

BDT_API(uint64_t) BdtPeerInfoGetCreateTime(const BdtPeerInfo* peerInfo)
{
    return peerInfo->createTime;
}

BDT_API(BdtPeerInfo*) BdtPeerInfoCreateForTest()
{
    BdtPeerInfo* peerinfo = BFX_MALLOC_OBJ(BdtPeerInfo);
    peerinfo->refCount = 1;
    return peerinfo;
}

BDT_API(uint32_t) BdtPeerInfoEncodeForTest(const BdtPeerInfo* peerInfo, BfxBufferStream* stream)
{
    size_t nWrite = 0;
    BfxBufferWriteUInt32(stream, peerInfo->flags, &nWrite);

    BfxBufferWriteByteArray(stream, (uint8_t*)(&peerInfo->peerid), sizeof(BdtPeerid));

    BfxBufferWriteByteArray(stream, (uint8_t*)(&peerInfo->constInfo), sizeof(BdtPeerConstInfo));

    BfxBufferWriteUInt8(stream, (uint8_t)(peerInfo->endpointLength), &nWrite);
    BfxBufferWriteByteArray(stream, (uint8_t*)(peerInfo->endpoints), peerInfo->endpointLength * sizeof(BdtEndpoint));

    BfxBufferWriteUInt32(stream, (uint32_t)peerInfo->snListLength, &nWrite);
    for (int i = 0; i < peerInfo->snListLength; i++)
    {
        BfxBufferWriteByteArray(stream, (uint8_t*)(peerInfo->snList + i), sizeof(BdtPeerid));
    }

    //忽略sninfo

    BfxBufferWriteByteArray(stream, (uint8_t*)(peerInfo->secret), sizeof(BdtPeerSecret));

    BfxBufferWriteByteArray(stream, (uint8_t*)(peerInfo->signature), BDT_PEER_LENGTH_SIGNATRUE);

    BfxBufferWriteUInt64(stream, peerInfo->createTime, &nWrite);

    return BFX_RESULT_SUCCESS;
}


BDT_API(uint32_t) BdtPeerInfoDecodeForTest(BfxBufferStream* stream, BdtPeerInfo* peerInfo)
{
    BfxBufferReadUInt32(stream, &peerInfo->flags);

    BfxBufferReadByteArray(stream, (uint8_t*)(&peerInfo->peerid), sizeof(BdtPeerid));

    BfxBufferReadByteArray(stream, (uint8_t*)(&peerInfo->constInfo), sizeof(BdtPeerConstInfo));

    uint8_t len = 0;
    BfxBufferReadUInt8(stream, &len);
    peerInfo->endpointLength = len;
    peerInfo->endpoints = BFX_MALLOC_ARRAY(BdtEndpoint, peerInfo->endpointLength);
    BfxBufferReadByteArray(stream, (uint8_t*)(peerInfo->endpoints), peerInfo->endpointLength * sizeof(BdtEndpoint));

    uint32_t count = 0;
    BfxBufferReadUInt32(stream, &count);
    peerInfo->snListLength = count;
    if (peerInfo->snListLength)
    {
        peerInfo->snList = BFX_MALLOC_ARRAY(BdtPeerid, peerInfo->snListLength);
        for (int i = 0; i < peerInfo->snListLength; i++)
        {
            BfxBufferReadByteArray(stream, (uint8_t*)(peerInfo->snList + i), sizeof(BdtPeerid));
        }
    }

    //忽略sninfo
    peerInfo->secret = BFX_MALLOC_OBJ(BdtPeerSecret);
    BfxBufferReadByteArray(stream, (uint8_t*)(peerInfo->secret), sizeof(BdtPeerSecret));

    BfxBufferReadByteArray(stream, peerInfo->signature, BDT_PEER_LENGTH_SIGNATRUE);

    BfxBufferReadUInt64(stream, &peerInfo->createTime);

    return BFX_RESULT_SUCCESS;
}

void BdtPeerInfoSetCreateTime(BdtPeerInfoBuilder* peerInfoBuilder, uint64_t createTime)
{
    peerInfoBuilder->createTime = createTime;
}

void BdtPeerInfoSetSignature(BdtPeerInfoBuilder* peerInfoBuilder, const uint8_t* signature, size_t signSize)
{
    if (signSize > sizeof(peerInfoBuilder->result->signature))
    {
        assert(false);
        return;
    }
    memcpy(peerInfoBuilder->result->signature, signature, signSize);
    peerInfoBuilder->result->flags |= BDT_PEER_INFO_FLAG_SIGNED;
}

BDT_API(const uint8_t*) BdtPeerInfoGetSignature(const BdtPeerInfo* peerInfo, size_t* outSize)
{
    if (peerInfo->flags & BDT_PEER_INFO_FLAG_SIGNED)
    {
        *outSize = sizeof(peerInfo->signature);
        return peerInfo->signature;
    }
    return NULL;
}

const BdtEndpoint* BdtPeerInfoFindEndpoint(const BdtPeerInfo* peerInfo, const BdtEndpoint* endpoint)
{
    for (size_t i = 0; i < peerInfo->endpointLength; i++)
    {
        const BdtEndpoint* srcEndpoint = peerInfo->endpoints + i;
        if (!BdtEndpointCompare(srcEndpoint, endpoint, false))
        {
            return srcEndpoint;
        }
    }
    return NULL;
}

static size_t PeeridFormat(const BdtPeerid* peerid, char* outBuffer, size_t bufferSize)
{
    if (bufferSize > 65)
    {
        BdtPeeridToString(peerid, outBuffer);
        return 64;
    }
    else
    {
        char pidStr[65];
        BdtPeeridToString(peerid, pidStr);
        strncpy(outBuffer, pidStr, bufferSize - 1);
        return bufferSize - 1;
    }
}

const char* BdtPeerInfoFormat(const BdtPeerInfo* peerInfo, char* outBuffer, size_t* inoutBufferSize)
{
    size_t offset = 0;
    size_t bufferSize = *inoutBufferSize;
    if (!peerInfo)
    {
        if (outBuffer != NULL)
        {
            outBuffer[0] = '\0';
        }
        *inoutBufferSize = 0;
        return outBuffer;
    }

    int len = snprintf(outBuffer + offset,
        bufferSize - offset,
        "peerid:");
    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }
    offset += len;

    offset += PeeridFormat(&peerInfo->peerid, outBuffer + offset, bufferSize - offset);
    if (offset >= bufferSize)
    {
        goto FORMAT_FINISH;
    }

    const BdtPeerConstInfo* constInfo = &peerInfo->constInfo;
    const BdtPeerArea* area = &constInfo->areaCode;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rarea:(%u.%u.%u.%u.%u)",
        (uint32_t)area->continent,
        (uint32_t)area->country,
        (uint32_t)area->carrier,
        (uint32_t)area->city,
        (uint32_t)area->inner
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    const uint32_t* devid32 = (const uint32_t*)constInfo->deviceId;
    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rdevid:%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
        devid32[0], devid32[1], devid32[2], devid32[3],
        devid32[4], devid32[5], devid32[6], devid32[7],
        devid32[8], devid32[9], devid32[10], devid32[11],
        devid32[12], devid32[13], devid32[14], devid32[15]);

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rpkType:%d",
        (uint32_t)constInfo->publicKeyType
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rendpoints:["
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    const char* epFormat[] = { "%s", ",%s" };

    for (size_t epi = 0; epi < peerInfo->endpointLength; epi++)
    {
        char eps[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
        BdtEndpointToString(peerInfo->endpoints + epi, eps);
        len = snprintf(
            outBuffer + offset,
            bufferSize - offset,
            epFormat[epi == 0 ? 0 : 1],
            eps
        );

        if (len <= 0)
        {
            goto FORMAT_FINISH;
        }

        offset += len;
    }

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "]"
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rsn:["
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    for (size_t sni = 0; sni < peerInfo->snListLength; sni++)
    {
        if (sni > 0)
        {
            len = snprintf(outBuffer + offset, bufferSize - offset, ",");
            if (len <= 0)
            {
                goto FORMAT_FINISH;
            }
            offset += len;
        }

        offset += PeeridFormat(peerInfo->snList + sni, outBuffer + offset, bufferSize - offset);
        if (offset >= bufferSize)
        {
            goto FORMAT_FINISH;
        }
    }

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "]"
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\rsn-info:["
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    for (size_t sninfoi = 0; sninfoi < peerInfo->snInfoListLength; sninfoi++)
    {
        if (sninfoi > 0)
        {
            len = snprintf(outBuffer + offset, bufferSize - offset, ",");
            if (len <= 0)
            {
                goto FORMAT_FINISH;
            }
            offset += len;
        }

        size_t len = bufferSize - offset;
        BdtPeerInfoFormat(peerInfo->snInfoList[sninfoi], outBuffer + offset, &len);
        offset += len;
        if (offset >= bufferSize)
        {
            goto FORMAT_FINISH;
        }
    }

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "]"
    );

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\r%u",
        peerInfo->flags);

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

    len = snprintf(
        outBuffer + offset,
        bufferSize - offset,
        "\n\r%llu",
        peerInfo->createTime);

    if (len <= 0)
    {
        goto FORMAT_FINISH;
    }

    offset += len;

FORMAT_FINISH:
    outBuffer[offset] = '\0';
    *inoutBufferSize = offset;
    return outBuffer;
}