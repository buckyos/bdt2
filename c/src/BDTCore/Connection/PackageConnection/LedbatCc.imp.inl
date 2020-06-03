#include "./LedbatCc.inl"
#include <SGLib/SGLib.h>

#define  CONGESTION_LEDBAT_MAX	((int64_t)1 << 62)

#define  CONGESTION_LEDBET_BASE_SAMPLE_COUNT 10

#define  CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT 4

// ledbet的目标超时，ms，如果超过这个值wnd会下降
#define  CONGESTION_LEDBET_TARGET_TIME 100

#define  CONGESTION_LEDBET_GAIN 1

#define  CONGESTION_LEDBET_INIT_CWND 2

#define  CONGESTION_LEDBET_MIN_CWND 2

#define  CONGESTION_LEDBET_HASH_TABLE_DIM 20

#define  MAX_CWND_INCREASE_BYTES_PER_RTT 8000

#define MAX_INT32 (((uint32_t)1<<31)-1)

typedef struct LedbatDelayEstimateSample
{
	BfxListItem base;
	uint64_t streamPos;
	uint64_t sendTime;
	uint32_t packageId;
}LedbatDelayEstimateSample, sample_rbtree;

//static int LedbatDelayEstimateSampleCompare(sample_rbtree* left, sample_rbtree* right)
//{
//	if (left->streamPos < right->streamPos)
//	{
//		return -1;
//	}
//	if (left->streamPos > right->streamPos)
//	{
//		return 1;
//	}
//	return 0;
//}
//SGLIB_DEFINE_RBTREE_PROTOTYPES(sample_rbtree, left, right, color_field, LedbatDelayEstimateSampleCompare);
//SGLIB_DEFINE_RBTREE_FUNCTIONS(sample_rbtree, left, right, color_field, LedbatDelayEstimateSampleCompare);

struct LedbatCc
{
	BaseCc base;
	struct
	{
		uint32_t baseHeadIndex;
		int64_t baseDelaySamples[CONGESTION_LEDBET_BASE_SAMPLE_COUNT];
		int64_t minBaseDelay;
		uint32_t currHeadIndex;
		int64_t currDelaySamples[CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT];
		//上次删除baseDelaySamples头部并新插入到尾部的时间
		int64_t rollcoverTime;
		//用来评估延迟
		uint64_t maxStreamPos;
		BfxList estimate;
		volatile int32_t packageId;
		uint64_t prevEstTime;
	} sample;
	BfxThreadLock lockSample;

	uint32_t wnd;
	BfxThreadLock lockWnd;

	size_t mss;
	uint16_t mtu;

	volatile int32_t newAck;

	//收到对方检查delay的包的packageid;
	volatile int32_t remotePackageId;
};


LedbatCc* LedbatCcNew(uint16_t mtu, uint32_t mss)
{
	LedbatCc* ledbat = BFX_MALLOC_OBJ(LedbatCc);
	memset(ledbat, 0, sizeof(LedbatCc));

	//mss *= 3;
	ledbat->wnd = mss * CONGESTION_LEDBET_INIT_CWND;

	ledbat->sample.rollcoverTime = 0;
	ledbat->sample.baseHeadIndex = ledbat->sample.currHeadIndex = 0;

	for (int i = 0; i < CONGESTION_LEDBET_BASE_SAMPLE_COUNT; i++)
	{
		ledbat->sample.baseDelaySamples[i] = CONGESTION_LEDBAT_MAX;
	}

	for (int i = 0; i < CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT; i++)
	{
		ledbat->sample.currDelaySamples[i] = CONGESTION_LEDBAT_MAX;
	}

	BfxUserData userData = { 0 };
	BfxListInit(&ledbat->sample.estimate);
	BfxThreadLockInit(&ledbat->lockSample);

	BfxThreadLockInit(&ledbat->lockWnd);

	ledbat->mss = mss;
	ledbat->mtu = mtu;


	BaseCc* cc = (BaseCc*)ledbat;
	cc->uninit = LedbatCcUninit;
	cc->onChangeState = LedbatCcOnChangeState;
	cc->onAck = LedbatCcOnAck;
	cc->onSend = LedbatCcOnSend;
	cc->onData = LedbatCcOnData;
	cc->trySendPayload = LedbatCcTrySendPayload;
	cc->trySendAck = LedbatCcTrySendAck;

	return ledbat;
}


static void LedbatCcOnAck(struct BaseCc* cc, const Bdt_SessionDataPackage* package, uint32_t nAcks, uint64_t flight)
{
	LedbatCc* self = (LedbatCc*)cc;

	BfxThreadLockLock(&self->lockSample);
	UpdateSample(self, package);

	int64_t minCurrDelay = self->sample.currDelaySamples[0];
	for (int i = 1; i < CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT; i++)
	{
		if (minCurrDelay > self->sample.currDelaySamples[i])
		{
			minCurrDelay = self->sample.currDelaySamples[i];
		}
	}

	int64_t minBaseDelay = self->sample.baseDelaySamples[0];
	for (int i = 0; i < CONGESTION_LEDBET_BASE_SAMPLE_COUNT; i++)
	{
		if (minBaseDelay > self->sample.baseDelaySamples[i])
		{
			minBaseDelay = self->sample.baseDelaySamples[i];
		}
	}
	int64_t queueDelay = minCurrDelay - minBaseDelay;

	/*if (queueDelay > 50)
	{
		for (int i = 0; i < CONGESTION_LEDBET_BASE_SAMPLE_COUNT; i++)
		{
			BLOG_DEBUG("base===%lld", self->sample.baseDelaySamples[i]);
		}

		for (int i = 1; i < CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT; i++)
		{
			BLOG_DEBUG("curr===%lld", self->sample.currDelaySamples[i]);
		}

		BLOG_DEBUG("minbase=%lld,mincurr=%lld", minBaseDelay, minCurrDelay);
	}*/
	BfxThreadLockUnlock(&self->lockSample);

	if (minBaseDelay == CONGESTION_LEDBAT_MAX)
	{
		return;
	}

	BfxThreadLockLock(&self->lockWnd);
	//cwnd += GAIN * off_target * bytes_newly_acked * MSS / cwnd
	double delayFactor = (double)(CONGESTION_LEDBET_TARGET_TIME - queueDelay) / (double)CONGESTION_LEDBET_TARGET_TIME;
	double wndFactor = (double)min(nAcks, self->wnd) / (double)max(self->wnd, nAcks);
	double scaledGain = MAX_CWND_INCREASE_BYTES_PER_RTT * wndFactor * delayFactor;
	//double scaledGain = delayFactor * (double)nAcks * (double)self->mss / (double)self->wnd;

	/*if (scaledGain + self->wnd > (uint32_t)(flight + self->mss + nAcks))
	{
		self->wnd = (uint32_t)(flight + self->mss + nAcks);
	}
	else
	{
		self->wnd = (uint32_t)(scaledGain + self->wnd);
	}
	if (self->wnd < (double)(CONGESTION_LEDBET_MIN_CWND * self->mss))
	{
		self->wnd = (uint32_t)(CONGESTION_LEDBET_MIN_CWND * self->mss);
	}*/

	if (scaledGain + self->wnd < (double)(CONGESTION_LEDBET_MIN_CWND * self->mss))
	{
		self->wnd = (uint32_t)(CONGESTION_LEDBET_MIN_CWND * self->mss);
	}
	else
	{
		self->wnd = (uint32_t)(scaledGain + self->wnd);
	}

	//self->wnd+=(uint32_t)self->mss;
	BfxThreadLockUnlock(&self->lockWnd);

	//BLOG_DEBUG("-------------flight=%llu, wnd=%u,scaledGain=%f, queueDelay=%d, nAcked=%d, packageid=%d", flight,self->wnd, scaledGain, queueDelay, nAcks, (package->packageIDPart ? package->packageIDPart->packageId : 0));
}


static void LedbatCcOnChangeState(struct BaseCc* cc, CcState oldState, CcState newState)
{
	LedbatCc* self = (LedbatCc*)cc;
	switch (newState)
	{
	case CC_STATE_RTO:
	{
		BLOG_DEBUG("================= LedbatCcOnChangeState Rto");
		BfxThreadLockLock(&self->lockWnd);
		self->wnd = (uint32_t)(CONGESTION_LEDBET_MIN_CWND * self->mss);
		BfxThreadLockUnlock(&self->lockWnd);
		break;
	}
	case CC_STATE_LOST:
	{
		BLOG_DEBUG("================= LedbatCcOnChangeState lost");
		BfxThreadLockLock(&self->lockWnd);
		self->wnd /= 2;
		if (self->wnd < CONGESTION_LEDBET_MIN_CWND * self->mss)
		{
			self->wnd = (uint32_t)(CONGESTION_LEDBET_MIN_CWND * self->mss);
		}
		//self->wnd = (uint32_t)(CONGESTION_LEDBET_MIN_CWND * self->mss);
		BfxThreadLockUnlock(&self->lockWnd);
		break;
	}

	default:
		break;
	}
}

static void LedbatCcOnSend(struct BaseCc* cc, Bdt_SessionDataPackage* package)
{
	LedbatCc* self = (LedbatCc*)cc;
	if (!package->sendTime)
	{
		package->sendTime = (uint64_t)BfxTimeGetNow(false);
	}
	//带上对方的pacakgeid，以让对方可以计算延迟
	uint32_t packageId = (uint32_t)BfxAtomicExchange32(&self->remotePackageId, 0);
	if (packageId > 0)
	{
		package->packageIDPart = BFX_MALLOC_OBJ(Bdt_SessionDataPackageIdPart);
		package->packageIDPart->totalRecv = 0;
		package->packageIDPart->packageId = packageId;
	}
	if (package->payloadLength)
	{
		BfxThreadLockLock(&self->lockSample);
		LedbatDelayEstimateSample sample = { 0 };
		sample.streamPos = package->streamPos + package->payloadLength;
		if (package->streamPos + package->payloadLength > self->sample.maxStreamPos)
		{
			if ((package->sendTime - self->sample.prevEstTime) >= 30 * 1000)
			{
				self->sample.prevEstTime = package->sendTime;
				LedbatDelayEstimateSample* newSample = BFX_MALLOC_OBJ(LedbatDelayEstimateSample);
				newSample->packageId = 0;
				newSample->streamPos = package->streamPos + package->payloadLength;
				newSample->sendTime = (uint64_t)BfxTimeGetNow(false);
				self->sample.maxStreamPos = newSample->streamPos;
				BfxListPushBack(&self->sample.estimate, (BfxListItem*)newSample);
				//新的数据包，可以用来探测delay,但是优先给对方回复packageid
				if (!package->packageIDPart)
				{
					package->packageIDPart = BFX_MALLOC_OBJ(Bdt_SessionDataPackageIdPart);
					//TODO 这里把totalRecv用来做了一个标识，标识这个packageid是对数据包起作用的
					//这个在后期去锁的时候进行更改
					package->packageIDPart->totalRecv = 1;
					package->packageIDPart->packageId = (uint32_t)(++self->sample.packageId);
					newSample->packageId = package->packageIDPart->packageId;
					if (self->sample.packageId == MAX_INT32)
					{
						self->sample.packageId = 0;
					}
					BLOG_DEBUG("---------------send sample delay, packageid=%ld, time=%llu", package->packageIDPart->packageId, package->sendTime);
				}
			}
		}
		else
		{
			LedbatDelayEstimateSample* sample = (LedbatDelayEstimateSample*)BfxListFirst(&self->sample.estimate);
			while (sample)
			{
				if (sample->streamPos == package->streamPos + package->payloadLength)
				{
					BfxListErase(&self->sample.estimate, (BfxListItem*)sample);
					BFX_FREE(sample);
					break;
				}
				sample = (LedbatDelayEstimateSample*)BfxListNext(&self->sample.estimate, (BfxListItem*)sample);
			}
		}
		BfxThreadLockUnlock(&self->lockSample);
	}
}

static void LedbatCcUninit(struct BaseCc* cc)
{
	LedbatCc* self = (LedbatCc*)cc;

	do
	{
		LedbatDelayEstimateSample* sample = (LedbatDelayEstimateSample*)BfxListPopFront(&self->sample.estimate);
		if (!sample)
		{
			break;
		}
		BFX_FREE(sample);
	} while (true);


	BfxThreadLockDestroy(&self->lockSample);
	BfxThreadLockDestroy(&self->lockWnd);

	BFX_FREE(cc);
}

static uint32_t LedbatCcOnData(BaseCc* cc, const Bdt_SessionDataPackage* package, RecvPackageType type)
{
	LedbatCc* self = (LedbatCc*)cc;
	if (type != RECV_PACKAGE_TYPE_NONE && type != RECV_PACKAGE_TYPE_OUT_OF_BUFFER)
	{
		self->newAck = 1;
		if (package && package->packageIDPart && package->packageIDPart->totalRecv == 1)
		{
			BfxAtomicExchange32(&self->remotePackageId, (int32_t)package->packageIDPart->packageId);
		}
		return BFX_RESULT_SUCCESS;
	}

	return BFX_RESULT_PENDING;
}

static uint32_t LedbatCcTrySendPayload(struct BaseCc* cc, uint32_t flight, uint16_t* outPayloadLength)
{
	LedbatCc* self = (LedbatCc*)cc;
	if (self->wnd > flight)
	{
        uint32_t maxFree = self->wnd - flight;
        if (maxFree > self->mtu)
        {
            maxFree = self->mtu;
        }
        *outPayloadLength = maxFree;
		return BFX_RESULT_SUCCESS;
	}
	*outPayloadLength = 0;

	return BFX_RESULT_TOO_SMALL;
}

static uint32_t LedbatCcTrySendAck(struct BaseCc* cc)
{
	LedbatCc* self = (LedbatCc*)cc;
	if (1 == BfxAtomicCompareAndSwap32(&self->newAck, 1, 0))
	{
		return BFX_RESULT_SUCCESS;
	}

	return BFX_RESULT_NOT_CHANGED;
}

static bool CompareStreamPosWith(uint64_t streamPos, BdtStreamRange* sackArray)
{
	int i = 0;
	while (sackArray && sackArray[i].length)
	{
		if ((streamPos > sackArray[i].pos) && (streamPos <= (sackArray[i].pos + sackArray[i].length)))
		{
			return true;
		}
		i++;
	}
	return false;
}

static void UpdateBaseDelay(LedbatCc* self, int64_t delay)
{
	int64_t _now = BfxTimeGetNow(false);
	if (_now - self->sample.rollcoverTime < 60 * 1000 * 1000)
	{
		//同一分钟内
		uint32_t tailIndex = !self->sample.baseHeadIndex ? CONGESTION_LEDBET_BASE_SAMPLE_COUNT - 1 : self->sample.baseHeadIndex - 1;
		self->sample.baseDelaySamples[tailIndex] = self->sample.baseDelaySamples[tailIndex] < delay ? self->sample.baseDelaySamples[tailIndex] : delay;
	}
	else
	{
		self->sample.rollcoverTime = _now;

		self->sample.baseDelaySamples[self->sample.baseHeadIndex] = delay;
		self->sample.baseHeadIndex++;
		if (self->sample.baseHeadIndex == CONGESTION_LEDBET_BASE_SAMPLE_COUNT)
		{
			self->sample.baseHeadIndex = 0;
		}
	}
}

static void UpdateCurrDelay(LedbatCc* self, int64_t delay)
{
	self->sample.currDelaySamples[self->sample.currHeadIndex] = delay;
	self->sample.currHeadIndex++;
	if (self->sample.currHeadIndex == CONGESTION_LEDBET_CURRENT_DELAY_SAMPLE_COUNT)
	{
		self->sample.currHeadIndex = 0;
	}
}
//TODO 这里的sample处理方式应该不正确，通过ack计算delay，如果ack丢失，会导致延迟增加，使窗口下降
//后面改成了加packageid的方式，但是目前的单线程模式排队还是可能导致delay增加，后面需要改成多线程带packageid的包立马回复，其他再排队处理
static void UpdateSample(LedbatCc* self, const Bdt_SessionDataPackage* package)
{
	uint32_t packageId = 0;
	if (package->packageIDPart && package->packageIDPart->totalRecv == 0)
	{
		packageId = package->packageIDPart->packageId;
	}
	int64_t delay = CONGESTION_LEDBAT_MAX;
	do
	{
		LedbatDelayEstimateSample* sample = (LedbatDelayEstimateSample*)BfxListFirst(&self->sample.estimate);
		if (!sample)
		{
			break;
		}

		LedbatDelayEstimateSample* curr = sample; 
		sample = (LedbatDelayEstimateSample*)BfxListNext(&self->sample.estimate, (BfxListItem*)sample);

		if ((curr->streamPos <= package->ackStreamPos) || CompareStreamPosWith(curr->streamPos, package->sackArray))
		{
			if ((packageId > 0) && (packageId == curr->packageId))
			{
				BLOG_DEBUG("---------------recv sample delay, packageid=%ld, time=%llu", package->packageIDPart->packageId, package->sendTime);
				delay = (int64_t)(((int64_t)package->sendTime - (int64_t)curr->sendTime) / 1000);
			}
			
			BfxListErase(&self->sample.estimate, (BfxListItem*)curr);
			BFX_FREE(curr);
		}
		else
		{
			break;
		}
	} while (true);

	if (delay != CONGESTION_LEDBAT_MAX)
	{
		BLOG_DEBUG("======delay=%lld", delay);
		UpdateBaseDelay(self, delay);
		UpdateCurrDelay(self, delay);
	}
}

