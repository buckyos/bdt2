#include <BuckyBase/BuckyBase.h>

uint32_t SysResQueryEnable()
{
    return BFX_RESULT_SUCCESS;
}

uint32_t SysResUsageQuery(int64_t* memBytes, int64_t* vmemBytes, uint32_t* handleCount)
{
    *memBytes = 0;
    *vmemBytes = 0;
    *handleCount = 0;
    return BFX_RESULT_SUCCESS;
}