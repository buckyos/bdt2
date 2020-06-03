#ifndef __BDT_PACKAGE_CONGESTION_BASE_TYPE_INL__
#define __BDT_PACKAGE_CONGESTION_BASE_TYPE_INL__

#include "../../BdtCore.h"
#include "../../BdtSystemFramework.h"
#include "../../Protocol/Package.h"

typedef enum RecvPackageType
{
	RECV_PACKAGE_TYPE_NONE = 0,
	//���յ���packageΪ�ɰ�
	RECV_PACKAGE_TYPE_OLD,
	//���ն�û���㹻�Ŀռ����洢��package
	RECV_PACKAGE_TYPE_OUT_OF_BUFFER,
	//��������
	RECV_PACKAGE_TYPE_IN_ORDER,
	//���յ��İ�������İ�
	RECV_PACKAGE_TYPE_LOST_ORDER,
	//���յ��İ�ֻ���沿��
	RECV_PACKAGE_TYPE_PART,
}RecvPackageType;

#define PACKAGE_CONNECTION_NOW_TIME_2MS() (BfxTimeGetNow(false) / 1000)

#endif