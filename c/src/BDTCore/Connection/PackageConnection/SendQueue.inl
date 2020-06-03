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
	//��ǰbuffer�����ݱ������λ��
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
	SEND_BLOCK_CHECK_STATE_PREV,	//���ֵ�ǰ״̬������Ƕ�ʧ������ʧ��flightҲ���
	SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE,
}SendBlockCheckState;

typedef struct BdtSendCacheUnit
{
	BfxListItem base;
	size_t len; //���ݳ���
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

//��������
static uint32_t SendQueueAddBuffer(SendQueue* queue,const uint8_t* data,size_t len,BdtConnectionSendCallback callback,const BfxUserData* userData);

//��ӹرղ���
static uint32_t SendQueueAddCloseOperation(SendQueue* queue, uint64_t* endStreamPos);

//�յ�ack
static uint32_t SendQueueConfirmRange(SendQueue* queue, uint64_t ackStreamPos, BdtStreamRange* sackArray, uint32_t* outAcked);

//�������streampos��payload
static uint32_t SendQueueFillPayload(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* payloadLength);

//���Ѿ����͹����������streampos��payload
static uint32_t SendQueueFillPayloadFromOld(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength);

//���������������
static uint32_t SendQueueFillPayloadFromNew(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength);

//֪ͨ������ɵ�buffer
static uint32_t SendQueuePopCompleteBuffers(SendQueue* queue, BfxList* buffers);

//û�з�����ɵ�buffer
static uint32_t SendQueuePopAllBuffers(SendQueue* queue, BfxList* buffers);

//��ʱ���
static uint32_t SendQueueMarkRangeTimeout(SendQueue* queue, bool force, uint64_t rto);

//���ʹ��������ڷ��͵ģ��������Ѿ����϶�Ϊ��ʧ�ģ�
static uint32_t SendQueueGetFilght(SendQueue* queue);

//��ǰδ���͵�����
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
