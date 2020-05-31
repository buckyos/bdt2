#ifndef __BDT_HISTORY_STORAGE_INL__
#define __BDT_HISTORY_STORAGE_INL__
#ifndef __BDT_HISTORY_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of history module"
#endif //__BDT_HISTORY_MODULE_IMPL__

#include <BuckyBase/Coll/vector.h>

#define BDT_MICRO_SECONDS_2_MINUTES(microS)	(uint32_t)((microS) / 60000000)
#define BDT_MICRO_SECONDS_2_SECONDS(microS)	((microS) / 1000000)
#define BDT_MICRO_SECONDS_2_MS(microS)	((microS) / 1000)
#define BDT_MINUTES_NOW						BDT_MICRO_SECONDS_2_MINUTES(BfxTimeGetNow(false))
#define BDT_SECONDS_NOW						BDT_MICRO_SECONDS_2_SECONDS(BfxTimeGetNow(false))
#define BDT_MS_NOW							BDT_MICRO_SECONDS_2_MS(BfxTimeGetNow(false))

#define RBTREE_COLOR_INVALID				(-1)

typedef BdtHistory_PeerConnectVector history_connect;
BFX_VECTOR_DECLARE_FUNCTIONS(history_connect, BdtHistory_PeerConnectInfo*, connections, size, _allocsize)
typedef BdtHistory_PeerSuperNodeInfoVector super_node_info;
BFX_VECTOR_DECLARE_FUNCTIONS(super_node_info, BdtHistory_SuperNodeInfo*, snHistorys, size, _allocsize)

#endif //__BDT_HISTORY_STORAGE_INL__