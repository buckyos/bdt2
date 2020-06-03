#ifndef __BDT_UNIQUE_CREATOR_H__
#define __BDT_UNIQUE_CREATOR_H__
#include "../BdtCore.h"

#define BDT_UINT32_ID_INVALID_VALUE			0

// BdtUint32UniqueCreator is thread safe!!!
typedef struct Bdt_Uint32IdCreator
{
	int32_t curValue;
} Bdt_Uint32IdCreator;

void Bdt_Uint32IdCreatorInit(Bdt_Uint32IdCreator* creator);
void Bdt_Uint32IdCreatorUninit(Bdt_Uint32IdCreator* creator);
uint32_t Bdt_Uint32IdCreatorValue(Bdt_Uint32IdCreator* creator);
uint32_t Bdt_Uint32IdCreatorNext(Bdt_Uint32IdCreator* creator);


#endif //__BDT_UNIQUE_CREATOR_H__