
#include "../BuckyBase.h"
#include <assert.h>

#if defined(BFX_OS_WIN)
#include "./win.inl"
#else
#include "./posix.inl"
#endif


BFX_API(int64_t) BfxTimeGetNow(bool asLocal)
{
	return _getTime(asLocal);
}

BFX_API(int64_t) BfxUTCTimeToLocalTime(int64_t tm)
{
	return _convert(tm, true);
}

BFX_API(int64_t) BfxLocalTimeToUTCTime(int64_t tm) 
{
	return _convert(tm, false);
}

BFX_API(int64_t) BfxSystemTimeToTime(const BfxSystemTime* systemTime, bool asLocal) {
	return _fromSystemTime(systemTime, asLocal);
}

BFX_API(void) BfxTimeToSystemTime(int64_t time, BfxSystemTime* systemTime, bool asLocal) {
	_toSystemTime(time, systemTime, asLocal);
}

