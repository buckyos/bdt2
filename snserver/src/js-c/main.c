#include <node_api.h>

extern void protocol_export(napi_env env, napi_value exports);

napi_value regist_modules(napi_env env, napi_value exports)
{
	napi_value newModuleExports = NULL;

	napi_create_object(env, &newModuleExports);
	protocol_export(env, newModuleExports);
	napi_set_named_property(env, exports, "protocol", newModuleExports);

	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, regist_modules)
