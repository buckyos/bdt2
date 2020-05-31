#ifndef __BSTAT_STAT_IMP_INL__
#define __BSTAT_STAT_IMP_INL__
#ifndef __BSTAT_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of Stat module"
#endif //__BSTAT_STAT_IMP_INL__

typedef struct BuckyStat
{
    ReportInterface* reporter;

    volatile uint32_t nextSeq;
    volatile int32_t ref;
} BuckyStat;

BuckyStat* BStatCreate(
    const uint8_t* peerid,
    uint16_t peeridLength,
    const char* productId,
    const char* productVersion,
    const char* userProductId,
    const char* userProductVersion
)
{
    BuckyStat* stat = BFX_MALLOC_OBJ(BuckyStat);
    stat->ref = 1;
    stat->nextSeq = 0;
    stat->reporter = ReportInterfaceCreate(
        peeridLength, peerid,
        productId,
        productVersion,
        userProductId,
        userProductVersion,
        stat
    );

    return stat;
}

uint32_t BStatReport(
    BuckyStat* stat,
    BStatCommandType cmdType,
    const uint8_t* data,
    uint32_t dataLength
)
{
    assert(stat != NULL);
    assert(stat->reporter != NULL);

    uint32_t seq = (uint32_t)BfxAtomicInc32((volatile int32_t*)&stat->nextSeq);
    ReportInterfaceSend(stat->reporter,
        cmdType,
        seq,
        BfxTimeGetNow(false),
        data,
        dataLength
    );
}

int32_t BStatAddRef(
    BuckyStat* stat
)
{
    return BfxAtomicInc32(&stat->ref);
}

int32_t BStatRelease(
    BuckyStat* stat
)
{
    int32_t ref = BfxAtomicDec32(&stat->ref);
    if (ref == 1)
    {
        ReportInterfaceDestroy(stat->reporter);
        stat->reporter = NULL;
    }
    else if (ref == 0)
    {
        BFX_FREE(stat);
    }
}

static void StatOnResponce(
    BuckyStat* stat,
    const ProtocolResponce* resp
)
{
    // none
    // 以后如果有磁盘缓存，可以在这里清理
    return;
}