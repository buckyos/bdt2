#include <napi.h>
#include "./Global.h"
#include "./Stack.h"
#include "./PeerInfo.h"
#include "./Connection.h"
#include "./NetModifier.h"

Napi::Object InitBdt2 (Napi::Env env, Napi::Object exports) 
{
    BdtResult::Register(env, exports);
    PeerConstInfo::Register(env, exports);
    Stack::Register(env, exports);
    NetModifier::Register(env, exports);
	Connection::Register(env, exports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitBdt2)