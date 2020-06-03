#include "../TestCase.h"
#include <Global/CryptoToolKit.h>

static int CryptoAesProfiler(Bdt_TestCase* testCase)
{
	uint8_t aesKey[BDT_AES_KEY_LENGTH] = 
		{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };

	{
		uint32_t len = 1024;
		uint8_t* data = BfxMalloc(len + 16);

		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			Bdt_AesEncryptDataWithPadding(aesKey, data, len, len + 16, data, len + 16);
		}
		uint64_t e = BfxTimeGetNow(false);

		BfxFree(data);
		
		printf("aes inplace encrypt %u %i times use: %u ms\r\n", len, count, (uint32_t)((e - s) / 1000));
	}


	{
		uint32_t len = 1024;
		uint8_t* data = BfxMalloc(len + 16);
		uint8_t* out = BfxMalloc(len + 16);

		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			Bdt_AesEncryptDataWithPadding(aesKey, data, len, len + 16, out, len + 16);
		}
		uint64_t e = BfxTimeGetNow(false);

		BfxFree(data);
		BfxFree(out);

		printf("aes different place encrypt %u %i times use: %u ms\r\n", len, count, (uint32_t)((e - s) / 1000));
	}

	{
		uint32_t len = 1024;
		uint8_t* data = BfxMalloc(len + 16);
		uint8_t* out = BfxMalloc(len + 16);

		Bdt_AesEncryptDataWithPadding(aesKey, data, len, len + 16, out, len + 16);

		int count = 100 * 1000;
		uint64_t s = BfxTimeGetNow(false);
		for (int ix = 0; ix < count; ++ix)
		{
			Bdt_AesDecryptDataWithPadding(aesKey, out, len + 16, data, len + 16);
		}
		uint64_t e = BfxTimeGetNow(false);

		BfxFree(data);
		BfxFree(out);

		printf("aes different place decrypt %u %i times use: %u ms\r\n", len, count, (uint32_t)((e - s) / 1000));
	}
	
	return 0;
}