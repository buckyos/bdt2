typedef uv_key_t BfxTlsKey;

BFX_API(int)  BfxTlsCreate(BfxTlsKey *key);
BFX_API(void) BfxTlsDelete(BfxTlsKey *key);

BFX_API(void) BfxTlsSetData(BfxTlsKey *key, void *data);
BFX_API(void*) BfxTlsGetData(BfxTlsKey *key);