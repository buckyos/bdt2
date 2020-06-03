#ifndef __BSTAT_REPORT_INTERFACE_INL__
#define __BSTAT_REPORT_INTERFACE_INL__

#ifndef __BSTAT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of stat module"
#endif //__BSTAT_MODULE_IMPL__

#include <BuckyBase/BuckyBase.h>

typedef struct ReportInterface ReportInterface;

static ReportInterface* ReportInterfaceCreate(
    uint16_t peeridLength,
    const uint8_t* peerid,
    const char* productId,
    const char* productVersion,
    const char* userProductId,
    const char* userProductVersion,
    BuckyStat* stat
);

static void ReportInterfaceDestroy(ReportInterface* reporter);

static uint32_t ReportInterfaceSend(
    ReportInterface* reporter,
	uint32_t cmdType,
	uint32_t seq,
	uint64_t timestamp,
	const uint8_t* body,
	uint32_t bodyLength
    );

#endif // __BSTAT_REPORT_INTERFACE_INL__