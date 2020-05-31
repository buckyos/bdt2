#include "../BuckyBase.h"


BFX_API(int)  BfxTlsCreate(BfxTlsKey* key) {
	assert(key);

	int ret = uv_key_create(key);
	if (ret != 0) {
		BLOG_ERROR("allow tls key failed, err=%d", ret);
		return BFX_RESULT_FAILED;
	}

	return 0;
}

BFX_API(void) BfxTlsDelete(BfxTlsKey* key) {
	assert(key);

	uv_key_delete(key);
}

BFX_API(void) BfxTlsSetData(BfxTlsKey* key, void* data) {
	assert(key);

	uv_key_set(key, data);
}

BFX_API(void*) BfxTlsGetData(BfxTlsKey* key) {
	assert(key);

	return uv_key_get(key);
}