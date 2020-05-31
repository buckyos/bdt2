#include "./SuperNodeClient.imp.inl"
#include "./Test.h"
#include "../TestCase.h"

void BdtSuperNodeClient_UnitTest()
{
	Bdt_AddTestCase("SuperNode.Client", SuperNodeClientUnitTest);
}