#ifndef _BDT_STATISTICS_H_
#define _BDT_STATISTICS_H_

#include "../BdtCore.h"

typedef struct BdtStatistics BdtStatistics;

void BdtStat_OnConnectionCreate();
void BdtStat_OnConnectionEstablish(bool isUdp, bool isPassive);
void BdtStat_OnConnectionFailed(uint32_t errorCode, bool isPassive);
void BdtStat_OnConnectionRecv(size_t size, bool isUdp);

BDT_API(uint32_t) BdtGetStatisticsSnapshot(BdtStack* stack, BdtStatisticsSnapshot* snapshot);

#endif // _BDT_STATISTICS_H_