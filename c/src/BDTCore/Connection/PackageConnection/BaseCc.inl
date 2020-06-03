#ifndef __BDT_PACKAGE_CONGESTION_CONTROL_BASE_INL__ 
#define __BDT_PACKAGE_CONGESTION_CONTROL_BASE_INL__
#include "./BaseType.inl"

//拥塞算法状态
typedef enum CcState
{
	CC_STATE_OPEN = 0,
	//发送丢包
	CC_STATE_LOST,
	//发生超时
	CC_STATE_RTO,
	//链接关闭
	CC_STATE_CLOSE,
}CcState;

typedef struct BaseCc
{
	//反初始化
	void (*uninit)(struct BaseCc* cc);

	//状态更改
	void (*onChangeState)(struct BaseCc* cc, CcState oldState, CcState newState);

	//在package被传递到tunnel前调用
	void (*onSend)(struct BaseCc* cc, Bdt_SessionDataPackage* package);

	//收到ack时候会被调用
	void (*onAck)(struct BaseCc* cc, const Bdt_SessionDataPackage* package, uint32_t nAcks, uint64_t flight);

	//收到数据包,返回值决定是否立即发送ack
	uint32_t (*onData)(struct BaseCc* cc, const Bdt_SessionDataPackage* package, RecvPackageType type);

	//拥塞控制发送数据的大小
	uint32_t (*trySendPayload)(struct BaseCc* cc, uint32_t flight, uint16_t* outPayloadLength);

	//是否需要发送ack
	uint32_t (*trySendAck)(struct BaseCc* cc);
}BaseCc;

#endif