#include "../TestCase.h"
#include <BdtCore.h>

static int EndpointTest(Bdt_TestCase* testCase)
{
	{
		const char* sz = "v6unk[fe80::abc:efff:1234:5678]:0";
		BdtEndpoint ep;
		BdtEndpointFromString(&ep, sz);
		char f[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
		BdtEndpointToString(&ep, f);
		assert(!strcmp(sz, f));
	}

	{
		const char* sz = "v6unk[::1]:0";
		BdtEndpoint ep;
		BdtEndpointFromString(&ep, sz);
		char f[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = { 0, };
		BdtEndpointToString(&ep, f);
		assert(!strcmp(sz, f));
	}

	return 0;
}