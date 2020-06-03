#pragma once

static int64_t _ftToMicroSeconds(const FILETIME *ft) {
	return (*(int64_t*)ft) / 10;
}

static void _microSecondsToFT(int64_t ms, FILETIME *ft) {
	assert(ms >= 0LL);

	*(int64_t*)ft = (ms * 10);
}


static int64_t _getTime(bool local) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	if (local) {
		FILETIME local;
		FileTimeToLocalFileTime(&ft, &local);
		ft = local;
	}

	return _ftToMicroSeconds(&ft);
}

static int64_t _convert(int64_t time, bool toLocal) {
	FILETIME ft, dest;
	_microSecondsToFT(time, &ft);

	if (toLocal) {
		FileTimeToLocalFileTime(&ft, &dest);
	} else {
		LocalFileTimeToFileTime(&ft, &dest);
	}

	return _ftToMicroSeconds(&dest);
}

static int64_t _fromSystemTime(const BfxSystemTime *st, bool asLocal) {

	// 1 切换到win32下的系统时间格式，
	// 2 统一转换到UTC时间，
	// 3 切换到标准时间类型
	SYSTEMTIME wst;
	wst.wYear = (WORD)(st->year);
	wst.wMonth = (WORD)(st->month);
	wst.wDay = (WORD)(st->dayOfMonth);
	wst.wDayOfWeek = (WORD)(st->dayOfWeak);
	wst.wHour = (WORD)(st->hour);
	wst.wMinute = (WORD)(st->minute);
	wst.wSecond = (WORD)(st->second);
	wst.wMilliseconds = (WORD)(st->millisecond);

	FILETIME ft;
	BOOL ret;
	if (asLocal) {
		SYSTEMTIME utcTime;
		ret = TzSpecificLocalTimeToSystemTime(NULL, &wst, &utcTime);
		if (ret) {
			ret = SystemTimeToFileTime(&utcTime, &ft);
		}
	} else {
		ret = SystemTimeToFileTime(&wst, &ft);
	}

	int64_t ms = 0;
	if (ret) {
		ms = _ftToMicroSeconds(&ft);
	} else {
		BLOG_ERROR("convert from system time got error, error=%d", GetLastError());
	}

	return ms;
}

static void _toSystemTime(int64_t ms, BfxSystemTime *st, bool asLocal) {
	if (ms < INT64_C(0)) {
		memset(&st, 0, sizeof(st));
		return;
	}

	// 1. 先转换到UTC的文件时间，
	// 2. 转换到对应的系统时间
	// 3. 生成对应的SystemTime格式
	FILETIME ftUTC;
	_microSecondsToFT(ms, &ftUTC);

	SYSTEMTIME wst;
	BOOL ret = FALSE;
	if (asLocal) {
		SYSTEMTIME stUTC;
		ret = FileTimeToSystemTime(&ftUTC, &stUTC);
		if (ret) {
			SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &wst);
		}
	} else {
		ret = FileTimeToSystemTime(&ftUTC, &wst);
	}

	if (ret) {
		st->year = wst.wYear;
		st->month = wst.wMonth;
		st->dayOfMonth = wst.wDay;
		st->dayOfWeak = wst.wDayOfWeek;
		st->hour = wst.wHour;
		st->minute = wst.wMinute;
		st->second = wst.wSecond;
		st->millisecond = wst.wMilliseconds;
	} else {
		BLOG_ERROR("convert to system time got error, err=%d", GetLastError());
		memset(&st, 0, sizeof(st));
	}
}
