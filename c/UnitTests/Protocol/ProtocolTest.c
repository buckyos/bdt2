#include <Protocol/ProtocolModule.c>
#include "./Package.imp.inl"
#include "./Test.h"
#include "../TestCase.h"

void BdtProtocol_UnitTest()
{
	Bdt_AddTestCase("protocol.package.encode", TestPackageEncodeDecode);
}