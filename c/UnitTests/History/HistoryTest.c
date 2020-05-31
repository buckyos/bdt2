#include "./PeerHistory.imp.inl"
#include "./Test.h"
#include "../TestCase.h"

void BdtHistory_UnitTest()
{
	Bdt_AddTestCase("history", HistoryTest);
}