#ifndef __BUCKY_BASE_TIME_H__
#define __BUCKY_BASE_TIME_H__

typedef struct
{
	int32_t	year;		// 年份，eg: 2019
	int32_t month;		// 月份，1-12
	int32_t dayOfWeak;	// 周中第几天，0=星期天，1=星期一，取值范围0-6
	int32_t dayOfMonth;	// 月中第几天，从1开始，取值范围1-31
	int32_t hour;		// 小时，从0开始，取值范围0-23
	int32_t minute;		// 分钟，从0开始，取值范围0-59
	int32_t second;		// 秒，从0开始，取值范围0-59(但由于存在闰秒，所以有可能会加1正闰秒产生60的值)
	int32_t millisecond;	// 毫秒，从0开始，取值范围0-999
} BfxSystemTime;


// 获取当前系统时间
// local=true:本地时间  local=false:UTC时间
BFX_API(int64_t) BfxTimeGetNow(bool asLocal);

// UTC时间<->本地时间
BFX_API(int64_t) BfxUTCTimeToLocalTime(int64_t tm);
BFX_API(int64_t) BfxLocalTimeToUTCTime(int64_t tm);

// SYSTEMTIME<->时间
BFX_API(int64_t) BfxSystemTimeToTime(const BfxSystemTime* lpSystemTime, bool asLocal);
BFX_API(void) BfxTimeToSystemTime(int64_t time, BfxSystemTime* lpSystemTime, bool asLocal);

#endif