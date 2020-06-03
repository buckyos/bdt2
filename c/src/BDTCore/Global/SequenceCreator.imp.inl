#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include <BuckyBase/BuckyBase.h>
#include "./SequenceCreator.h"

#define TIME_NOW (uint32_t)((BfxTimeGetNow(false) >> 20) & ((1 << SEQUENCE_TIME_BITS) - 1))

void Bdt_SequenceInit(Bdt_SequenceCreator* creator)
{
	creator->lastValue.bits.time = TIME_NOW;
	creator->lastValue.bits.increase = (uint32_t)(BfxRandom64() & ((1 << SEQUENCE_INCREASE_BITS) - 1));
}

uint32_t Bdt_SequenceValue(Bdt_SequenceCreator* creator)
{
	return creator->lastValue.u32;
}

void Bdt_SequenceUninit(Bdt_SequenceCreator* creator)
{

}

uint32_t Bdt_SequenceNext(Bdt_SequenceCreator* creator)
{
	Bdt_Sequence lastValue = creator->lastValue;
	Bdt_Sequence newValue;
	newValue.bits.time = TIME_NOW;
	newValue.bits.increase = lastValue.bits.increase + 1;

	if (lastValue.i32 != BfxAtomicCompareAndSwap32(&creator->lastValue.i32, lastValue.i32, newValue.i32))
	{
		return Bdt_SequenceNext(creator);
	}
	return newValue.u32;
}

// 假定同时不存在两个相差过大的活跃sequence；
// 相差过大多半是因为时间段刚好溢出导致sequence绝对值变小
int Bdt_SequenceCompare(uint32_t seq1, uint32_t seq2)
{
	if (seq1 == seq2)
	{
		return 0;
	}

	if (seq1 > seq2)
	{
		if (seq1 - seq2 < 0x80000000)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if (seq2 - seq1 < 0x80000000)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
}

bool Bdt_SequenceIsTimeout(uint32_t seq, uint32_t timeLimitS)
{
    Bdt_Sequence seqObj = {
        .u32 = seq
    };

    uint32_t now = TIME_NOW;
    if (now < seqObj.bits.time)
    {
        now += (1 << SEQUENCE_TIME_BITS);
    }
    return now - seqObj.bits.time > timeLimitS;
}