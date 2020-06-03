#include "../TestCase.h"
#include <BdtCore.h>

static int MemcpyProfiler(Bdt_TestCase* testCase)
{
	{
		uint32_t len = 1024;
		uint8_t* data = BfxMalloc(len);

		uint8_t* out = BfxMalloc(len);

		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			memcpy(out, data, len);
		}

		uint64_t e = BfxTimeGetNow(false);

		BfxFree(data);

		printf("memcpy %u %i times use: %u ms\r\n", len, count, (uint32_t)((e - s) / 1000));
	}

	return 0;
}


static int LockProfiler(Bdt_TestCase* testCase)
{
	{
		BfxThreadLock lock;
		BfxThreadLockInit(&lock);


		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			BfxThreadLockLock(&lock);
			BfxThreadLockUnlock(&lock);
		}
		uint64_t e = BfxTimeGetNow(false);

		printf("thread lock %i times use: %u ms\r\n", count, (uint32_t)((e - s) / 1000));
	}

	{
		BfxRwLock lock;
		BfxRwLockInit(&lock);


		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			BfxRwLockRLock(&lock);
			BfxRwLockRUnlock(&lock);
		}
		uint64_t e = BfxTimeGetNow(false);
		
		printf("rwlock read lock %i times use: %u ms\r\n", count, (uint32_t)((e - s) / 1000));
	}


	{
		BfxRwLock lock;
		BfxRwLockInit(&lock);


		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			BfxRwLockWLock(&lock);
			BfxRwLockWUnlock(&lock);
		}
		uint64_t e = BfxTimeGetNow(false);

		printf("rwlock write lock %i times use: %u ms\r\n", count, (uint32_t)((e - s) / 1000));
	}

	return 0;
}