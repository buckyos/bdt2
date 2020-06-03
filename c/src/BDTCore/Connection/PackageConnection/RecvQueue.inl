#ifndef __BDT_PACKAGE_CONNECTIOIN_RECV_QUEUE_INL__
#define __BDT_PACKAGE_CONNECTIOIN_RECV_QUEUE_INL__

#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "../../BdtCore.h"
#include "../../BdtSystemFramework.h"
#include "../../Protocol/Package.h"
#include "./BaseType.inl"


typedef struct RecvQueueRecvBlock
{
	BfxListItem base;
	uint64_t startStreamPos;
	uint64_t endStreamPos;
}RecvQueueRecvBlock;

//block的相对位置
typedef enum RecvBlockRelativePos
{
	RECV_BLOCK_RELATIVE_POS_NONE = 0,
	RECV_BLOCK_RELATIVE_POS_PREV,
	RECV_BLOCK_RELATIVE_POS_INTERSECT,
	RECV_BLOCK_RELATIVE_POS_INCLUDE,
	RECV_BLOCK_RELATIVE_POS_NEXT,
}RecvBlockRelativePos;

typedef struct RecvQueueRecvBuffer
{
	BfxListItem base;
	uint8_t* data;
	size_t len; //buffer长度
	uint64_t startStreamPos;
	uint64_t endStreamPos; //buffer上面最大的streampos，中间可能有gap
	BdtConnectionRecvCallback callback;
	BfxUserData userData; 
}RecvQueueRecvBuffer;

typedef struct RecvQueue RecvQueue;

static RecvQueue* RecvQueueCreate(uint64_t streamPos);
static void RecvQueueDestory(RecvQueue* queue);

//新包
static uint32_t RecvQueueWriteData(RecvQueue* queue, uint64_t streamPos, const uint8_t* payload, uint16_t payloadLength, RecvPackageType* recvType);

static uint32_t RecvQueuePopCompleteBuffers(RecvQueue* queue, bool force, BfxList* buffers);

static uint32_t RecvQueuePopAllBuffers(RecvQueue* queue, BfxList* buffers);

//ack、sack
static uint32_t RecvQueueGetAckInfo(RecvQueue* queue, uint64_t* ackStreamPos, BdtStreamRange** sackArray);

//传入接收buffer
static uint32_t RecvQueueAddBuffer(RecvQueue* queue, uint8_t* data, size_t len, BdtConnectionRecvCallback callback, const BfxUserData* userData);


static RecvBlockRelativePos RecvQueueRecvBlockCalcRelaPos(RecvQueueRecvBlock* baseBlock, RecvQueueRecvBlock* other);
static RecvQueueRecvBlock* RecvQueueRecvBlockNew(uint64_t startStreamPos, uint64_t endStreamPos);
static void RecvQueueDeleteBlock(RecvQueue* queue, RecvQueueRecvBlock* block);

//返回值表示是否是存储空间不足
static size_t RecvQueueSavePayload(RecvQueue* queue, uint64_t streamPos, const uint8_t* data, size_t len);

//更新streampos信息
static RecvPackageType RecvQueueUpdateStreamPos(RecvQueue* queue, uint64_t startStreamPos, uint64_t endStreamPos);

//非连续package，更新streampos信息到block，返回是否old
static bool RecvQueueAddToBlocks(RecvQueue* queue, uint64_t startStreamPos, uint64_t endStreamPos);

#endif
