#ifndef __BDT_PACKAGE_ENCODER_H__
#define __BDT_PACKAGE_ENCODER_H__
#include "./Package.h"

int Bdt_EncodeSinglePackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes);
int Bdt_EncodePackage(
	const Bdt_Package** packages, 
	size_t packageCount, 
	const Bdt_Package* refPackage,
	uint8_t* buffer, 
	size_t bufferLength, 
	size_t* pWriteBytes);

#endif //__BDT_PACKAGE_ENCODER_H__