#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./TimerHelper.h"

void Bdt_TimerHelperInit(Bdt_TimerHelper* timer,
	BdtSystemFramework* framework)
{
	BLOG_CHECK(timer != NULL);
	BLOG_CHECK(framework != NULL);

	timer->framework = framework;
	timer->isStartPending = 0;
	timer->activeTimeSeq = 1;
	timer->timerInstance = NULL;
	BfxThreadLockInit(&timer->lock);
}

void Bdt_TimerHelperUninit(Bdt_TimerHelper* timer)
{
	Bdt_TimerHelperStop(timer);
	BfxThreadLockDestroy(&timer->lock);
}

bool Bdt_TimerHelperStart(Bdt_TimerHelper* timer,
	LPFN_TIMER_PROCESS onTimerEvent,
	const BfxUserData* userData,
	int32_t timeout)
{
	BLOG_CHECK(timer != NULL);
	BLOG_CHECK(onTimerEvent != NULL);
	BLOG_CHECK(timeout >= 0);

	
	uint32_t curActiveSeq = 0;

	{
		BfxThreadLockLock(&timer->lock);
		if (timer->timerInstance == NULL && !timer->isStartPending)
		{
			timer->isStartPending = 1;
			curActiveSeq = timer->activeTimeSeq;
		}
		BfxThreadLockUnlock(&timer->lock);
	}

	if (curActiveSeq == 0)
	{
		BLOG_CHECK(false);
		return false;
	}

	BDT_SYSTEM_TIMER newTimer = BdtCreateTimeout(timer->framework,
		onTimerEvent,
		userData,
		timeout);

	bool timerHasDestroy = false;

	{
		BfxThreadLockLock(&timer->lock);
		if (timer->activeTimeSeq == curActiveSeq)
		{
			timer->timerInstance = newTimer;
			timer->isStartPending = 0;
		}
		else
		{
			timerHasDestroy = true;
		}
		BfxThreadLockUnlock(&timer->lock);
	}

	if (timerHasDestroy)
	{
		BdtDestroyTimeout(timer->framework, newTimer);
	}
	return newTimer != NULL;
}

void Bdt_TimerHelperStop(Bdt_TimerHelper* timer)
{
	BLOG_CHECK(timer != NULL);
	
	BDT_SYSTEM_TIMER oldTimer = NULL;

	{
		BfxThreadLockLock(&timer->lock);
		timer->activeTimeSeq++;
		oldTimer = timer->timerInstance;
		timer->timerInstance = NULL;
		timer->isStartPending = 0;
		BfxThreadLockUnlock(&timer->lock);
	}

	if (oldTimer != NULL)
	{
		BdtDestroyTimeout(timer->framework, oldTimer);
	}
}

bool Bdt_TimerHelperRestart(Bdt_TimerHelper* timer,
	LPFN_TIMER_PROCESS onTimerEvent,
	const BfxUserData* userData,
	int32_t timeout)
{
	Bdt_TimerHelperStop(timer);

	return Bdt_TimerHelperStart(timer, onTimerEvent, userData, timeout);
}