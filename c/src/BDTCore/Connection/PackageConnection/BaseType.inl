#ifndef __BDT_PACKAGE_CONGESTION_BASE_TYPE_INL__
#define __BDT_PACKAGE_CONGESTION_BASE_TYPE_INL__

#include "../../BdtCore.h"
#include "../../BdtSystemFramework.h"
#include "../../Protocol/Package.h"

typedef enum RecvPackageType
{
	RECV_PACKAGE_TYPE_NONE = 0,
	//接收到的package为旧包
	RECV_PACKAGE_TYPE_OLD,
	//接收端没有足够的空间来存储改package
	RECV_PACKAGE_TYPE_OUT_OF_BUFFER,
	//正常接收
	RECV_PACKAGE_TYPE_IN_ORDER,
	//接收到的包是乱序的包
	RECV_PACKAGE_TYPE_LOST_ORDER,
	//接收到的包只保存部分
	RECV_PACKAGE_TYPE_PART,
}RecvPackageType;

#define PACKAGE_CONNECTION_NOW_TIME_2MS() (BfxTimeGetNow(false) / 1000)

#endif