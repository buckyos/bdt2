#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./StackTls.h"

uint32_t Bdt_StackTlsInit(Bdt_StackTls* tls)
{
	BfxTlsCreate(&tls->slot);
	return BFX_RESULT_SUCCESS;
}

void Bdt_StackTlsUninit(Bdt_StackTls* tls)
{
	BfxTlsDelete(&tls->slot);
}

Bdt_StackTlsData* Bdt_StackTlsGetData(Bdt_StackTls* tls)
{
	Bdt_StackTlsData* data = (Bdt_StackTlsData*)BfxTlsGetData(&tls->slot);
	if (!data)
	{
		// tofix: when to free
		data = BFX_MALLOC_OBJ(Bdt_StackTlsData);
		BfxTlsSetData(&tls->slot, data);
	}
	return data;
}