#include "../BuckyBase/BuckyBase.h"
#include "./sys_adapter.h"

#ifdef BFX_FRAMEWORK_NATIVE
#include "../native/sys_adapter.inl"
#elif defined(BFX_FRAMEWORK_UV)
#include "../uv/sys_adapter.inl"
#endif
