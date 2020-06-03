#include <Global/GlobalModule.c>
#include "./Endpoint.imp.inl"
#include "./Test.h"
#include "../TestCase.h"

void BdtGlobal_UnitTest()
{
	Bdt_AddTestCase("global.endpoint", EndpointTest);
}