#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./Statistics.h"
#include <BuckyBase/BuckyBase.h>

#define SPEED_STAT_INTERVAL 5
#define PEAK_SPEED_SIMPLE_COUNT BFX_ARRAYSIZE(((PeakSpeed*)0) -> speed)

typedef struct BdtSpeed Speed;

typedef struct Flowrate
{
    size_t total;
    size_t udp;
    size_t tcp;
} Flowrate;

typedef struct PeakSpeed
{
    uint32_t speed[4];
} PeakSpeed;

typedef struct BdtStatistics
{
    BdtStatisticsSnapshot stat;

    PeakSpeed peakSpeedTotal;
    PeakSpeed peakSpeedUdp;
    PeakSpeed peakSpeedTcp;

    Flowrate flowrate;
    int64_t flowrateUpdateTime;
    Flowrate flowrateSnapshot;
    int64_t snapshotTime;
} BdtStatistics;

BdtStatistics g_stat = {
    .stat = {
        .size = sizeof(BdtStatisticsSnapshot),
        //.connectionCount = {
        //    .total = 0,
        //    .fail = 0,
        //    .succ = 0,
        //    .tcp = 0,
        //    .udp = 0
        //},
        //.speed = {
        //    .total = 0,
        //    .tcp = 0,
        //    .udp = 0,
        //},
    },
    //.peakSpeed = {
    //    .total = {0 ,0, 0, 0},
    //    .udp = {0, 0, 0, 0},
    //    .tcp = {0, 0, 0, 0},
    //},
    //.flowrate = {
    //    .total = 0,
    //    .udp = 0,
    //    .tcp = 0
    //},
    //.flowrateSnapshot = {
    //    .total = 0,
    //    .udp = 0,
    //    .tcp = 0
    //},
    //.snapshotTime = 0
};

// 统计误差不敏感，不加锁，简单累加
void BdtStat_OnConnectionCreate()
{
    g_stat.stat.connectionCount.total++;
}

void BdtStat_OnConnectionEstablish(bool isUdp, bool isPassive)
{
    if (isUdp)
    {
        g_stat.stat.connectionCount.udp++;
    }
    else
    {
        g_stat.stat.connectionCount.tcp++;
    }
    if (isPassive)
    {
        g_stat.stat.connectionCount.succPassive++;
    }
    else
    {
        g_stat.stat.connectionCount.succActive++;
    }
}

void BdtStat_OnConnectionFailed(uint32_t errorCode, bool isPassive)
{
    if (isPassive)
    {
        g_stat.stat.connectionCount.failPassive++;
    }
    else
    {
        g_stat.stat.connectionCount.failActive++;
    }
}

static void AddPeakSpeedAndReAvg(PeakSpeed* peakSpeed, uint32_t speed, uint32_t* avgSpeed)
{
    if (peakSpeed->speed[0] >= speed)
    {
        return;
    }

    *avgSpeed = (*avgSpeed * PEAK_SPEED_SIMPLE_COUNT + speed - peakSpeed->speed[0]) / PEAK_SPEED_SIMPLE_COUNT;

    for (int i = 1; i < PEAK_SPEED_SIMPLE_COUNT; i++)
    {
        if (peakSpeed->speed[i] > speed)
        {
            peakSpeed->speed[i - 1] = speed;
            break;
        }
        else
        {
            peakSpeed->speed[i - 1] = peakSpeed->speed[i];
        }
    }

    if (peakSpeed->speed[PEAK_SPEED_SIMPLE_COUNT - 1] < speed)
    {
        peakSpeed->speed[PEAK_SPEED_SIMPLE_COUNT - 1] = speed;
    }
}

void BdtStat_OnConnectionRecv(size_t size, bool isUdp)
{
    int64_t now = BfxTimeGetNow(false) >> 20;
    uint32_t statDuration = (uint32_t)(now - g_stat.snapshotTime);
    if (statDuration >= SPEED_STAT_INTERVAL ||
        now - g_stat.flowrateUpdateTime >= SPEED_STAT_INTERVAL / 2)
    {
        // 换一段
        AddPeakSpeedAndReAvg(&g_stat.peakSpeedTotal,
            (uint32_t)(g_stat.flowrate.total - g_stat.flowrateSnapshot.total) / statDuration,
            &g_stat.stat.speed.total);
        AddPeakSpeedAndReAvg(&g_stat.peakSpeedTcp,
            (uint32_t)(g_stat.flowrate.tcp - g_stat.flowrateSnapshot.tcp) / statDuration,
            &g_stat.stat.speed.tcp);
        AddPeakSpeedAndReAvg(&g_stat.peakSpeedUdp,
            (uint32_t)(g_stat.flowrate.udp - g_stat.flowrateSnapshot.udp) / statDuration,
            &g_stat.stat.speed.udp);

        g_stat.flowrateSnapshot = g_stat.flowrate;
        g_stat.snapshotTime = now;
    }

    g_stat.flowrateUpdateTime = now;

    g_stat.flowrate.total += size;
    if (isUdp)
    {
        g_stat.flowrate.udp += size;
    }
    else
    {
        g_stat.flowrate.tcp += size;
    }
}

BDT_API(uint32_t) BdtGetStatisticsSnapshot(BdtStack* stack, BdtStatisticsSnapshot* snapshot)
{
    memcpy(snapshot, &g_stat.stat, sizeof(BdtStatisticsSnapshot));
    assert(snapshot->size == sizeof(BdtStatisticsSnapshot));
    if (snapshot->speed.total == 0)
    {
        return BFX_RESULT_PENDING;
    }
    else
    {
        return BFX_RESULT_SUCCESS;
    }
}
