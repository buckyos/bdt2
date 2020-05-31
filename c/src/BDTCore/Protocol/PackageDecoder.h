#ifndef _BDT_PACKAGE_DECODER_H_
#define _BDT_PACKAGE_DECODER_H_

#include "./Package.h"


#define TCP_FIRSTPACKAGE_PARSER_CACHE_SIZE 4096

//用来解码BOX Package中的data
int Bdt_DecodePackage(
	const uint8_t* buffer, 
	size_t length, 
	const Bdt_Package* refPackage, 
	Bdt_Package** outResult, 
	bool isStartWithExchangeKey);


#endif //_BDT_PACKAGE_PARSER_
