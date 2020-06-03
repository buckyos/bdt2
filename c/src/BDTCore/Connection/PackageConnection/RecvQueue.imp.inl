#ifndef __BDT_CONNECTION_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of connection module"
#endif //__BDT_CONNECTION_MODULE_IMPL__

#include "./RecvQueue.inl"
#include <SGLib/SGLib.h>
#include "./PackageConnection.inl"

struct RecvQueue
{
	struct  
	{
		uint64_t ackStreamPos;
		//上次添加的block，一般包是连续的，出现block的情况，很大可能会加到这个block
		RecvQueueRecvBlock* latest;
		BfxList blocks;
	} recvBlock;
	BfxThreadLock lockRecvBlock;
	
	struct
	{
		//buffer数据开始的pos
		uint64_t readStreamPos;
		//buffer上数据结束的pos
		uint64_t endStreamPos;
		//上层的接收buffer
		BfxList buffers;
	} recvStream;
	BfxThreadLock lockRecvStream;
};


static inline uint64_t HelpRecvQueueMax(uint64_t left, uint64_t right)
{
	return ((left > right) ? left : right);
}
static inline uint64_t HelpRecvQueueMin(uint64_t left, uint64_t right)
{
	return ((left > right) ? right : left);
}

static RecvBlockRelativePos RecvQueueRecvBlockCalcRelaPos(RecvQueueRecvBlock* baseBlock, RecvQueueRecvBlock* other)
{
	uint64_t maxOffset = HelpRecvQueueMax(baseBlock->endStreamPos, other->endStreamPos) - HelpRecvQueueMin(baseBlock->startStreamPos, other->startStreamPos);
	uint64_t totallen = baseBlock->endStreamPos - baseBlock->startStreamPos + other->endStreamPos - other->startStreamPos;
	uint64_t maxLen = HelpRecvQueueMax(baseBlock->endStreamPos - baseBlock->startStreamPos, other->endStreamPos - other->startStreamPos);

	assert(maxOffset >= maxLen);
	if (maxOffset <= totallen)
	{
		return maxOffset == maxLen ? RECV_BLOCK_RELATIVE_POS_INCLUDE : RECV_BLOCK_RELATIVE_POS_INTERSECT;
	}
	return other->startStreamPos < baseBlock->startStreamPos ? RECV_BLOCK_RELATIVE_POS_PREV : RECV_BLOCK_RELATIVE_POS_NEXT;
}

static RecvQueueRecvBlock* RecvQueueRecvBlockNew(uint64_t startStreamPos, uint64_t endStreamPos)
{
	RecvQueueRecvBlock* block = BFX_MALLOC_OBJ(RecvQueueRecvBlock);
	block->startStreamPos = startStreamPos;
	block->endStreamPos = endStreamPos;

	return block;
}


static void RecvQueueDeleteBlock(RecvQueue* queue, RecvQueueRecvBlock* block)
{
	if (queue->recvBlock.latest == block)
	{
		queue->recvBlock.latest = NULL;
	}
	BfxListErase(&queue->recvBlock.blocks, (BfxListItem*)block);
	BFX_FREE(block);
}

static size_t RecvQueueSavePayload(RecvQueue* queue, uint64_t streamPos, const uint8_t* data, size_t len)
{
	size_t save = 0;
	BfxThreadLockLock(&queue->lockRecvStream);
	do
	{
		if (streamPos < queue->recvStream.readStreamPos)
		{
			if (queue->recvStream.readStreamPos - streamPos < len)
			{
				len -= (size_t)(queue->recvStream.readStreamPos - streamPos);
				save += (size_t)(queue->recvStream.readStreamPos - streamPos);
				streamPos = queue->recvStream.readStreamPos;
			}
			else
			{
				save = len;
				break;
			}
		}
		uint64_t bufferBeginPos = queue->recvStream.readStreamPos;
		RecvQueueRecvBuffer* buffer = (RecvQueueRecvBuffer*)BfxListFirst(&queue->recvStream.buffers);
		while (buffer)
		{
			if ((streamPos >= bufferBeginPos) && (streamPos < bufferBeginPos + buffer->len))
			{
				break;
			}
			else
			{
				bufferBeginPos += buffer->len;
			}
			buffer = (RecvQueueRecvBuffer*)BfxListNext(&queue->recvStream.buffers, (BfxListItem*)buffer);
		}
		if (!buffer)
		{
			break;
		}

		while (len && buffer)
		{
			size_t nCopy = (len > (buffer->len - (streamPos - bufferBeginPos))) ? (buffer->len - (size_t)(streamPos - bufferBeginPos)) : len;

			memcpy(buffer->data + (streamPos - bufferBeginPos), data + save, nCopy);
			save += nCopy;

			len -= nCopy;
			streamPos += nCopy;

			buffer->endStreamPos = buffer->endStreamPos < streamPos ? streamPos : buffer->endStreamPos;
			assert(buffer->endStreamPos <= bufferBeginPos + buffer->len);
			queue->recvStream.endStreamPos = queue->recvStream.endStreamPos < buffer->endStreamPos ? buffer->endStreamPos : queue->recvStream.endStreamPos;

			bufferBeginPos += buffer->len;
			buffer = (RecvQueueRecvBuffer*)BfxListNext(&queue->recvStream.buffers, (BfxListItem*)buffer);

			assert((len && (streamPos == bufferBeginPos)) || (!len && (streamPos <= bufferBeginPos)));
		}
	} while (false);
	BfxThreadLockUnlock(&queue->lockRecvStream);

	return save;
}


static RecvPackageType RecvQueueUpdateStreamPos(RecvQueue* queue, uint64_t startStreamPos, uint64_t endStreamPos)
{
	RecvPackageType recvType = RECV_PACKAGE_TYPE_IN_ORDER;
	BfxThreadLockLock(&queue->lockRecvBlock);
	do 
	{
		if (endStreamPos <= queue->recvBlock.ackStreamPos)
		{
			recvType = RECV_PACKAGE_TYPE_OLD;
			break;
		}

		if (startStreamPos <= queue->recvBlock.ackStreamPos)
		{
			queue->recvBlock.ackStreamPos = endStreamPos;
			RecvQueueRecvBlock* block = (RecvQueueRecvBlock*)BfxListFirst(&queue->recvBlock.blocks);
			if (block)
			{
				RecvQueueRecvBlock toCompare = { .startStreamPos = startStreamPos,.endStreamPos = endStreamPos };
				RecvBlockRelativePos pos = RecvQueueRecvBlockCalcRelaPos(block, &toCompare);
				if (pos == RECV_BLOCK_RELATIVE_POS_INCLUDE || pos == RECV_BLOCK_RELATIVE_POS_INTERSECT)
				{
					queue->recvBlock.ackStreamPos = HelpRecvQueueMax(queue->recvBlock.ackStreamPos, block->endStreamPos);
					RecvQueueDeleteBlock(queue, block);
				}
			}
			break;
		}

		bool bOld = RecvQueueAddToBlocks(queue, startStreamPos, endStreamPos);
		recvType = bOld ? RECV_PACKAGE_TYPE_OLD : RECV_PACKAGE_TYPE_LOST_ORDER;
	} while (false);
	BfxThreadLockUnlock(&queue->lockRecvBlock);

	return recvType;
}


static void RecvQueueBlocksMerge(RecvQueue* queue, RecvQueueRecvBlock* block)
{
	do 
	{
		RecvQueueRecvBlock* nextBlock = (RecvQueueRecvBlock*)BfxListNext(&(queue->recvBlock.blocks), (BfxListItem*)block);
		if (!nextBlock)
		{
			break;
		}
		RecvBlockRelativePos pos = RecvQueueRecvBlockCalcRelaPos(block, nextBlock);
		if (pos == RECV_BLOCK_RELATIVE_POS_INCLUDE || pos == RECV_BLOCK_RELATIVE_POS_INTERSECT)
		{
			block->startStreamPos = HelpRecvQueueMin(block->startStreamPos, nextBlock->startStreamPos);
			block->endStreamPos = HelpRecvQueueMax(block->endStreamPos, nextBlock->endStreamPos);
			RecvQueueDeleteBlock(queue, nextBlock);
		}
		else
		{
			break;
		}
	} while (true);

	do
	{
		RecvQueueRecvBlock* prevBlock = (RecvQueueRecvBlock*)BfxListPrev(&(queue->recvBlock.blocks), (BfxListItem*)block);
		if (!prevBlock)
		{
			break;
		}
		RecvBlockRelativePos pos = RecvQueueRecvBlockCalcRelaPos(block, prevBlock);
		if (pos == RECV_BLOCK_RELATIVE_POS_INCLUDE || pos == RECV_BLOCK_RELATIVE_POS_INTERSECT)
		{
			block->startStreamPos = HelpRecvQueueMin(block->startStreamPos, prevBlock->startStreamPos);
			block->endStreamPos = HelpRecvQueueMax(block->endStreamPos, prevBlock->endStreamPos);
			RecvQueueDeleteBlock(queue, prevBlock);
		}
		else
		{
			break;
		}
	} while (true);
}

static bool RecvQueueAddToBlocks(RecvQueue* queue, uint64_t startStreamPos, uint64_t endStreamPos)
{
	if (!BfxListSize(&(queue->recvBlock.blocks)))
	{
		queue->recvBlock.latest = RecvQueueRecvBlockNew(startStreamPos, endStreamPos);
		BfxListPushBack(&queue->recvBlock.blocks, (BfxListItem*)(queue->recvBlock.latest));
		return false;
	}

	RecvQueueRecvBlock toCompareBlock = { .startStreamPos = startStreamPos, .endStreamPos = endStreamPos };
	RecvQueueRecvBlock* prevBlock = NULL;
	RecvQueueRecvBlock* block = queue->recvBlock.latest;
	if (!block || endStreamPos < block->startStreamPos)
	{
		block = (RecvQueueRecvBlock*)BfxListFirst(&(queue->recvBlock.blocks));
	}

	do
	{
		RecvBlockRelativePos pos = RecvQueueRecvBlockCalcRelaPos(block, &toCompareBlock);
		switch (pos)
		{
		case RECV_BLOCK_RELATIVE_POS_INCLUDE:
		{
			if (startStreamPos >= block->startStreamPos && endStreamPos <= block->endStreamPos)
			{
				//旧的包
				return true;
			}
			block->startStreamPos = HelpRecvQueueMin(startStreamPos, block->startStreamPos);
			block->endStreamPos = HelpRecvQueueMax(endStreamPos, block->endStreamPos);

			//包含当前的包，也可能包含后面的包
			queue->recvBlock.latest = block;
			RecvQueueBlocksMerge(queue, block);
			return false;
		}
		case RECV_BLOCK_RELATIVE_POS_INTERSECT:
		{
			block->startStreamPos = HelpRecvQueueMin(startStreamPos, block->startStreamPos);
			block->endStreamPos = HelpRecvQueueMax(endStreamPos, block->endStreamPos);

			RecvQueueBlocksMerge(queue, block);
			return false;
		}
		case RECV_BLOCK_RELATIVE_POS_NEXT:
		{
			prevBlock = block;
			block = (RecvQueueRecvBlock*)BfxListNext(&queue->recvBlock.blocks, (BfxListItem*)block);
			if (!block)
			{
				queue->recvBlock.latest = RecvQueueRecvBlockNew(startStreamPos, endStreamPos);
				BfxListPushBack(&queue->recvBlock.blocks, (BfxListItem*)(queue->recvBlock.latest));
				return false;
			}
			break;
		}
		case RECV_BLOCK_RELATIVE_POS_PREV:
		{
			queue->recvBlock.latest = RecvQueueRecvBlockNew(startStreamPos, endStreamPos);
			if (!prevBlock)
			{
				BfxListPushFront(&queue->recvBlock.blocks, (BfxListItem*)(queue->recvBlock.latest));
				return false;
			}

			BfxListInsertAfter(&queue->recvBlock.blocks, (BfxListItem*)prevBlock, (BfxListItem*)(queue->recvBlock.latest));
			return false;
		}
		default:
			break;
		}

	} while (true);
}

static RecvQueue* RecvQueueCreate(uint64_t streamPos)
{
	RecvQueue* queue = BFX_MALLOC_OBJ(RecvQueue);
	memset(queue, 0, sizeof(RecvQueue));

	queue->recvBlock.latest = NULL;
	queue->recvBlock.ackStreamPos = streamPos;
	BfxListInit(&queue->recvBlock.blocks);
	BfxThreadLockInit(&queue->lockRecvBlock);

	queue->recvStream.readStreamPos = streamPos;
	queue->recvStream.endStreamPos = streamPos;
	BfxListInit(&queue->recvStream.buffers);
	BfxThreadLockInit(&queue->lockRecvStream);

	return queue;
}

static void RecvQueueDestory(RecvQueue* queue)
{
	do
	{
		RecvQueueRecvBlock* block = (RecvQueueRecvBlock*)BfxListPopFront(&queue->recvBlock.blocks);
		if (!block)
		{
			break;
		}
		BFX_FREE(block);
	} while (true);

	do
	{
		BfxListItem* block = BfxListPopFront(&queue->recvStream.buffers);
		if (!block)
		{
			break;
		}
		BFX_FREE(block);
	} while (true);

	BfxThreadLockDestroy(&queue->lockRecvBlock);
	BfxThreadLockDestroy(&queue->lockRecvStream);

	BFX_FREE(queue);
}

static uint32_t RecvQueueWriteData(RecvQueue* queue, uint64_t streamPos, const uint8_t* payload, uint16_t payloadLength, RecvPackageType* recvType)
{
	*recvType = ((streamPos + payloadLength) <= queue->recvBlock.ackStreamPos) ? RECV_PACKAGE_TYPE_OLD : RECV_PACKAGE_TYPE_IN_ORDER;
	if (*recvType != RECV_PACKAGE_TYPE_OLD && payloadLength)
	{
		//保存数据
		size_t saveLen = RecvQueueSavePayload(queue, streamPos, payload, payloadLength);
		if (saveLen)
		{
			//更新位置
			*recvType = RecvQueueUpdateStreamPos(queue, streamPos, streamPos + saveLen);
			if (saveLen < payloadLength)
			{
				*recvType = RECV_PACKAGE_TYPE_PART;
			}
		}
		else
		{
			*recvType = RECV_PACKAGE_TYPE_OUT_OF_BUFFER;
		}
	}

	return BFX_RESULT_SUCCESS;
}

static uint32_t RecvQueuePopCompleteBuffers(RecvQueue* queue, bool force, BfxList* buffers)
{
	uint64_t currAckStreamPos = queue->recvBlock.ackStreamPos;

	BfxThreadLockLock(&queue->lockRecvStream);
	do
	{
		RecvQueueRecvBuffer* buffer = (RecvQueueRecvBuffer*)BfxListFirst(&queue->recvStream.buffers);
		if (!buffer)
		{
			break;
		}

		bool bNotify = false;
		assert(buffer->endStreamPos <= queue->recvStream.endStreamPos);
		if ((buffer->endStreamPos == queue->recvStream.readStreamPos + buffer->len) && (buffer->endStreamPos <= currAckStreamPos))
		{
			//buffer接收满
			bNotify = true;
		}
		else if (force && buffer->endStreamPos && queue->recvStream.endStreamPos <= currAckStreamPos)
		{
			//当前有数据且没有间隙
			bNotify = true;
		}

		if (bNotify)
		{
			BfxListPopFront(&queue->recvStream.buffers);
			BfxListPushBack(buffers, (BfxListItem*)buffer);
			buffer->startStreamPos = queue->recvStream.readStreamPos;
			BLOG_DEBUG("============buffer->endStreamPos=%llu, queue->recvStream.ackStreamPos=%llu, queue->recvStream.readStreamPos=%llu, force=%d", buffer->endStreamPos, currAckStreamPos, queue->recvStream.readStreamPos, force ? 1 : 0);
			queue->recvStream.readStreamPos = buffer->endStreamPos;
		}
		else
		{
			break;
		}
	} while (true);
	BfxThreadLockUnlock(&queue->lockRecvStream);

	return BFX_RESULT_SUCCESS;
}

static uint32_t RecvQueuePopAllBuffers(RecvQueue* queue, BfxList* buffers)
{
	BfxThreadLockLock(&queue->lockRecvStream);
	BfxListSwap(buffers, &queue->recvStream.buffers);
	BfxThreadLockUnlock(&queue->lockRecvStream);

	return BFX_RESULT_SUCCESS;
}

static uint32_t RecvQueueGetAckInfo(RecvQueue* queue, uint64_t* ackStreamPos, BdtStreamRange** sackArray)
{
	if (ackStreamPos)
	{
		*ackStreamPos = queue->recvBlock.ackStreamPos;
	}
	if (sackArray)
	{
		*sackArray = NULL;
		BfxThreadLockLock(&queue->lockRecvBlock);
		if (BfxListSize(&queue->recvBlock.blocks))
		{
			size_t count = BfxListSize(&queue->recvBlock.blocks);
			if (count > PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT)
			{
				count = PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT;
			}
			*sackArray = BFX_MALLOC_ARRAY(BdtStreamRange, (count + 1));
			memset(*sackArray, 0, sizeof(BdtStreamRange) * (count + 1));

			int index = 0;
			bool bAddLatestBlock = !(queue->recvBlock.latest);
			RecvQueueRecvBlock* block = (RecvQueueRecvBlock*)BfxListFirst(&(queue->recvBlock.blocks));
			while (index < count && block)
			{
				(*sackArray + index)->pos = block->startStreamPos;
				(*sackArray + index)->length = (uint32_t)(block->endStreamPos - block->startStreamPos);

				index++;
				bAddLatestBlock = bAddLatestBlock || (queue->recvBlock.latest == block);
				if (index == (count - 1) && !bAddLatestBlock)
				{
					break;
				}

				block = (RecvQueueRecvBlock*)BfxListNext(&(queue->recvBlock.blocks), (const BfxListItem*)block);
			}
			if (!bAddLatestBlock)
			{
				(*sackArray + index)->pos = queue->recvBlock.latest->startStreamPos;
				(*sackArray + index)->length = (uint32_t)(queue->recvBlock.latest->endStreamPos - queue->recvBlock.latest->startStreamPos);
			}
		}
		*ackStreamPos = queue->recvBlock.ackStreamPos;
		BfxThreadLockUnlock(&queue->lockRecvBlock);
	}
	
	return BFX_RESULT_SUCCESS;
}

static uint32_t RecvQueueAddBuffer(RecvQueue* queue, uint8_t* data, size_t len, BdtConnectionRecvCallback callback, const BfxUserData* userData)
{
	if (!data || !len || !callback)
	{
		return BFX_RESULT_INVALID_PARAM;
	}

	uint32_t errcode = BFX_RESULT_SUCCESS;
	BfxThreadLockLock(&queue->lockRecvStream);
	RecvQueueRecvBuffer* buffer = (RecvQueueRecvBuffer*)BfxListFirst(&queue->recvStream.buffers);
	do
	{
		if (!buffer || buffer->data == data)
		{
			break;
		}
		buffer = (RecvQueueRecvBuffer*)BfxListNext(&queue->recvStream.buffers, (BfxListItem*)buffer);
	} while (true);

	if (!buffer)
	{
		buffer = BFX_MALLOC_OBJ(RecvQueueRecvBuffer);
		memset(buffer, 0, sizeof(RecvQueueRecvBuffer));
		buffer->data = data;
		buffer->len = len;
		buffer->callback = callback;
		buffer->userData = *userData;
		BfxListPushBack(&queue->recvStream.buffers, (BfxListItem*)buffer);

        if (userData && userData->lpfnAddRefUserData != NULL)
        {
            userData->lpfnAddRefUserData(userData->userData);
        }
	}
	else
	{
		errcode = BFX_RESULT_ALREADY_EXISTS;
	}
	BfxThreadLockUnlock(&queue->lockRecvStream);

	return errcode;
}
