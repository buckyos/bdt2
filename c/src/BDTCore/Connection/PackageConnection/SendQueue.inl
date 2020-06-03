#ifndef __BDT_PACKAGE_CONNECTIOIN_SEND_QUEUE_INL__
#define __BDT_PACKAGE_CONNECTIOIN_SEND_QUEUE_INL__

#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "../../BdtCore.h"
#include "../../BdtSystemFramework.h"
#include "../../Protocol/Package.h"

typedef struct SendQueueSendBuffer
{
	BfxListItem base;
	const uint8_t* data;
	size_t len;
	uint64_t startStreamPos;
	//当前buffer的数据被打包的位置
	uint64_t offsetStreamPos;
	uint64_t endStreamPos;
	BdtConnectionSendCallback callback;
	BfxUserData userData;
}SendQueueSendBuffer;

typedef struct SendQueueSendBlock
{
	BfxListItem base;
	uint64_t startStreamPos;
	uint64_t endStreamPos;
	uint64_t sendTime;
	uint8_t* data;
}SendQueueSendBlock;


typedef enum SendBlockCheckState
{
	SEND_BLOCK_CHECK_STATE_LOST = 0,
	SEND_BLOCK_CHECK_STATE_ACK,
	SEND_BLOCK_CHECK_STATE_PREV,	//保持当前状态，如果是丢失继续丢失，flight也如此
	SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE,
}SendBlockCheckState;

typedef struct BdtSendCacheUnit
{
	BfxListItem base;
	size_t len; //数据长度
}BdtSendCacheUnit;

typedef struct BdtSendCache
{
	uint8_t* data;
	BfxList freeUnit;
	BfxList useUnit;
	uint16_t unitSize;
}BdtSendCache;

typedef void (*SendQueueCloseCallback)(void* userData);
typedef struct SendQueue SendQueue;

static SendQueue* SendQueueCreate(uint64_t streamPos, const BdtPackageConnectionConfig* config);
static void SendQueueDestory(SendQueue* queue);

//发送数据
static uint32_t SendQueueAddBuffer(SendQueue* queue,const uint8_t* data,size_t len,BdtConnectionSendCallback callback,const BfxUserData* userData);

//添加关闭操作
static uint32_t SendQueueAddCloseOperation(SendQueue* queue, uint64_t* endStreamPos);

//收到ack
static uint32_t SendQueueConfirmRange(SendQueue* queue, uint64_t ackStreamPos, BdtStreamRange* sackArray, uint32_t* outAcked);

//给包填充streampos、payload
static uint32_t SendQueueFillPayload(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* payloadLength);

//从已经发送过的里面填充streampos、payload
static uint32_t SendQueueFillPayloadFromOld(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength);

//从最新数据中填充
static uint32_t SendQueueFillPayloadFromNew(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength);

//通知发送完成的buffer
static uint32_t SendQueuePopCompleteBuffers(SendQueue* queue, BfxList* buffers);

//没有发送完成的buffer
static uint32_t SendQueuePopAllBuffers(SendQueue* queue, BfxList* buffers);

//超时检测
static uint32_t SendQueueMarkRangeTimeout(SendQueue* queue, bool force, uint64_t rto);

//发送窗口中正在发送的（不包括已经被认定为丢失的）
static uint32_t SendQueueGetFilght(SendQueue* queue);

//当前未发送的数量
static uint64_t SendQueueGetUnsendSize(SendQueue* queue);

static uint64_t SendQueueGetLostSize(SendQueue* queue);

static void BdtSendCacheInit(BdtSendCache* cache, size_t maxsize, uint16_t mtu);
static void BdtSendCacheUninit(BdtSendCache* cache);
static BdtSendCacheUnit* BdtSendCacheNewUnit(BdtSendCache* cache);
static void BdtSendCacheDeleteUnit(BdtSendCache* cache, BdtSendCacheUnit* unit);
static BdtSendCacheUnit* BdtSendCacheUnitFromData(BdtSendCache* cache, const uint8_t* data);
static uint8_t* BdtSendCacheUnitToData(BdtSendCache* cache, BdtSendCacheUnit* unit);
static SendBlockCheckState CheckBlockStateByAck(SendQueueSendBlock* block, uint64_t ackStreamPos, BdtStreamRange* sackArray);

static SendQueueSendBlock* SendQueueGenerateBlock(SendQueue* queue, uint16_t len);
static void SendQueueMoveLostToFlight(SendQueue* queue, SendQueueSendBlock* lostBlock);

#endif
