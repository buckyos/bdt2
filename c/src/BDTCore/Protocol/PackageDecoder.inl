#ifndef _BDT_PACKAGE_DECODER_INL_
#define _BDT_PACKAGE_DECODER_INL_

#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

#include "./PackageDecoder.h"

static Bdt_Package* DecodeSinglePackage(
	uint8_t cmdtype,
	uint16_t totallen,
	BfxBufferStream* bufferStream,
	Bdt_Package* pRefPackage);

static int DecodeExchangeKeyPackage(Bdt_ExchangeKeyPackage* pPackage, BfxBufferStream* bufferStream,const Bdt_Package* pRefPackage);
static int DecodeDataGramPackage(Bdt_DataGramPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage);
static int DecodeSessionDataPackage(Bdt_SessionDataPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage);

#endif //_BDT_PACKAGE_DECODER_INL_