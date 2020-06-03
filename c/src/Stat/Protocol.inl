#ifndef _BSTAT_PROTOCOL_INL_
#define _BSTAT_PROTOCOL_INL_

#include <BuckyBase/BuckyBase.h>

// 暂时编解码包依赖包结构是对齐的，不存在填充内存
typedef struct ProtocolHeader
{
    uint32_t protocolVersion;
    uint32_t length; // 整个包长，包括header部分
    uint32_t flags;
    uint32_t magic;
    uint32_t cmdType;
    uint32_t seq;
    uint64_t timestamp;
} ProtocolHeader;

typedef struct ProtocolSource
{
    const uint16_t peeridLength;
    const uint8_t* peerid;
    const char* productId;
    const char* productVersion;
    const char* userProductId;
    const char* userProductVersion;
} ProtocolSource;

typedef struct ProtocolRequest
{
    ProtocolHeader header;
    ProtocolSource source;
    // const uint8_t body[0];
} ProtocolRequest;

typedef struct ProtocolResponce
{
    ProtocolHeader header;
    uint32_t result;
} ProtocolResponce;

#endif // _BSTAT_PROTOCOL_INL_