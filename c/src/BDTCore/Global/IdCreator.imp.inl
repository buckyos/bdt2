#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./IdCreator.h"


void Bdt_Uint32IdCreatorInit(Bdt_Uint32IdCreator* creator)
{
	int32_t init = BfxRandomRange32(BDT_UINT32_ID_INVALID_VALUE + 1, 0x7fffffff);
	creator->curValue = init;
}

void Bdt_Uint32IdCreatorUninit(Bdt_Uint32IdCreator* creator)
{

}

uint32_t Bdt_Uint32IdCreatorValue(Bdt_Uint32IdCreator* creator)
{
	return creator->curValue;
}

uint32_t Bdt_Uint32IdCreatorNext(Bdt_Uint32IdCreator* creator)
{
	int32_t nextValue = 0;
	do
	{
		int32_t curValue = 0;
		BfxAtomicExchange32(&curValue, creator->curValue);
		if (curValue == 0x7fffffff)
		{
			nextValue = BDT_UINT32_ID_INVALID_VALUE + 1;
		}
		else
		{
			nextValue = curValue + 1;
		}
		if (curValue == BfxAtomicCompareAndSwap32(&creator->curValue, curValue, nextValue))
		{
			break;
		}
	} while (true);
	return nextValue;
}
