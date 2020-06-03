#include <Global/GlobalModule.c>
#include "./Test.h"
#include "../TestCase.h"
#include "./Base.imp.inl"
#include "./CryptoToolkit.imp.inl"

void BdtGlobal_Profiler()
{
	Bdt_AddTestCase("global.lock", LockProfiler);
	Bdt_AddTestCase("global.memcpy", MemcpyProfiler);
	Bdt_AddTestCase("global.crypto.aes", CryptoAesProfiler);
}