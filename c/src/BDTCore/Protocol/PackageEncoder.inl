#ifndef _BDT_PACKAGE_ENCODER_INL_
#define _BDT_PACKAGE_ENCODER_INL_

#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

#include "./PackageEncoder.h"

static int EncodeDataGramPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes);
static int EncodeExchangeKeyPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes);
static int EncodeSessionDataPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes);

#endif //_BDT_PACKAGE_ENCODER_INL_