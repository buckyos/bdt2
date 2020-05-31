#ifndef _BDT_PACKAGE_INL_
#define _BDT_PACKAGE_INL_
#ifndef __BDT_PROTOCOL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of protocol module"
#endif //__BDT_PROTOCOL_MODULE_IMPL__

#include "./Package.h"

static int ReadPackageFieldValueFromReference(uint8_t cmdtype, uint16_t fieldFlags, const Bdt_Package* refPackage, void* pFieldValue);
static const char* GetPackageFieldName(uint8_t cmdtype, uint16_t fieldFlags);
static const void* GetPackageFieldPointByName(const Bdt_Package* package, const char* fieldName);
static int GetPackageFieldValueByName(const Bdt_Package* pPackage, const char* fieldName, void* pFieldValue);
//bool IsEqualPackageFieldValueWithReference(BdtPackage* package, uint16_t fieldFlags, BdtPackage* refPackage);
static bool IsEqualPackageFieldDefaultValue(const Bdt_Package* package, uint16_t fieldFlags);



#endif //_BDT_PACKAGE_INL_