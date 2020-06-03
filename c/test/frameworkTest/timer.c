#include <BDT2.h>
#include <LibuvTimer.h>
#include <assert.h>
#include <uv.h>
#include <stdio.h>

uv_sem_t semCanFinish;

extern void BFX_STDCALL BdtLibuvFrameworkDestroySelf(struct BdtSystemFramework* framework);

void BFX_STDCALL StackEventCallbackTimer(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData)
{

}

void BFX_STDCALL BdtLibuvTimerCB(BDT_SYSTEM_TIMER timer, void* userData)
{
	printf("=========ontimer \n");
	uv_sem_post(&semCanFinish);
}

int main(int argc, char** argv)
{
	//uv_sem_init(&semCanFinish, 0);
	//BdtStack* handle = NULL;

	//uint32_t errcode = BdtLibuvCreateStack(NULL, NULL, 0, StackEventCallbackTimer, NULL, &handle);
	//if (errcode)
	//{
	//	assert(false);
	//	return 0;
	//}
	//printf("timer create stack succ \n");

	//uint8_t* pHandle = (uint8_t*)handle;
	////struct SystemFramework* framework = *((struct SystemFramework**)(pHandle + 4));
	//struct BdtSystemFramework* framework = (struct BdtSystemFramework*)(handle);
	//BDT_SYSTEM_TIMER testTimer = BdtLibuvCreateTimer(framework, BdtLibuvTimerCB, NULL, 5000, false, 0);
	//uv_sem_wait(&semCanFinish);
	//BdtLibuvDestroyTimer(testTimer);
	//BdtLibuvFrameworkDestroySelf(framework);
	//printf("stack finish \n");

	//BdtLibuvPrintObjectMem();
	return 0;
}