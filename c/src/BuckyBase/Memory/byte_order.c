#include "../BuckyBase.h"
#include "./byte_order.h"


BFX_API(uint16_t) BfxReverse16(uint16_t value) {
    uint8_t* buf = (uint8_t*)(&value);
    return (uint16_t)(buf[0] << 8 | buf[1]);
}

BFX_API(uint32_t) BfxReverse32(uint32_t value) {
    uint8_t* buf = (uint8_t*)(&value);
    return (uint32_t)(buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);
}

typedef union {
    uint64_t v64;
    uint32_t v32[2];
}_Reverse64Helper;

BFX_API(uint64_t) BfxReverse64(uint64_t value) {
    _Reverse64Helper rs = { value };
    _Reverse64Helper ret = {
        .v32 = {BfxReverse32(rs.v32[1]), BfxReverse32(rs.v32[0])}
    };
    // ret.v32[0] = BfxReverse32(rs.v32[1]);
    // ret.v32[1] = BfxReverse32(rs.v32[0]);

    return ret.v64;
}