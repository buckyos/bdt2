#ifndef _BUCKY_STAT_H_
#define _BUCKY_STAT_H_

#include <BuckyBase/BuckyBase.h>

typedef struct BuckyStat BuckyStat;

typedef enum BStatCommandType
{
	// bdt
	BSTAT_COMMAND_TYPE_BDT_LOW_BOUNDLE = 128,

    BSTAT_COMMAND_TYPE_BDT_CONNECT_DELTA,

	BSTAT_COMMAND_TYPE_BDT_UP_BOUNDLE,
} BStatCommandType;

BuckyStat* BStatCreate(
	const uint8_t* peerid,
	uint16_t peeridLength,
    const char* productId,
    const char* productVersion,
	const char* userProductId,
	const char* userProductVersion
);

uint32_t BStatReport(
    BuckyStat* stat,
    BStatCommandType cmdType,
    const uint8_t* data,
    uint32_t dataLength
);

int32_t BStatAddRef(
    BuckyStat* stat
);

int32_t BStatRelease(
    BuckyStat* stat
);

#endif // _BUCKY_STAT_H_