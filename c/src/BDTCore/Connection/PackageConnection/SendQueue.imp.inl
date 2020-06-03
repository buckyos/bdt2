#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "./SendQueue.inl"
#include <SGLib/SGLib.h>
#include "./PackageConnection.inl"

struct SendQueue
{
	BdtSendCache cache;
	BfxThreadLock lockCache;

	struct
	{
		uint64_t ackStreamPos;
		//发送窗口中的最大pos
		uint64_t streamPos;
		BfxList lostBlocks;
		uint64_t lost;
		BfxList flightBlocks;
		uint32_t flight;
		uint64_t latestAckStreamPos;
		uint64_t prevDupAckStreamPos;
		uint8_t dupAckTimes;
	} sendContext;
	BfxThreadLock lockSendContext;

	struct
	{
		//所有buffer的结束pos
		uint64_t streamPos;
		uint64_t unSendLength;
		SendQueueSendBuffer* curr;
		BfxList buffers;
	} sendingBuffers;
	BfxThreadLock lockSendingBuffers;

	uint16_t mtu;
};

static void BdtSendCacheInit(BdtSendCache* cache, size_t maxsize, uint16_t mtu)
{
	memset(cache, 0, sizeof(BdtSendCache));
	BfxListInit(&(cache->freeUnit));
	BfxListInit(&(cache->useUnit));
	size_t count = maxsize / mtu;
	if (!count)
	{
		count = 1;
	}
	cache->unitSize = mtu;

	size_t len = count * (sizeof(BdtSendCacheUnit) + mtu);
	cache->data = (uint8_t*)BfxMalloc(len);
	memset(cache->data, 0, len);
	for (int i = 0; i < count; i++)
	{
		BfxListPushBack(&(cache->freeUnit), (BfxListItem*)(cache->data + i * (sizeof(BdtSendCacheUnit) + mtu)));
	}
}
static void BdtSendCacheUninit(BdtSendCache* cache)
{
	BfxFree(cache->data);
}

static BdtSendCacheUnit* BdtSendCacheNewUnit(BdtSendCache* cache)
{
	if (BfxListSize(&(cache->freeUnit)))
	{
		BfxListItem* item = BfxListPopFront(&(cache->freeUnit));
		BfxListPushBack(&(cache->useUnit), item);
		return (BdtSendCacheUnit*)item;
	}

	return NULL;
}

static void BdtSendCacheDeleteUnit(BdtSendCache* cache, BdtSendCacheUnit* unit)
{
	BfxListErase(&(cache->useUnit), (BfxListItem*)unit);
	BfxListPushBack(&(cache->freeUnit), (BfxListItem*)unit);
}

static BdtSendCacheUnit* BdtSendCacheUnitFromData(BdtSendCache* cache, const uint8_t* data)
{
	return (BdtSendCacheUnit*)(data - sizeof(BdtSendCacheUnit));
}

static uint8_t* BdtSendCacheUnitToData(BdtSendCache* cache, BdtSendCacheUnit* unit)
{
	return (uint8_t*)unit + sizeof(BdtSendCacheUnit);
}


static SendBlockCheckState CheckBlockStateByAck(SendQueueSendBlock* block, uint64_t ackStreamPos, BdtStreamRange* sackArray)
{
	//全部确认
	if (block->endStreamPos <= ackStreamPos)
	{
		return SEND_BLOCK_CHECK_STATE_ACK;
	}
	//部分确认
	if (block->startStreamPos < ackStreamPos)
	{
		return SEND_BLOCK_CHECK_STATE_LOST;
	}

	if (!sackArray || !sackArray[0].length)
	{
		return SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE;
	}

	int64_t now = BfxTimeGetNow(false);
	int i = 0;
	while (sackArray[i].length)
	{
		if (block->endStreamPos <= sackArray[i].pos)
		{
			return SEND_BLOCK_CHECK_STATE_PREV;
		}
		else if (block->startStreamPos >= sackArray[i].pos && block->endStreamPos <= (sackArray[i].pos + sackArray[i].length))
		{
			return SEND_BLOCK_CHECK_STATE_ACK;
		}
		i++;
	}
	return SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE;
}

//从sendingbuffer里面打包新数据
static SendQueueSendBlock* SendQueueGenerateBlock(SendQueue* queue, uint16_t len)
{
	SendQueueSendBlock* block = NULL;
	BfxThreadLockLock(&queue->lockSendingBuffers);
	if (queue->sendingBuffers.unSendLength)
	{
		block = BFX_MALLOC_OBJ(SendQueueSendBlock);
		memset(block, 0, sizeof(SendQueueSendBlock));

		size_t leftLen = (size_t)(queue->sendingBuffers.curr->endStreamPos - queue->sendingBuffers.curr->offsetStreamPos);
		assert(leftLen);
		if (leftLen <= len)
		{
			block->data = (uint8_t*)(queue->sendingBuffers.curr->data + (queue->sendingBuffers.curr->len - leftLen));
			block->endStreamPos = queue->sendingBuffers.curr->endStreamPos;
			block->startStreamPos = queue->sendingBuffers.curr->offsetStreamPos;

			queue->sendingBuffers.curr->offsetStreamPos = queue->sendingBuffers.curr->endStreamPos;
			queue->sendingBuffers.curr = (SendQueueSendBuffer*)BfxListNext(&queue->sendingBuffers.buffers, (BfxListItem*)(queue->sendingBuffers.curr));
		}
		else
		{
			block->data = (uint8_t*)(queue->sendingBuffers.curr->data + (queue->sendingBuffers.curr->len - leftLen));
			block->startStreamPos = queue->sendingBuffers.curr->offsetStreamPos;
			block->endStreamPos = block->startStreamPos + len;

			queue->sendingBuffers.curr->offsetStreamPos += len;
		}
		queue->sendingBuffers.unSendLength -= (block->endStreamPos - block->startStreamPos);
	}
	BfxThreadLockUnlock(&queue->lockSendingBuffers);
	return block;
}

static void SendQueueMoveLostToFlight(SendQueue* queue, SendQueueSendBlock* lostBlock)
{
	SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListFirst(&(queue->sendContext.flightBlocks));
	SendQueueSendBlock* prev = NULL;
	while (block)
	{
		if (lostBlock->startStreamPos < block->startStreamPos)
		{
			break;
		}

		prev = block;
		block = (SendQueueSendBlock*)BfxListNext(&(queue->sendContext.flightBlocks), (BfxListItem*)block);
	}

	if (!prev)
	{
		BfxListPushFront(&(queue->sendContext.flightBlocks), (BfxListItem*)lostBlock);
	}
	else
	{
		BfxListInsertAfter(&(queue->sendContext.flightBlocks), (BfxListItem*)prev, (BfxListItem*)lostBlock);
	}
}



static SendQueue* SendQueueCreate(uint64_t streamPos, const BdtPackageConnectionConfig* config)
{
	SendQueue* queue = BFX_MALLOC_OBJ(SendQueue);
	memset(queue, 0, sizeof(SendQueue));

	BdtSendCacheInit(&queue->cache, config->maxSendBuffer, config->mtu);
	BfxThreadLockInit(&queue->lockCache);

	queue->sendContext.ackStreamPos = streamPos;
	queue->sendContext.streamPos = streamPos;
	BfxListInit(&queue->sendContext.lostBlocks);
	queue->sendContext.lost = 0;
	BfxListInit(&queue->sendContext.flightBlocks);
	queue->sendContext.flight = 0;
	BfxThreadLockInit(&queue->lockSendContext);

	queue->sendingBuffers.streamPos = streamPos;
	queue->sendingBuffers.unSendLength = 0;
	queue->sendingBuffers.curr = 0;
	BfxListInit(&queue->sendingBuffers.buffers);
	BfxThreadLockInit(&queue->lockSendingBuffers);

	queue->mtu = config->mtu;

	return queue;
}

static void SendQueueDestory(SendQueue* queue)
{
	BdtSendCacheUninit(&queue->cache);
	BfxThreadLockDestroy(&queue->lockCache);
	BfxThreadLockDestroy(&queue->lockSendContext);
	BfxThreadLockDestroy(&queue->lockSendingBuffers);
	do 
	{
		BfxListItem* item = BfxListPopFront(&queue->sendContext.lostBlocks);
		if (!item)
		{
			break;
		}

		BFX_FREE(item);
	} while (true);

	do
	{
		BfxListItem* item = BfxListPopFront(&queue->sendContext.flightBlocks);
		if (!item)
		{
			break;
		}

		BFX_FREE(item);
	} while (true);

	do
	{
		BfxListItem* item = BfxListPopFront(&queue->sendingBuffers.buffers);
		if (!item)
		{
			break;
		}

		BFX_FREE(item);
	} while (true);

	BFX_FREE(queue);
}

static uint32_t SendQueueAddBuffer(
	SendQueue* queue, 
	const uint8_t* data, 
	size_t len, 
	BdtConnectionSendCallback callback, 
	const BfxUserData* userData)
{
	if (!data || !len || !callback)
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	uint32_t errcode = BFX_RESULT_SUCCESS;

	/*BfxThreadLockLock(&queue->lockSendingBuffers);
	do
	{
		SendQueueSendBuffer* buffer = (SendQueueSendBuffer*)BfxListFirst(&queue->sendingBuffers.buffers);
		do
		{
			if (!buffer || buffer->data == data)
			{
				break;
			}

			buffer = (SendQueueSendBuffer*)BfxListNext(&queue->sendingBuffers.buffers, (BfxListItem*)buffer);
		} while (true);

		if (!buffer)
		{
			buffer = BFX_MALLOC_OBJ(SendQueueSendBuffer);
			memset(buffer, 0, sizeof(SendQueueSendBuffer));
			buffer->data = data;
			buffer->len = len;
			buffer->startStreamPos = queue->sendingBuffers.streamPos;
			buffer->offsetStreamPos = queue->sendingBuffers.streamPos;
			buffer->endStreamPos = buffer->offsetStreamPos + buffer->len;
			buffer->callback = callback;
			buffer->userData = *userData;
			if (buffer->userData.lpfnAddRefUserData)
			{
				buffer->userData.lpfnAddRefUserData(buffer->userData.userData);
			}
			BfxListPushBack(&queue->sendingBuffers.buffers, (BfxListItem*)buffer);
			queue->sendingBuffers.unSendLength += len;

			if (!queue->sendingBuffers.curr)
			{
				queue->sendingBuffers.curr = buffer;
			}

			queue->sendingBuffers.streamPos = buffer->endStreamPos;
		}
		else
		{
			assert(false);
			errcode = BFX_RESULT_ALREADY_EXISTS;
		}
	} while (false);
	BfxThreadLockUnlock(&queue->lockSendingBuffers);*/
	
	// optimize: 前面这段遍历不需要，直接走 if (!buffer) 的逻辑就好
	SendQueueSendBuffer* buffer = BFX_MALLOC_OBJ(SendQueueSendBuffer);
	memset(buffer, 0, sizeof(SendQueueSendBuffer));
	buffer->data = data;
	buffer->len = len;
	buffer->callback = callback;
	buffer->userData = *userData;
	if (buffer->userData.lpfnAddRefUserData)
	{
		buffer->userData.lpfnAddRefUserData(buffer->userData.userData);
	}

	BfxThreadLockLock(&queue->lockSendingBuffers);
	uint64_t unSend = queue->sendingBuffers.unSendLength;
	do
	{
		buffer->startStreamPos = queue->sendingBuffers.streamPos;
		buffer->offsetStreamPos = queue->sendingBuffers.streamPos;
		buffer->endStreamPos = buffer->offsetStreamPos + buffer->len;
		BfxListPushBack(&queue->sendingBuffers.buffers, (BfxListItem*)buffer);
		queue->sendingBuffers.unSendLength += len;
		if (!queue->sendingBuffers.curr)
		{
			queue->sendingBuffers.curr = buffer;
		}
		queue->sendingBuffers.streamPos = buffer->endStreamPos;
	} while (false);
	BfxThreadLockUnlock(&queue->lockSendingBuffers);
	
	return unSend > 0 ? BFX_RESULT_PENDING : BFX_RESULT_SUCCESS;
}

static void BFX_STDCALL SendQueueOnClose(uint32_t result, const uint8_t* buffer, size_t length, void* userData)
{
	SendQueue* queue = (SendQueue*)userData;
	do
	{
		SendQueueSendBuffer* buffer = (SendQueueSendBuffer*)BfxListPopFront(&queue->sendingBuffers.buffers);
		if (!buffer)
		{
			break;
		}
		buffer->callback(BFX_RESULT_INVALID_STATE, buffer->data, 0, buffer->userData.userData);
		if (buffer->userData.lpfnReleaseUserData)
		{
			buffer->userData.lpfnReleaseUserData(buffer->userData.userData);
		}
		BFX_FREE(buffer);
	} while (true);


	do
	{
		SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListPopFront(&queue->sendContext.flightBlocks);
		if (!block)
		{
			break;
		}
		BFX_FREE(block);
	} while (true);

	do
	{
		SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListPopFront(&queue->sendContext.lostBlocks);
		if (!block)
		{
			break;
		}
		BFX_FREE(block);
	} while (true);
}

static uint32_t SendQueueAddCloseOperation(SendQueue* queue, uint64_t* endStreamPos)
{
	*endStreamPos = queue->sendingBuffers.streamPos;
	if (*endStreamPos == queue->sendContext.ackStreamPos)
	{
		return BFX_RESULT_SUCCESS;
	}

	return BFX_RESULT_PENDING;
}


static uint32_t SendQueueConfirmRange(SendQueue* queue, uint64_t ackStreamPos, BdtStreamRange* sackArray, uint32_t* outAcked)
{
	*outAcked = 0;
	if ((ackStreamPos < queue->sendContext.ackStreamPos) && (!sackArray || !sackArray[0].length))
	{
		return BFX_RESULT_NOT_CHANGED;
	}
	
	BfxList flightBlocks;
	BfxListInit(&flightBlocks);

	BfxList lostBlocks;
	BfxListInit(&lostBlocks);

	BfxThreadLockLock(&queue->lockSendContext);

	if (queue->sendContext.prevDupAckStreamPos < ackStreamPos)
	{
		if (queue->sendContext.latestAckStreamPos < ackStreamPos)
		{
			queue->sendContext.latestAckStreamPos = ackStreamPos;
			queue->sendContext.dupAckTimes = 1;
		}
		else if (queue->sendContext.latestAckStreamPos == ackStreamPos)
		{
			queue->sendContext.dupAckTimes++;

		}
	}
	//多线程，需要比较
	if (queue->sendContext.ackStreamPos < ackStreamPos)
	{
		queue->sendContext.ackStreamPos = ackStreamPos;
	}

	//debug///////////////////////////////////////////////////////////////////
	/*{
		uint32_t totalflight = 0;
		SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.flightBlocks);
		do
		{
			if (!block)
			{
				break;
			}

			totalflight += (uint32_t)(block->endStreamPos - block->startStreamPos);
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.flightBlocks, (BfxListItem*)block);
		} while (true);
		assert(totalflight == queue->sendContext.flight);

		uint64_t totallost = 0;
		block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.lostBlocks);
		do
		{
			if (!block)
			{
				break;
			}

			totallost += (block->endStreamPos - block->startStreamPos);
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.lostBlocks, (BfxListItem*)block);
		} while (true);
		assert(totallost == queue->sendContext.lost);
	}*/
	//////////////////////////////////////////////////////////////////////////

	BfxListSwap(&queue->sendContext.flightBlocks, &flightBlocks);
	BfxListSwap(&queue->sendContext.lostBlocks, &lostBlocks);
	SendQueueSendBlock* flightBlock = NULL, * lostBlock = NULL;
	do
	{
		if (!flightBlock)
		{
			flightBlock = (SendQueueSendBlock*)BfxListPopFront(&flightBlocks);
		}
		if (!lostBlock)
		{
			lostBlock = (SendQueueSendBlock*)BfxListPopFront(&lostBlocks);
		}
		if (!flightBlock && !lostBlock)
		{
			break;
		}
		bool bFlight = true;
		SendQueueSendBlock* block = NULL;
		if ((flightBlock && !lostBlock) || (flightBlock && lostBlock && (flightBlock->startStreamPos < lostBlock->startStreamPos)))
		{
			block = flightBlock;
		}
		else
		{
			block = lostBlock;
			bFlight = false;
		}
		SendBlockCheckState state = SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE;
		if ((block->startStreamPos == queue->sendContext.latestAckStreamPos) && (queue->sendContext.dupAckTimes == 4))
		{
			BLOG_DEBUG("-------------dup ack %llu", queue->sendContext.latestAckStreamPos);
			queue->sendContext.prevDupAckStreamPos = queue->sendContext.latestAckStreamPos;
			queue->sendContext.latestAckStreamPos = 0;
			queue->sendContext.dupAckTimes = 0;
			state = SEND_BLOCK_CHECK_STATE_LOST;
		}
		else
		{
			state = CheckBlockStateByAck(block, queue->sendContext.ackStreamPos, sackArray);
		}
		if (state == SEND_BLOCK_CHECK_STATE_OUT_OF_RANGE)
		{
			break;
		}
		switch (state)
		{
		case SEND_BLOCK_CHECK_STATE_ACK:
		{
			*outAcked += (uint32_t)(block->endStreamPos - block->startStreamPos);
			if (bFlight)
			{
				queue->sendContext.flight -= (uint32_t)(block->endStreamPos - block->startStreamPos);
				flightBlock = NULL;
			}
			else
			{
				queue->sendContext.lost -= (block->endStreamPos - block->startStreamPos);
				lostBlock = NULL;
			}
			BFX_FREE(block);
			break;
		}
		case SEND_BLOCK_CHECK_STATE_PREV:
		{
			if (bFlight)
			{
				BfxListPushBack(&queue->sendContext.flightBlocks, (BfxListItem*)block);
				flightBlock = NULL;
			}
			else
			{
				BfxListPushBack(&queue->sendContext.lostBlocks, (BfxListItem*)block);
				lostBlock = NULL;
			}
			break;
		}
		case SEND_BLOCK_CHECK_STATE_LOST:
		{
			BfxListPushBack(&queue->sendContext.lostBlocks, (BfxListItem*)block);
			if (bFlight)
			{
				queue->sendContext.flight -= (uint32_t)(block->endStreamPos - block->startStreamPos);
				queue->sendContext.lost += (block->endStreamPos - block->startStreamPos);
				flightBlock = NULL;
			}
			else
			{
				lostBlock = NULL;
			}
			break;
		}
		default:
			assert(false);
		}
	} while (true);
	if (flightBlock)
	{
		BfxListPushBack(&queue->sendContext.flightBlocks, (BfxListItem*)flightBlock);
	}
	BfxListMerge(&queue->sendContext.flightBlocks, &flightBlocks, true);

	if (lostBlock)
	{
		BfxListPushBack(&queue->sendContext.lostBlocks, (BfxListItem*)lostBlock);
	}
	BfxListMerge(&queue->sendContext.lostBlocks, &lostBlocks, true);

	//debug///////////////////////////////////////////////////////////////////
	/*{
		uint32_t totalflight = 0;
		SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.flightBlocks);
		do
		{
			if (!block)
			{
				break;
			}

			totalflight += (uint32_t)(block->endStreamPos - block->startStreamPos);
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.flightBlocks, (BfxListItem*)block);
		} while (true);
		assert(totalflight == queue->sendContext.flight);

		uint64_t totallost = 0;
		block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.lostBlocks);
		do
		{
			if (!block)
			{
				break;
			}

			totallost += (block->endStreamPos - block->startStreamPos);
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.lostBlocks, (BfxListItem*)block);
		} while (true);
		assert(totallost == queue->sendContext.lost);
	}*/
	//////////////////////////////////////////////////////////////////////////

	BfxThreadLockUnlock(&queue->lockSendContext);

	if (*outAcked)
	{
		return BFX_RESULT_SUCCESS;
	}

	return BFX_RESULT_NOT_CHANGED;
}


static uint32_t SendQueueFillPayload(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* payloadLength)
{
	uint32_t errcode = BFX_RESULT_SUCCESS;
	BfxThreadLockLock(&queue->lockSendContext);

	SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListPopFront(&queue->sendContext.lostBlocks);
	if (!block)
	{
		block = SendQueueGenerateBlock(queue, queue->mtu);
		if (block)
		{
			*payload = block->data;
			*payloadLength = (uint16_t)(block->endStreamPos - block->startStreamPos);
			*streamPos = block->startStreamPos;

			BfxListPushBack(&queue->sendContext.flightBlocks, (BfxListItem*)block);
			queue->sendContext.flight += (uint32_t)(block->endStreamPos - block->startStreamPos);
			block->sendTime = (uint64_t)BfxTimeGetNow(false);

			queue->sendContext.streamPos += *payloadLength;
		}
		else
		{
			errcode = BFX_RESULT_NOT_CHANGED;
			*streamPos = queue->sendContext.streamPos;
			*payload = NULL;
			*payloadLength = 0;
		}
	}
	else
	{
		*payload = block->data;
		*payloadLength = (uint16_t)(block->endStreamPos - block->startStreamPos);
		*streamPos = block->startStreamPos;

		SendQueueMoveLostToFlight(queue, block);
		queue->sendContext.flight += (uint32_t)(block->endStreamPos - block->startStreamPos);
		queue->sendContext.lost -= (block->endStreamPos - block->startStreamPos);
		block->sendTime = (uint64_t)BfxTimeGetNow(false);
	}
	BfxThreadLockUnlock(&queue->lockSendContext);

	return errcode;
}

static uint32_t SendQueueFillPayloadFromOld(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength)
{
	uint32_t errcode = BFX_RESULT_SUCCESS;
	BfxThreadLockLock(&queue->lockSendContext);

	SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.lostBlocks);
	if (!block)
	{
		*inAndOutLength = 0;
		errcode = BFX_RESULT_NOT_FOUND;
	}
	else if ((uint16_t)(block->endStreamPos - block->startStreamPos) <= *inAndOutLength)
	{
		BfxListPopFront(&queue->sendContext.lostBlocks);
		*payload = block->data;
		*inAndOutLength = (uint16_t)(block->endStreamPos - block->startStreamPos);
		*streamPos = block->startStreamPos;
		SendQueueMoveLostToFlight(queue, block);
		queue->sendContext.flight += (uint32_t)(block->endStreamPos - block->startStreamPos);
		queue->sendContext.lost -= (block->endStreamPos - block->startStreamPos);
		block->sendTime = (uint64_t)BfxTimeGetNow(false);
	}
	else
	{
		*inAndOutLength = 0;
		errcode = BFX_RESULT_NOT_ENOUGH_BUFFER;
	}
	BfxThreadLockUnlock(&queue->lockSendContext);

	return errcode;
}

static uint32_t SendQueueFillPayloadFromNew(SendQueue* queue, uint64_t* streamPos, uint8_t** payload, uint16_t* inAndOutLength)
{
	uint32_t errcode = BFX_RESULT_NO_MORE_DATA;
	
	if (streamPos)
	{
		*streamPos = queue->sendContext.streamPos;
	}
	if (payload && inAndOutLength && *inAndOutLength)
	{
		BfxThreadLockLock(&queue->lockSendContext);
		SendQueueSendBlock* block = SendQueueGenerateBlock(queue, *inAndOutLength);
		if (block)
		{
			*payload = block->data;
			*inAndOutLength = (uint16_t)(block->endStreamPos - block->startStreamPos);
			*streamPos = block->startStreamPos;

			BfxListPushBack(&queue->sendContext.flightBlocks, (BfxListItem*)block);
			queue->sendContext.flight += (uint32_t)(block->endStreamPos - block->startStreamPos);
			block->sendTime = (uint64_t)BfxTimeGetNow(false);

			queue->sendContext.streamPos += *inAndOutLength;
			errcode = BFX_RESULT_SUCCESS;
		}
		else
		{
			*streamPos = queue->sendContext.streamPos;
			*payload = NULL;
			*inAndOutLength = 0;
		}
		BfxThreadLockUnlock(&queue->lockSendContext);
	}

	return errcode;
}

static uint32_t SendQueuePopCompleteBuffers(SendQueue* queue, BfxList* buffers)
{
	uint64_t ackStreamPos = queue->sendContext.ackStreamPos;
	BfxThreadLockLock(&queue->lockSendingBuffers);
	do
	{
		SendQueueSendBuffer* buffer = (SendQueueSendBuffer*)BfxListFirst(&queue->sendingBuffers.buffers);
		if (!buffer || (buffer->endStreamPos > ackStreamPos))
		{
			break;
		}

		BfxListPopFront(&queue->sendingBuffers.buffers);
		BfxListPushBack(buffers, (BfxListItem*)buffer);
		if (queue->sendingBuffers.curr == buffer)
		{
			queue->sendingBuffers.curr = (SendQueueSendBuffer*)BfxListNext(&queue->sendingBuffers.buffers, (BfxListItem*)buffer);
		}
	} while (true);
	BfxThreadLockUnlock(&queue->lockSendingBuffers);

	return BFX_RESULT_SUCCESS;
}

static uint32_t SendQueuePopAllBuffers(SendQueue* queue, BfxList* buffers)
{
	BfxThreadLockLock(&queue->lockSendingBuffers);
	BfxListSwap(buffers, &queue->sendingBuffers.buffers);
	BfxThreadLockUnlock(&queue->lockSendingBuffers);

	return BFX_RESULT_SUCCESS;
}

static MoveFlightToLost(SendQueue* queue, BfxList* blocks)
{
	SendQueueSendBlock* prev = NULL;
	SendQueueSendBlock* blockQueue = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.lostBlocks);
	do 
	{
		SendQueueSendBlock* blockInsert = (SendQueueSendBlock*)BfxListPopFront(blocks);
		if (!blockInsert)
		{
			break;
		}

		while (blockQueue)
		{
			if (blockInsert->endStreamPos <= blockQueue->startStreamPos)
			{
				break;
			}
			prev = blockQueue;
			blockQueue = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.lostBlocks, (BfxListItem*)blockQueue);
		}

		if (!prev)
		{
			BfxListPushFront(&queue->sendContext.lostBlocks, (BfxListItem*)blockInsert);
		}
		else
		{
			BfxListInsertAfter(&queue->sendContext.lostBlocks, (BfxListItem*)prev, (BfxListItem*)blockInsert);
		}
		prev = blockInsert;
	} while (true);
}

static uint32_t SendQueueMarkRangeTimeout(SendQueue* queue, bool force, uint64_t rto)
{
	uint64_t now = BfxTimeGetNow(force);
	uint32_t totalTimeout = 0;
	BfxList lostBlocks;
	BfxListInit(&lostBlocks);
	BfxThreadLockLock(&queue->lockSendContext);
	SendQueueSendBlock* block = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.flightBlocks);
	do 
	{
		if (!block)
		{
			break;
		}

		if (force || ((now - block->sendTime) > rto))
		{
			SendQueueSendBlock* lostBlock = block;
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.flightBlocks, (BfxListItem*)block);
			BfxListErase(&queue->sendContext.flightBlocks, (BfxListItem*)lostBlock);
			BfxListPushBack(&lostBlocks, (BfxListItem*)lostBlock);
			totalTimeout += (uint32_t)(lostBlock->endStreamPos - lostBlock->startStreamPos);
		}
		else
		{
			block = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.flightBlocks, (BfxListItem*)block);
		}
	} while (true);

	if (totalTimeout > 0)
	{
		MoveFlightToLost(queue, &lostBlocks);
		assert(queue->sendContext.flight >= totalTimeout);
		queue->sendContext.flight -= totalTimeout;
		queue->sendContext.lost += totalTimeout;
	}
	/*if (block)
	{
		SendQueueSendBlock* prev = NULL;
		SendQueueSendBlock* lostBlock = (SendQueueSendBlock*)BfxListFirst(&queue->sendContext.lostBlocks);
		while (lostBlock)
		{
			if (block->endStreamPos <= lostBlock->startStreamPos)
			{
				break;
			}
			prev = lostBlock;
			lostBlock = (SendQueueSendBlock*)BfxListNext(&queue->sendContext.lostBlocks, (BfxListItem*)lostBlock);
		}

		if (!prev)
		{
			BfxListPushFront(&queue->sendContext.lostBlocks, (BfxListItem*)block);
		}
		else
		{
			BfxListInsertAfter(&queue->sendContext.lostBlocks, (BfxListItem*)prev, (BfxListItem*)block);
		}
		queue->sendContext.flight -= (uint32_t)(block->endStreamPos - block->startStreamPos);
		queue->sendContext.lost += (block->endStreamPos - block->startStreamPos);
	}*/
	BfxThreadLockUnlock(&queue->lockSendContext);

	return totalTimeout;
}

static uint32_t SendQueueGetFilght(SendQueue* queue)
{
	return queue->sendContext.flight;
}

static uint64_t SendQueueGetUnsendSize(SendQueue* queue)
{
	return queue->sendingBuffers.unSendLength;
}

static uint64_t SendQueueGetLostSize(SendQueue* queue)
{
	return queue->sendContext.lost;
}