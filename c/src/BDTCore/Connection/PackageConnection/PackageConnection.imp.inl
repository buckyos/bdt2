#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include <assert.h>
#include "../../Global/Module.h"
#include "./PackageConnection.inl"
#include "../Connection.inl"
#include "./LedbatCc.inl"


#define  PACKAGE_CONNECTION_INIT_STREAM_POS 1

#define		PACKAGE_CONNECTION_FLAG_CLOSE_SLIENT (1<<0)
#define		PACKAGE_CONNECTION_FLAG_CLOSE_RECV (1<<1)
#define		PACKAGE_CONNECTION_FLAG_CLOSE_SEND (1<<2)

#define  PACKAGE_CONNECTION_CHECK_RTO_INTERVAL 50 //50ms
#define  PACKAGE_CONNECTION_MIN_RTO	 200000 //200ms
#define  PACKAGE_CONNECTION_INIT_RTO	 1000000 //1000ms

typedef enum PackageConnectionRttEstimateState
{
	PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_NONE = 0,
	PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_PREPARE,
	PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT,
	PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_CANCEL,
	PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_CALC,
}PackageConnectionRttEstimateState;

struct PackageConnection
{
	BdtSystemFramework* framework;
	char* name;
	BdtConnection* owner;
	Bdt_TunnelContainer* tunnelContainer;
	volatile PackageConnectionState state;
	volatile PackageConnectionCloseSubState closeSubState;
	int32_t flags;
	SendQueue* sendQueue;
	RecvQueue* recvQueue;
	BaseCc* cc;

	Bdt_TimerHelper closeTimer;

	uint32_t rto;
	uint64_t lastRecvAckTime;
	Bdt_TimerHelper rtoTimer;

	Bdt_TimerHelper recvTimer;

	volatile int32_t trySendCnt;
	uint64_t remoteCloseStreamPos;
	uint64_t selfCloseStreamPos;
	uint64_t lastLostStateTime;

	//等待通知上层已经接收数据的buffers
	struct
	{
		BfxList buffers;
		//已经通知上层接收buffer的最大的streamPos
		uint64_t streamPos;
		bool onNotifying;
	}recvReadyQueue;
	BfxThreadLock lockRecvReadyQueue;

	struct
	{
		uint32_t srtt; /* smoothed round trip time << 3 */
		uint32_t mdev; /* medium deviation */
		uint32_t mdev_max; /* maximal mdev for the last rtt period */
		uint32_t rttvar; /* smoothed mdev_max */
		uint32_t rtt_seq; /* sequence number to update rttvar */

		uint32_t rtt;
		uint32_t rttval;
		uint64_t sendTime;
		uint64_t lastEstTime;
		uint64_t ackStreamPos; //ackStreamPos指定的包来进行评估，不能使用重复发送的包
		uint64_t prevAckStreamPos;//上次被指定为评估包得ackstreampos
		volatile int32_t state;
	} rttEst; //rtt评估

	volatile int32_t totalSendSize;
	volatile int32_t totalRecvSize;
	volatile int32_t packageId;
};

static const char* PackageConnectionGetName(PackageConnection* connection)
{
	return connection->name;
}

static inline const BdtPackageConnectionConfig* PackageConnectionGetConfig(PackageConnection* connection)
{
	return &Bdt_ConnectionGetConfig(connection->owner)->package;
}


static PackageConnection* PackageConnectionCreate(
	BdtSystemFramework* framework, 
	BdtConnection* owner)
{
	PackageConnection* connection = BFX_MALLOC_OBJ(PackageConnection);
	BLOG_DEBUG("PackageConnectionCreate address=0x%llx", connection);
	memset(connection, 0, sizeof(PackageConnection));
	connection->state = PACKAGE_CONNECTION_STATE_ESTABLISH;
	connection->flags = 0;
	// no add ref
	connection->owner = owner;
	connection->framework = framework;
	//name
	const char* prename = "PackageConnection:";
	size_t prenameLen = strlen(prename);
	size_t nameLen = prenameLen + BDT_PEERID_STRING_LENGTH + 1;
	connection->name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(connection->name, 0, nameLen);
	memcpy(connection->name, prename, prenameLen);
	BdtPeeridToString(Bdt_ConnectionGetRemotePeerid(owner), connection->name + prenameLen);
	connection->tunnelContainer = Bdt_ConnectionGetTunnelContainer(owner);

	Bdt_TimerHelperInit(&connection->closeTimer, framework);

	connection->rto = PACKAGE_CONNECTION_INIT_RTO;
	connection->lastRecvAckTime = 0;
	Bdt_TimerHelperInit(&connection->rtoTimer, framework);

	Bdt_TimerHelperInit(&connection->recvTimer, framework);


	connection->recvQueue = RecvQueueCreate(PACKAGE_CONNECTION_INIT_STREAM_POS);
	connection->sendQueue = SendQueueCreate(PACKAGE_CONNECTION_INIT_STREAM_POS, &(Bdt_ConnectionGetConfig(owner)->package));

	connection->cc = (BaseCc*)LedbatCcNew(
		Bdt_ConnectionGetConfig(owner)->package.mtu,
		Bdt_ConnectionGetConfig(owner)->package.mss
	);

	BfxListInit(&connection->recvReadyQueue.buffers);
	connection->recvReadyQueue.streamPos = PACKAGE_CONNECTION_INIT_STREAM_POS;
	connection->recvReadyQueue.onNotifying = false;
	BfxThreadLockInit(&connection->lockRecvReadyQueue);

	//PackageConnectionBeginRecvTimer(connection);
	PackageConnectionBeginRtoTimer(connection);

	return connection;
}

static void PackageConnectionDestroy(PackageConnection* connection)
{
	BFX_FREE(connection->name);
	if (connection->tunnelContainer)
	{
		Bdt_TunnelContainerRelease(connection->tunnelContainer);
		connection->tunnelContainer = NULL;
	}
	Bdt_TimerHelperUninit(&connection->closeTimer);
	Bdt_TimerHelperUninit(&connection->rtoTimer);
	Bdt_TimerHelperUninit(&connection->recvTimer);
	BfxThreadLockDestroy(&connection->lockRecvReadyQueue);
	RecvQueueDestory(connection->recvQueue);
	SendQueueDestory(connection->sendQueue);

	if (connection->cc)
	{
		connection->cc->uninit(connection->cc);
		connection->cc = NULL;
	}

	do 
	{
		BfxListItem* item = BfxListPopFront(&connection->recvReadyQueue.buffers);
		if (!item)
		{
			break;
		}
		BFX_FREE(item);
	} while (true);

	BFX_FREE(connection);
}

static void BFX_STDCALL PackageConnectionAsUserDataAddRef(void* userData)
{
	BdtAddRefConnection(((PackageConnection*)userData)->owner);
}

static void BFX_STDCALL PackageConnectionAsUserDataRelease(void* userData)
{
	BdtReleaseConnection(((PackageConnection*)userData)->owner);
}

static void PackageConnectionAsUserData(PackageConnection* connection, BfxUserData* outUserData)
{
	outUserData->lpfnAddRefUserData = PackageConnectionAsUserDataAddRef;
	outUserData->lpfnReleaseUserData = PackageConnectionAsUserDataRelease;
	outUserData->userData = connection;
}

static uint32_t PackageConnectionPushPackage(
	PackageConnection* connection,
	const Bdt_SessionDataPackage* package,
	bool* outHandled
)
{	 
	//BLOG_DEBUG("name=%s,recv package, streamPos=%llu, ackStreamPos=%llu, payloadlen=%d, packagetime=%llu, packageid=%d, flight=%u",connection->name, package->streamPos, package->ackStreamPos, package->payloadLength, package->sendTime, (package->packageIDPart ? package->packageIDPart->packageId : 0), SendQueueGetFilght(connection->sendQueue));
	if (package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN)
	{
		if (package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK)
		{
			PackageConnectionSendAckAck(connection);
		}
		else
		{
			// do nothing
		}
	}
	else
	{
		//BfxAtomicAdd32(&connection->totalRecvSize, package->payloadLength);
		connection->lastRecvAckTime = (uint64_t)BfxTimeGetNow(false);

		PackageConnectionEstimateRtt(connection, package);

		PackageConnectionUpdateCloseSubState(connection, package->cmdflags, true);

		uint32_t nAcked = 0;
		uint32_t errcode = SendQueueConfirmRange(connection->sendQueue, package->ackStreamPos, package->sackArray, &nAcked);
		if (!errcode)
		{
			if (nAcked > 0)
			{
				connection->cc->onAck(connection->cc, package, nAcked, SendQueueGetFilght(connection->sendQueue));
			}
			PackageConnectionTrySendPackage(connection);
			if (nAcked > 0)
			{
				BfxList buffers;
				BfxListInit(&buffers);
				SendQueuePopCompleteBuffers(connection->sendQueue, &buffers);
				if (BfxListSize(&buffers))
				{
					PackageConnectionNotifySendCompleteBuffers(connection, &buffers);
				}
			}
		}

		RecvPackageType recvType = RECV_PACKAGE_TYPE_NONE;
		if (package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FIN)
		{
			connection->remoteCloseStreamPos = package->streamPos + package->payloadLength;
			recvType = RECV_PACKAGE_TYPE_IN_ORDER;
		}
		if (package->payload && package->payloadLength)
		{
			RecvQueueWriteData(connection->recvQueue, package->streamPos, package->payload, package->payloadLength, &recvType);
			//BLOG_DEBUG("name=%s, come payload, streamPos=%llu, length=%d, dealRetType=%d", connection->name, package->streamPos, package->payloadLength, recvType);
		}
		
		if (recvType != RECV_PACKAGE_TYPE_NONE)
		{
			if (!connection->cc->onData(connection->cc, package, recvType))
			{
				PackageConnectionTrySendPackage(connection);
			}

			if (recvType == RECV_PACKAGE_TYPE_IN_ORDER || recvType == RECV_PACKAGE_TYPE_LOST_ORDER || recvType == RECV_PACKAGE_TYPE_PART)
			{
				PackageConnectionBeginRecvTimer(connection);
				BfxList buffers;
				BfxListInit(&buffers);
				RecvQueuePopCompleteBuffers(connection->recvQueue, package->payloadLength < Bdt_ConnectionGetConfig(connection->owner)->package.mtu, &buffers);
				if (BfxListSize(&buffers))
				{
					PackageConnectionNotifyRecvReadyBuffers(connection, &buffers);
				}
			}
		}
	}
	return BFX_RESULT_SUCCESS;
}

static uint32_t PackageConnectionClose(PackageConnection* connection)
{
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSE)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	if (PACKAGE_CONNECTION_STATE_CLOSING != (PackageConnectionState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->state), PACKAGE_CONNECTION_STATE_ESTABLISH, PACKAGE_CONNECTION_STATE_CLOSING))
	{
		if (!SendQueueAddCloseOperation(connection->sendQueue, &connection->selfCloseStreamPos))
		{
			PackageConnectionTrySendPackage(connection);
		}
		return BFX_RESULT_SUCCESS;
	}
	return BFX_RESULT_INVALID_STATE;
}


static uint32_t PackageConnectionReset(PackageConnection* connection)
{
	if (PACKAGE_CONNECTION_STATE_ESTABLISH == (PackageConnectionState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->state), PACKAGE_CONNECTION_STATE_ESTABLISH, PACKAGE_CONNECTION_STATE_CLOSING))
	{
		connection->closeSubState = PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED;
		connection->flags |= (PACKAGE_CONNECTION_FLAG_CLOSE_RECV | PACKAGE_CONNECTION_FLAG_CLOSE_SEND);
		PackageConnectionMaybeCloseFinish(connection);
		return BFX_RESULT_SUCCESS;
	}
	return BFX_RESULT_INVALID_STATE;
}

static uint32_t PackageConnectionSend(
	PackageConnection* connection,
	const uint8_t* buffer,
	size_t length,
	BdtConnectionSendCallback callback,
	const BfxUserData* userData)
{
	BLOG_DEBUG("name=%s,send buffer, size=%d,total unsend size=%llu", connection->name, length, SendQueueGetUnsendSize(connection->sendQueue));
	if (connection->state != PACKAGE_CONNECTION_STATE_ESTABLISH)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	// to optimize: SendQueueAddBuffer的实现可以判断出send queue里面是不是还有pending send？如果有的话，这里不需要再调用PackageConnectionTrySendPackage了
	uint32_t errcode = SendQueueAddBuffer(connection->sendQueue, buffer, length, callback, userData);
	if (errcode)
	{
		return errcode;
	}
	PackageConnectionTrySendPackage(connection);
	return BFX_RESULT_PENDING;
}




static Bdt_TunnelContainer* PackageConnectionGetTunnel(PackageConnection* connection)
{
	return connection->tunnelContainer;
}

static uint32_t PackageConnectionGetRemoteId(PackageConnection* connection)
{
	return ConnectionGetRemoteId(connection->owner);
}

static uint32_t PackageConnectionRecv(PackageConnection* connection, uint8_t* data, size_t len, BdtConnectionRecvCallback callback, const BfxUserData* userData)
{
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSE)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	uint64_t ackedPos = 0;
	RecvQueueGetAckInfo(connection->recvQueue, &ackedPos, NULL);
	BLOG_DEBUG("name=%s,add recv buffer, buffer length=%d, prev recv pos=%llu", connection->name, len, ackedPos);
	return RecvQueueAddBuffer(connection->recvQueue, data, len, callback, userData);
}

static uint32_t PackageConnectionSendAckAck(PackageConnection* connection)
{
	BLOG_DEBUG("%s send ackack", connection->name);
	//TODO 主动段可能需要回复ack，暂时先这样实现
	connection->cc->onData(connection->cc, NULL, RECV_PACKAGE_TYPE_IN_ORDER);
	PackageConnectionTrySendPackage(connection);
	return BFX_RESULT_SUCCESS;
}

static void PackageConnectionOnPreSendPackage(PackageConnection* connection, Bdt_SessionDataPackage* package)
{
	//BfxAtomicAdd32(&connection->totalSendSize, package->payloadLength);
	package->toSessionId = package->sessionId = ConnectionGetRemoteId(connection->owner);

	if (package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FIN)
	{
		PackageConnectionUpdateCloseSubState(connection, package->cmdflags, false);
	}
	if (!connection->lastRecvAckTime && package->payloadLength)
	{
		connection->lastRecvAckTime = (uint64_t)BfxTimeGetNow(false);
	}
	if (connection->cc->onSend)
	{
		connection->cc->onSend(connection->cc, package);
	}

	if (package->payloadLength)
	{
		if (connection->rttEst.ackStreamPos == package->streamPos + package->payloadLength)
		{
			//重复发送,尝试取消用这个包评估
			if (PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT == BfxAtomicCompareAndSwap32(&connection->rttEst.state, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_CANCEL))
			{
				connection->rttEst.ackStreamPos = 0;
				connection->rttEst.state = PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_NONE;
			}
		}
		else if ((connection->rttEst.ackStreamPos == 0) && 
			(connection->rttEst.prevAckStreamPos < (package->streamPos + package->payloadLength)) && 
			((uint64_t)BfxTimeGetNow(false) > 200000 + connection->rttEst.lastEstTime))
		{
			if (PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_NONE == BfxAtomicCompareAndSwap32(&connection->rttEst.state, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_NONE, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_PREPARE))
			{
				connection->rttEst.ackStreamPos = package->streamPos + package->payloadLength;
				connection->rttEst.prevAckStreamPos = connection->rttEst.ackStreamPos;
				connection->rttEst.sendTime = (uint64_t)BfxTimeGetNow(false);
				connection->rttEst.state = PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT;
			}
		}
	}
}

static uint32_t PackageConnectionSendPackage(PackageConnection* connection, Bdt_SessionDataPackage* package)
{
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSE)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	Bdt_Package* packages[] = { (Bdt_Package*)package };
	size_t sent = 1;
	BdtTunnel_Send(connection->tunnelContainer, packages, &sent, NULL);
	if (!connection->lastRecvAckTime && package->payloadLength)
	{
		connection->lastRecvAckTime = (uint64_t)BfxTimeGetNow(false);
	}

	//BLOG_DEBUG("name=%s, send package, streamPos=%llu, ackStreamPos=%llu, payloadlen=%d, packagetime=%llu, fin=%d, finack=%d",connection->name, package->streamPos, package->ackStreamPos, package->payloadLength, package->sendTime, package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FIN ? 1 :0, package->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FINACK ? 1 : 0);

	return BFX_RESULT_SUCCESS;
} 

static uint32_t PackageConnectionFillPackage(PackageConnection* connection, Bdt_SessionDataPackage* package, uint16_t payloadLength)
{
	uint32_t retcode = BFX_RESULT_NOT_CHANGED;

	//payload
	if (payloadLength == Bdt_ConnectionGetConfig(connection->owner)->package.mtu)
	{
		/*package->payloadLength = payloadLength;
		retcode = SendQueueFillPayloadFromOld(connection->sendQueue, &package->streamPos, &package->payload, &package->payloadLength);
		if (retcode)
		{
			package->payloadLength = payloadLength;
			retcode = SendQueueFillPayloadFromNew(connection->sendQueue, &package->streamPos, &package->payload, &package->payloadLength);
		}*/
		retcode = SendQueueFillPayload(connection->sendQueue, &package->streamPos, &package->payload, &package->payloadLength);
	}
	else
	{
		SendQueueFillPayloadFromNew(connection->sendQueue, &package->streamPos, NULL, NULL);
	}

	//fin
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSING &&
		(package->streamPos + package->payloadLength >= connection->selfCloseStreamPos) &&
		(
			connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE ||
			connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1 ||
			connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSING ||
			connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_WAITCLOSE ||
			connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_LASTACK
			)
		)
	{
		package->cmdflags |= BDT_SESSIONDATA_PACKAGE_FLAG_FIN;
		retcode = BFX_RESULT_SUCCESS;
	}

	//fin ack
	uint64_t ackStreamPos = 0;
	RecvQueueGetAckInfo(connection->recvQueue, &ackStreamPos, NULL);
	if (connection->remoteCloseStreamPos == ackStreamPos)
	{
		package->cmdflags |= BDT_SESSIONDATA_PACKAGE_FLAG_FINACK;
		retcode = BFX_RESULT_SUCCESS;
	}

	//other

	//ack+sack
	if (!retcode)
	{
		//TODO 既然这个包总会发送，那么就带上ack+sack，是否可以这样？
		RecvQueueGetAckInfo(connection->recvQueue, &package->ackStreamPos, &package->sackArray);
	}
	else
	{
		if (!connection->cc->trySendAck(connection->cc))
		{
			RecvQueueGetAckInfo(connection->recvQueue, &package->ackStreamPos, &package->sackArray);
			retcode = BFX_RESULT_SUCCESS;
		}
	}

	return retcode;
}

static void PackageConnectionSendDrive(PackageConnection* connection)
{
	do
	{
		uint16_t payloadLength = 0;
		connection->cc->trySendPayload(connection->cc, SendQueueGetFilght(connection->sendQueue), &payloadLength);

		Bdt_SessionDataPackage p = { 0 };
		p.cmdtype = BDT_SESSION_DATA_PACKAGE;

		if (!PackageConnectionFillPackage(connection, &p, payloadLength))
		{
			PackageConnectionOnPreSendPackage(connection, &p);
			if (PackageConnectionSendPackage(connection, &p))
			{
				Bdt_SessionDataPackageFinalize(&p);
				break;
			}
			Bdt_SessionDataPackageFinalize(&p);
		}
		else
		{
			break;
		}

		//没有数据需要发送了
		if (!payloadLength || SendQueueGetUnsendSize(connection->sendQueue) + SendQueueGetLostSize(connection->sendQueue) == 0)
		{
			//BLOG_DEBUG("-----------nothing to send payloadLength=%d, unsend=%llu,lost=%lld, flight=%u", payloadLength, SendQueueGetUnsendSize(connection->sendQueue), SendQueueGetLostSize(connection->sendQueue), SendQueueGetFilght(connection->sendQueue));
			break;
		}
	} while (true);
}

static void PackageConnectionTrySendPackage(PackageConnection* connection)
{
	int32_t v = BfxAtomicInc32(&connection->trySendCnt);
	if (v > 1)
	{
		return;
	}

	do 
	{
		PackageConnectionSendDrive(connection);
		v = BfxAtomicAdd32(&connection->trySendCnt, -v);
	} while (v);
	//BLOG_DEBUG("========send finish, flight=%u, unsend=%llu", SendQueueGetFilght(connection->sendQueue), SendQueueGetUnsendSize(connection->sendQueue));
}

static void PackageConnectionMaybeCloseFinish(PackageConnection* connection)
{
	if (connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED && (connection->flags & (PACKAGE_CONNECTION_FLAG_CLOSE_RECV | PACKAGE_CONNECTION_FLAG_CLOSE_SEND)))
	{
		if (PACKAGE_CONNECTION_STATE_CLOSING == BfxAtomicCompareAndSwap32((int32_t*)(&connection->state), PACKAGE_CONNECTION_STATE_CLOSING, PACKAGE_CONNECTION_STATE_CLOSE))
		{
			Bdt_TimerHelperStop(&connection->closeTimer);
			Bdt_TimerHelperStop(&connection->rtoTimer);
			Bdt_TimerHelperStop(&connection->recvTimer);
		
			BLOG_DEBUG("close finish, begin to clean and notify buffer");
			//清理recv buffer
			BfxList buffers;
			BfxListInit(&buffers);
			RecvQueuePopAllBuffers(connection->recvQueue, &buffers);
			do 
			{
				RecvQueueRecvBuffer* buffer = (RecvQueueRecvBuffer*)BfxListPopFront(&buffers);
				if (!buffer)
				{
					break;
				}

				buffer->callback(buffer->data, 0, buffer->userData.userData);
				if (buffer->userData.lpfnReleaseUserData)
				{
					buffer->userData.lpfnReleaseUserData(buffer->userData.userData);
				}
				BFX_FREE(buffer);
			} while (true);

			//清理send buffer
			SendQueuePopAllBuffers(connection->sendQueue, &buffers);
			do
			{
				SendQueueSendBuffer* buffer = (SendQueueSendBuffer*)BfxListPopFront(&buffers);
				if (!buffer)
				{
	
					break;
				}

				buffer->callback(BFX_RESULT_SUCCESS, buffer->data, buffer->len, buffer->userData.userData);
				if (buffer->userData.lpfnReleaseUserData)
				{
					buffer->userData.lpfnReleaseUserData(buffer->userData.userData);
				}
				BFX_FREE(buffer);
			} while (true);
		}
	}
}

static RecvQueue* PackageConnectionGetRecvQueue(PackageConnection* connection)
{
	return connection->recvQueue;
}

static SendQueue* PackageConnectionGetSendQueue(PackageConnection* connection)
{
	return connection->sendQueue;
}

static void PackageConnectionUpdateCloseSubState(PackageConnection* connection, uint16_t cmdflags, bool recv)
{
	if (recv)
	{
		if (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FIN)
		{
			if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE, PACKAGE_CONNECTION_CLOSE_SUBSTATE_WAITCLOSE))
			{
				BLOG_DEBUG("======waitclose");
				if (!Bdt_ConnectionGetConfig(connection->owner)->package.halfOpen)
				{
					PackageConnectionClose(connection);
				}
			}
			else if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1 == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1, PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSING))
			{
				BLOG_DEBUG("======closing");
			}
			else if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN2 == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN2, PACKAGE_CONNECTION_CLOSE_SUBSTATE_TIMEWAIT))
			{
				BLOG_DEBUG("======timewait");
				Bdt_TimerHelperStop(&connection->closeTimer);
				PackageConnectionBeginTimewaitTimer(connection);
			}
		}
		if (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FINACK)
		{
			if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1 == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1, PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN2))
			{
				BLOG_DEBUG("======fin2");
				connection->flags |= PACKAGE_CONNECTION_FLAG_CLOSE_SEND;
				Bdt_TimerHelperStop(&connection->closeTimer);
			}
			else if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSING == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSING, PACKAGE_CONNECTION_CLOSE_SUBSTATE_TIMEWAIT))
			{
				BLOG_DEBUG("======timewait");
				connection->flags |= PACKAGE_CONNECTION_FLAG_CLOSE_SEND;
				Bdt_TimerHelperStop(&connection->closeTimer);
				PackageConnectionBeginTimewaitTimer(connection);
			}
			else if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_LASTACK == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_LASTACK, PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED))
			{
				BLOG_DEBUG("======closeed");
				connection->flags |= PACKAGE_CONNECTION_FLAG_CLOSE_SEND;
				Bdt_TimerHelperStop(&connection->closeTimer);
				PackageConnectionMaybeCloseFinish(connection);
			}
		}
	}
	else
	{
		if (cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_FIN)
		{
			if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_NONE, PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN1))
			{
				BLOG_DEBUG("======fin1");
				PackageConnectionBeginFinTimer(connection);
			}
			else if (PACKAGE_CONNECTION_CLOSE_SUBSTATE_WAITCLOSE == (PackageConnectionCloseSubState)BfxAtomicCompareAndSwap32((int32_t*)(&connection->closeSubState), PACKAGE_CONNECTION_CLOSE_SUBSTATE_WAITCLOSE, PACKAGE_CONNECTION_CLOSE_SUBSTATE_LASTACK))
			{
				BLOG_DEBUG("======lastack");
				connection->flags |= PACKAGE_CONNECTION_FLAG_CLOSE_RECV;
				PackageConnectionBeginFinTimer(connection);
			}
		}
	}
}


static void BFX_STDCALL PackageConnectionFinTimerCallback(BDT_SYSTEM_TIMER timer, void* userData)
{
	PackageConnection* connection = (PackageConnection*)userData;
	Bdt_TimerHelperStop(&connection->closeTimer);
	if (connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_FIN2 ||
		connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_TIMEWAIT ||
		connection->closeSubState == PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED
		)
	{
		return;
	}
	PackageConnectionTrySendPackage(connection);
	PackageConnectionBeginFinTimer(connection);
}

static void BFX_STDCALL PackageConnectionAsRefUserDataAddRef(void* userData)
{
	BdtAddRefConnection(((PackageConnection*)userData)->owner);
}

static void BFX_STDCALL PackageConnectionAsRefUserDataRelease(void* userData)
{
	BdtReleaseConnection(((PackageConnection*)userData)->owner);
}

static void PackageConnectionAsRefUserData(PackageConnection* connection, BfxUserData* outUserData)
{
	outUserData->userData = connection;
	outUserData->lpfnAddRefUserData = PackageConnectionAsRefUserDataAddRef;
	outUserData->lpfnReleaseUserData = PackageConnectionAsRefUserDataRelease;
}

static void PackageConnectionBeginFinTimer(PackageConnection* connection)
{
	BfxUserData userData;
	PackageConnectionAsRefUserData(connection, &userData);

	Bdt_TimerHelperRestart(&connection->closeTimer,
		PackageConnectionFinTimerCallback,
		&userData,
		Bdt_ConnectionGetConfig(connection->owner)->package.resendInterval);
}

static void BFX_STDCALL PackageConnectionTimewaitTimerCallback(BDT_SYSTEM_TIMER timer, void* userData)
{
	PackageConnection* connection = (PackageConnection*)userData;
	Bdt_TimerHelperStop(&connection->closeTimer);
	BLOG_DEBUG("======closed");
	connection->flags |= PACKAGE_CONNECTION_FLAG_CLOSE_RECV;
	connection->closeSubState = PACKAGE_CONNECTION_CLOSE_SUBSTATE_CLOSED;
	PackageConnectionMaybeCloseFinish(connection);
}

static void PackageConnectionBeginTimewaitTimer(PackageConnection* connection)
{
	BfxUserData userData;
	PackageConnectionAsRefUserData(connection, &userData);

	Bdt_TimerHelperRestart(&connection->closeTimer,
		PackageConnectionTimewaitTimerCallback,
		&userData,
		2 * Bdt_ConnectionGetConfig(connection->owner)->package.msl);
}


static void BFX_STDCALL PackageConnectionRtoTimerCallback(BDT_SYSTEM_TIMER timer, void* userData)
{
	PackageConnection* connection = (PackageConnection*)userData;
	Bdt_TimerHelperStop(&connection->rtoTimer);
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSE)
	{
		return;
	}

	if (( SendQueueGetFilght(connection->sendQueue) > 0)
		|| (SendQueueGetLostSize(connection->sendQueue) > 0)
		 || (SendQueueGetUnsendSize(connection->sendQueue) > 0)
		)
	{
		uint64_t now = (uint64_t)BfxTimeGetNow(false);
		/*if (connection->lastRecvAckTime && ((now - connection->lastRecvAckTime) > 60000000))
		{
			BLOG_DEBUG("name=%s,============rto now=%llu, last ack time=%llu", connection->name, now, connection->lastRecvAckTime);
			SendQueueMarkRangeTimeout(connection->sendQueue, true, connection->rto);
			connection->cc->onChangeState(connection->cc, CC_STATE_OPEN, CC_STATE_RTO);
			PackageConnectionTrySendPackage(connection);
			connection->lastRecvAckTime = now;
		}
		else
		{*/
			uint32_t nCount = SendQueueMarkRangeTimeout(connection->sendQueue, false, connection->rto);
			if (nCount > 0)
			{
				BLOG_DEBUG("name=%s, not recv ack on timeout,size=%d", connection->name, nCount);
				//先把丢失的包发出去，再更新窗口
				PackageConnectionTrySendPackage(connection);
				if (now - connection->lastLostStateTime > 200000)
				{
					connection->cc->onChangeState(connection->cc, CC_STATE_OPEN, CC_STATE_LOST);
					connection->lastLostStateTime = now;
				}
				//TODO 这里需要优化
				//SendQueueMarkRangeTimeout(connection->sendQueue, true, connection->rto);
				
			}
		//}
	}
	else
	{
		connection->lastRecvAckTime = 0;
	}
	PackageConnectionBeginRtoTimer(connection);
}

static void PackageConnectionBeginRtoTimer(PackageConnection* connection)
{
	BfxUserData userData;
	PackageConnectionAsRefUserData(connection, &userData);

	Bdt_TimerHelperStart(&connection->rtoTimer,
		(LPFN_TIMER_PROCESS)PackageConnectionRtoTimerCallback,
		&userData,
		PACKAGE_CONNECTION_CHECK_RTO_INTERVAL);
}

static void BFX_STDCALL PackageConnectionRecvTimerCallback(BDT_SYSTEM_TIMER timer, void* userData)
{
	PackageConnection* connection = (PackageConnection*)userData;
	//Bdt_TimerHelperStop(&connection->recvTimer);
	if (connection->state == PACKAGE_CONNECTION_STATE_CLOSE)
	{
		return;
	}

	BfxList buffers;
	BfxListInit(&buffers);
	RecvQueuePopCompleteBuffers(connection->recvQueue, true, &buffers);
	if (BfxListSize(&buffers))
	{
		PackageConnectionNotifyRecvReadyBuffers(connection, &buffers);
	}
	//PackageConnectionBeginRecvTimer(connection);
}

static void PackageConnectionBeginRecvTimer(PackageConnection* connection)
{
	BfxUserData userData;
	PackageConnectionAsRefUserData(connection, &userData);

	Bdt_TimerHelperRestart(&connection->recvTimer,
		(LPFN_TIMER_PROCESS)PackageConnectionRecvTimerCallback,
		&userData,
		Bdt_ConnectionGetConfig(connection->owner)->package.recvTimeout);
}

static void PackageConnectionNotifyRecvReadyBuffers(PackageConnection* connection, BfxList* buffers)
{
	//BLOG_DEBUG("------------total recv=%d", connection->totalRecvSize);
	BfxThreadLockLock(&connection->lockRecvReadyQueue);
	RecvQueueRecvBuffer* bufferSrc = (RecvQueueRecvBuffer*)BfxListFirst(buffers);
	RecvQueueRecvBuffer* bufferDst = (RecvQueueRecvBuffer*)BfxListLast(&connection->recvReadyQueue.buffers);
	do
	{
		if (!bufferDst)
		{
			if (!BfxListSize(&connection->recvReadyQueue.buffers))
			{
				BfxListMerge(&connection->recvReadyQueue.buffers, buffers, true);
			}
			else
			{
				BfxListMerge(&connection->recvReadyQueue.buffers, buffers, false);
			}
			break;
		}
		if (bufferSrc->startStreamPos >= bufferDst->endStreamPos)
		{
			if (BfxListSize(buffers))
			{
				BfxListMergeAfter(&connection->recvReadyQueue.buffers, buffers, (BfxListItem*)bufferDst);
			}
			
			break;
		}
		bufferDst = (RecvQueueRecvBuffer*)BfxListPrev(&connection->recvReadyQueue.buffers, (BfxListItem*)bufferDst);
	} while (true);
	bool notifying = connection->recvReadyQueue.onNotifying;
	connection->recvReadyQueue.onNotifying = true;
	BfxThreadLockUnlock(&connection->lockRecvReadyQueue);

	if (!notifying)
	{
		while (true)
		{
            // <TODO>可以一次性取出所有待通知的buffer，减少lock次数
			BfxThreadLockLock(&connection->lockRecvReadyQueue);
			RecvQueueRecvBuffer* buffer = (RecvQueueRecvBuffer*)BfxListFirst(&connection->recvReadyQueue.buffers);
			if (buffer && buffer->startStreamPos == connection->recvReadyQueue.streamPos)
			{
				BfxListPopFront(&connection->recvReadyQueue.buffers);
				connection->recvReadyQueue.streamPos = buffer->endStreamPos;
			}
			else
			{
				buffer = NULL;
				connection->recvReadyQueue.onNotifying = false;
			}
			BfxThreadLockUnlock(&connection->lockRecvReadyQueue);

			if (buffer)
			{
				BLOG_DEBUG("name=%s,recv buffer begin notify,begin pos=%llu,length=%llu", connection->name, buffer->startStreamPos, buffer->endStreamPos - buffer->startStreamPos);
                size_t recvSize = buffer->endStreamPos - buffer->startStreamPos;
                BdtStat_OnConnectionRecv(recvSize, true);
                buffer->callback(buffer->data, recvSize, buffer->userData.userData);
				if (buffer->userData.lpfnReleaseUserData)
				{
					buffer->userData.lpfnReleaseUserData(buffer->userData.userData);
				}
				BFX_FREE(buffer);
			}
			else
			{
				break;
			}
		}
	}
}

static void PackageConnectionNotifySendCompleteBuffers(PackageConnection* connection, BfxList* buffers)
{
	//BLOG_DEBUG("-----------------total send=%d", connection->totalSendSize);
	do
	{
		SendQueueSendBuffer* buffer = (SendQueueSendBuffer*)BfxListPopFront(buffers);
		if (!buffer)
		{
			break;
		}
		assert(buffer->callback);
		buffer->callback(BFX_RESULT_SUCCESS, buffer->data, buffer->len, buffer->userData.userData);
		if (buffer->userData.lpfnReleaseUserData)
		{
			buffer->userData.lpfnReleaseUserData(buffer->userData.userData);
		}
		BFX_FREE(buffer);
	} while (true);
}

static void PackageConnectionEstimateRttFromLinuxTcp(PackageConnection* connection, uint32_t mrtt)
{
	long m = mrtt; /*此为得到的新的RTT测量值*/

	/* The following amusing code comes from Jacobson's article in
	 * SIGCOMM '88. Note that rtt and mdev are scaled versions of rtt and
	 * mean deviation. This is designed to be as fast as possible
	 * m stands for "measurement".
	 *
	 * On a 1990 paper the rto value is changed to :
	 * RTO = rtt + 4 * mdev
	 *
	 * Funny. This algorithm seems to be very broken.
	 * These formulae increase RTO, when it should be decreased, increase
	 * too slowly, when it should be increased quickly, decrease too quickly
	 * etc. I guess in BSD RTO takes ONE value, so that it is absolutely does
	 * not matter how to calculate it. Seems, it was trap that VJ failed to
	 * avoid. 8)
	 */
	if (m == 0)
		m = 1; /* RTT的采样值不能为0 */

	/* 不是得到第一个RTT采样*/
	if (connection->rttEst.srtt != 0) {
		m -= (connection->rttEst.srtt >> 3); /* m is now error in rtt est */
		connection->rttEst.srtt += m; /* rtt = 7/8 rtt + 1/8 new ，更新srtt*/

		if (m < 0) { /*RTT变小*/
			m = -m; /* m is now abs(error) */
			m -= (connection->rttEst.mdev >> 2); /* similar update on mdev */

			/* This is similar to one of Eifel findings.
			 * Eifel blocks mdev updates when rtt decreases.
			 * This solution is a bit different : we use finer gain
			 * mdev in this case (alpha * beta).
			 * Like Eifel it also prevents growth of rto, but also it
			 * limits too fast rto decreases, happening in pure Eifel.
			 */
			if (m > 0) /* |err| > 1/4 mdev */
				m >>= 3;

		}
		else { /* RTT变大 */
			m -= (connection->rttEst.mdev >> 2); /* similar update on mdev */
		}

		connection->rttEst.mdev += m; /* mdev = 3/4 mdev + 1/4 new，更新mdev */

		/* 更新mdev_max和rttvar */
		if (connection->rttEst.mdev > connection->rttEst.mdev_max) {
			connection->rttEst.mdev_max = connection->rttEst.mdev;
			if (connection->rttEst.mdev_max > connection->rttEst.rttvar)
				connection->rttEst.rttvar = connection->rttEst.mdev_max;
		}

		/* 过了一个RTT了，更新mdev_max和rttvar */
		//if (after(connection->rttEst.snd_una, connection->rttEst.rtt_seq)) {
			if (connection->rttEst.mdev_max < connection->rttEst.rttvar)/*减小rttvar */
				connection->rttEst.rttvar -= (connection->rttEst.rttvar - connection->rttEst.mdev_max) >> 2;
			//connection->rttEst.rtt_seq = connection->rttEst.snd_nxt;
			connection->rttEst.mdev_max = 200000;//tcp_rto_min(sk); /*重置mdev_max */
		//}

	}
	else {
		/* 获得第一个RTT采样*/
			/* no previous measure. */
		connection->rttEst.srtt = m << 3; /* take the measured time to be rtt */
		connection->rttEst.mdev = m << 1; /* make sure rto = 3 * rtt */
		connection->rttEst.mdev_max = connection->rttEst.rttvar = connection->rttEst.mdev > PACKAGE_CONNECTION_MIN_RTO ? connection->rttEst.mdev : PACKAGE_CONNECTION_MIN_RTO;
		//connection->rttEst.mdev_max = connection->rttEst.rttvar = max(connection->rttEst.mdev, tcp_rto_min(sk));
		//connection->rttEst.rtt_seq = connection->rttEst.snd_nxt; /*设置更新mdev_max的时间*/
	}

	uint32_t rto = (connection->rttEst.srtt >> 3) + connection->rttEst.rttvar;
	connection->rto = rto;
	BLOG_DEBUG("name=%s,====rtt=%u,rto=%u", connection->name, connection->rttEst.srtt >> 3, rto);
}

static void PackageConnectionEstimateRtt(PackageConnection* connection, const Bdt_SessionDataPackage* package)
{
	if (PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT != BfxAtomicCompareAndSwap32(&connection->rttEst.state, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT, PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_CALC))
	{
		return;
	}

	bool acked = false;
	if (connection->rttEst.ackStreamPos)
	{
		if (connection->rttEst.ackStreamPos <= package->ackStreamPos)
		{
			acked = true;
		}
		else if (package->sackArray)
		{
			int i = 0;
			while (package->sackArray[i].length)
			{
				if ((connection->rttEst.ackStreamPos > package->sackArray[i].pos) && (connection->rttEst.ackStreamPos <= (package->sackArray[i].pos + package->sackArray[i].length)))
				{
					acked = true;
					break;
				}
				i++;
			}
		}
	}

	if (!acked)
	{
		connection->rttEst.state = PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_WAIT;
		return;
	}

	uint64_t now = (uint64_t)BfxTimeGetNow(false);
	uint32_t rtt = (uint32_t)(now - connection->rttEst.sendTime);
	//uint32_t rto = 0;
	//if (!(connection->rttEst.rtt))
	//{
	//	// SRTT <- R
	//	// RTTVAR <- R/2
	//	// RTO <- SRTT + max (G, K*RTTVAR)
	//	connection->rttEst.rtt = rtt;
	//	connection->rttEst.rttval = rtt / 2;
	//	rto = rtt + 4 * connection->rttEst.rttval;
	//}
	//else
	//{
	//	// RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
	//	// SRTT <- (1 - alpha) * SRTT + alpha * R'
	//	// RTO <- SRTT + max (G, K*RTTVAR)
	//	// beta = 0.25  alpha = 0.125
	//	/*connection->rttEst.rttval = (3 * connection->rttEst.rttval + ((connection->rttEst.rtt > rtt) ? (connection->rttEst.rtt - rtt) : (rtt - connection->rttEst.rtt))) / 4;
	//	connection->rttEst.rtt = (7 * connection->rttEst.rtt + rtt) / 8;
	//	rto = connection->rttEst.rtt + 4 * connection->rttEst.rttval;*/

	//	uint32_t delta = ((connection->rttEst.rtt > rtt) ? (connection->rttEst.rtt - rtt) : (rtt - connection->rttEst.rtt));
	//	connection->rttEst.rttval = connection->rttEst.rttval + (int)((int)delta - connection->rttEst.rttval) / 4;
	//	connection->rttEst.rtt = (7* connection->rttEst.rtt + rtt) / 8;
	//	rto = connection->rttEst.rtt + 4 * connection->rttEst.rttval;

	//	//const int delta = (int)rtt - ertt;
	//	//rtt_var = rtt_var + (int)(abs(delta) - rtt_var) / 4;
	//	//rtt = rtt - rtt / 8 + ertt / 8;
	//}

	//BLOG_DEBUG("name=%s,rtt=%d,rto=%d,this package rtt=%u, rttval=%u",connection->name, connection->rttEst.rtt, rto, rtt, connection->rttEst.rttval);

	//if (rto < PACKAGE_CONNECTION_MIN_RTO)
	//{
	//	rto = PACKAGE_CONNECTION_MIN_RTO;
	//}
	//connection->rto = rto;

	PackageConnectionEstimateRttFromLinuxTcp(connection, rtt);
	connection->rttEst.lastEstTime = now;
	connection->rttEst.ackStreamPos = 0;
	connection->rttEst.state = PACKAGE_CONNECTION_RTT_ESTIMATE_STATE_NONE;
}