#ifndef __SYS_RES_H_INCLUDE_
#define __SYS_RES_H_INCLUDE_

#include <BuckyBase/BuckyBase.h>

uint32_t SysResQueryEnable();
uint32_t SysResUsageQuery(int64_t* memBytes, int64_t* vmemBytes, uint32_t* handleCount);

#endif // #ifndef __SYS_RES_H_INCLUDE_