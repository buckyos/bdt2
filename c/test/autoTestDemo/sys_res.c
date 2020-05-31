#include "sys_res.h"

#if defined(BFX_OS_WIN)
#include "./sys_res_win.inl"
#else
#include "./sys_res_linux.inl"
#endif
