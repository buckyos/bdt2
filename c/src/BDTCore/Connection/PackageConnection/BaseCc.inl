#ifndef __BDT_PACKAGE_CONGESTION_CONTROL_BASE_INL__ 
#define __BDT_PACKAGE_CONGESTION_CONTROL_BASE_INL__
#include "./BaseType.inl"

//ӵ���㷨״̬
typedef enum CcState
{
	CC_STATE_OPEN = 0,
	//���Ͷ���
	CC_STATE_LOST,
	//������ʱ
	CC_STATE_RTO,
	//���ӹر�
	CC_STATE_CLOSE,
}CcState;

typedef struct BaseCc
{
	//����ʼ��
	void (*uninit)(struct BaseCc* cc);

	//״̬����
	void (*onChangeState)(struct BaseCc* cc, CcState oldState, CcState newState);

	//��package�����ݵ�tunnelǰ����
	void (*onSend)(struct BaseCc* cc, Bdt_SessionDataPackage* package);

	//�յ�ackʱ��ᱻ����
	void (*onAck)(struct BaseCc* cc, const Bdt_SessionDataPackage* package, uint32_t nAcks, uint64_t flight);

	//�յ����ݰ�,����ֵ�����Ƿ���������ack
	uint32_t (*onData)(struct BaseCc* cc, const Bdt_SessionDataPackage* package, RecvPackageType type);

	//ӵ�����Ʒ������ݵĴ�С
	uint32_t (*trySendPayload)(struct BaseCc* cc, uint32_t flight, uint16_t* outPayloadLength);

	//�Ƿ���Ҫ����ack
	uint32_t (*trySendAck)(struct BaseCc* cc);
}BaseCc;

#endif