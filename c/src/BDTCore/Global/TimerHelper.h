#ifndef __BDT_TIMER_PROXY_H__
#define __BDT_TIMER_PROXY_H__
#include "../BdtSystemFramework.h"

/*
维护定时器句柄的线程安全性
*/

typedef struct Bdt_TimerHelper
{
	BdtSystemFramework* framework;

	BfxThreadLock lock; // <TODO> 自旋？保持activeTimeSeq和timerInstance的一致性
	struct
	{
		uint32_t isStartPending : 1;
		uint32_t activeTimeSeq : 31;
	};
	BDT_SYSTEM_TIMER timerInstance;
} Bdt_TimerHelper;

void Bdt_TimerHelperInit(Bdt_TimerHelper* timer,
	BdtSystemFramework* framework);
void Bdt_TimerHelperUninit(Bdt_TimerHelper* timer);

typedef void(BFX_STDCALL* LPFN_TIMER_PROCESS)(BDT_SYSTEM_TIMER timer, void* userData);

bool Bdt_TimerHelperStart(Bdt_TimerHelper* timer,
	LPFN_TIMER_PROCESS onTimerEvent,
	const BfxUserData* userData,
	int32_t timeout);
void Bdt_TimerHelperStop(Bdt_TimerHelper* timer);

bool Bdt_TimerHelperRestart(Bdt_TimerHelper* timer,
	LPFN_TIMER_PROCESS onTimerEvent,
	const BfxUserData* userData,
	int32_t timeout);

#endif // __BDT_TIMER_PROXY_H__