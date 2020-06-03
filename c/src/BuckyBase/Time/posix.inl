#include "../BuckyBase.h"

// time_t(since midnight, January 1, 1970)到统一计时纪元(midnight, January 1, 1601)的时间差
static const int64_t _timeTToMicrosecondsOffset = INT64_C(11644473600) * 1000 * 1000;

#if defined(BFX_OS_ANDROID) && defined(BFX_ARCH_CPU_32_BITS)
typedef time64_t SysTime;

SysTime _sysTimeFromTimeStruct(struct tm* timestruct, bool asLocal)
{
    if (asLocal)
    {
        return mktime64(timestruct);
    }
    else
    {
        return timegm64(timestruct);
    }
}

void _sysTimeToTimeStruct(SysTime t, struct tm* timestruct, bool asLocal)
{
    if (asLocal)
    {
        localtime64_r(&t, timestruct);
    }
    else
    {
        gmtime64_r(&t, timestruct);
    }
}

#else  // !BFX_OS_ANDROID
typedef time_t SysTime;

SysTime _sysTimeFromTimeStruct(struct tm* timestruct, bool asLocal)
{
    if (asLocal)
    {
        return mktime(timestruct);
    }
    else
    {
        return timegm(timestruct);
    }
}

void _sysTimeToTimeStruct(SysTime t, struct tm* timestruct, bool asLocal)
{
    if (asLocal)
    {
        localtime_r(&t, timestruct);
    }
    else
    {
        gmtime_r(&t, timestruct);
    }
}
#endif  // !BFX_OS_ANDROID

static int64_t _timeMax() {
    if (sizeof(time_t) == sizeof(int64_t)) {
        return INT64_MAX;
    }
    else {
        BLOG_CHECK(sizeof(time_t) == sizeof(int32_t));
        return INT32_MAX;
    }
}

static int64_t _timeMin() {
    if (sizeof(time_t) == sizeof(int64_t)) {
        return INT64_MIN;
    }
    else {
        BLOG_CHECK(sizeof(time_t) == sizeof(int32_t));
        return INT32_MIN;
    }
}

static int64_t _getTime(bool asLocal) {
    int64_t ms = 0;

    struct timeval tv;
    struct timezone tz = { 0, 0 };  // UTC
    if (gettimeofday(&tv, &tz) != 0)
    {
        BLOG_ERROR("call gettimeofday failed! err=%d", errno);
        ms = 0;
    }
    else
    {
        int64_t seconds = tv.tv_sec;
        if (asLocal)
        {
            seconds -= (tz.tz_minuteswest * 60);
        }

        // gettimeofday返回的是基于Unix Epoch
        ms = seconds * 1000 * 1000
            + tv.tv_usec
            + _timeTToMicrosecondsOffset;
    }

    return ms;
}

static int64_t _convert(int64_t tm, bool toLocal)
{
    // 这里编译可能会有警告，官方文档如下：
    // If either tv or tz is NULL, the corresponding structure is not set or returned.
    // (However, compilation warnings will result if tv is NULL.)
    struct timezone tz = { 0, 0 };  // UTC
    if (gettimeofday(NULL, &tz) != 0)
    {
        BLOG_ERROR("call gettimeofday failed! err=%d", errno);
    }
    else
    {
        int64_t diff = (int64_t)((tz.tz_minuteswest) * 60 * 60 * 1000 * 1000);
        if (toLocal)
        {
            tm -= diff;
        }
        else
        {
            tm += diff;
        }
    }

    return tm;
}

static int64_t _fromSystemTime(const BfxSystemTime *st, bool asLocal)
{
    struct tm timestruct;
    timestruct.tm_sec = st->second;
    timestruct.tm_min = st->minute;
    timestruct.tm_hour = st->hour;
    timestruct.tm_mday = st->dayOfMonth;
    timestruct.tm_mon = st->month - 1;
    timestruct.tm_year = st->year - 1900;
    timestruct.tm_wday = st->dayOfWeak;  // mktime/timegm ignore this
    timestruct.tm_yday = 0;     // mktime/timegm ignore this
    timestruct.tm_isdst = -1;    // attempt to figure it out
    timestruct.tm_gmtoff = 0;     // not a POSIX field, so mktime/timegm ignore
    timestruct.tm_zone = NULL;  // not a POSIX field, so mktime/timegm ignore


    int64_t milliSeconds;
    SysTime seconds;

    // Certain exploded dates do not really exist due to daylight saving times,
    // and this causes mktime() to return implementation-defined values when
    // tm_isdst is set to -1. On Android, the function will return -1, while the
    // C libraries of other platforms typically return a liberally-chosen value.
    // Handling this requires the special code below.

    // _sysTimeFromTimeStruct() modifies the input structure, save current value.
    struct tm timestruct0 = timestruct;

    seconds = _sysTimeFromTimeStruct(&timestruct, asLocal);
    if (seconds == -1)
    {
        // Get the time values with tm_isdst == 0 and 1, then select the closest one
        // to UTC 00:00:00 that isn't -1.
        timestruct = timestruct0;
        timestruct.tm_isdst = 0;
        int64_t seconds_isdst0 = _sysTimeFromTimeStruct(&timestruct, asLocal);

        timestruct = timestruct0;
        timestruct.tm_isdst = 1;
        int64_t seconds_isdst1 = _sysTimeFromTimeStruct(&timestruct, asLocal);

        // seconds_isdst0 or seconds_isdst1 can be -1 for some timezones.
        // E.g. "CLST" (Chile Summer Time) returns -1 for 'tm_isdt == 1'.
        if (seconds_isdst0 < 0)
        {
            seconds = seconds_isdst1;
        }
        else if (seconds_isdst1 < 0)
        {
            seconds = seconds_isdst0;
        }
        else
        {
            seconds = min(seconds_isdst0, seconds_isdst1);
        }
    }

    // Handle overflow.  Clamping the range to what mktime and timegm might
    // return is the best that can be done here.  It's not ideal, but it's better
    // than failing here or ignoring the overflow case and treating each time
    // overflow as one second prior to the epoch.
    if (seconds == -1 && (st->year < 1969 || st->year > 1970))
    {
        // If exploded.year is 1969 or 1970, take -1 as correct, with the
        // time indicating 1 second prior to the epoch.  (1970 is allowed to handle
        // time zone and DST offsets.)  Otherwise, return the most future or past
        // time representable.  Assumes the time_t epoch is 1970-01-01 00:00:00 UTC.
        //
        // The minimum and maximum representible times that mktime and timegm could
        // return are used here instead of values outside that range to allow for
        // proper round-tripping between exploded and counter-type time
        // representations in the presence of possible truncation to time_t by
        // division and use with other functions that accept time_t.
        //
        // When representing the most distant time in the future, add in an extra
        // 999ms to avoid the time being less than any other possible value that
        // this function can return.

        // On Android, SysTime is int64, special care must be taken to avoid
        // overflows.
        const int64_t minSeconds = (sizeof(SysTime) < sizeof(int64_t))
            ? _timeMin()
            : INT32_MIN;
        const int64_t maxSeconds = (sizeof(SysTime) < sizeof(int64_t))
            ? _timeMax()
            : INT32_MAX;
        if (st->year < 1969)
        {
            milliSeconds = minSeconds * 1000;
        }
        else
        {
            milliSeconds = maxSeconds * 1000;
            milliSeconds += (1000 - 1);
        }
    }
    else
    {
        milliSeconds = seconds * 1000 + st->millisecond;
    }

    // 调整到windows epoch时间
    return (milliSeconds * 1000 + _timeTToMicrosecondsOffset);
}

static void _toSystemTime(int64_t ms, BfxSystemTime *st, bool asLocal)
{
    int64_t microSeconds = ms - _timeTToMicrosecondsOffset;

    int64_t milliSecond;
    SysTime seconds;
    if (microSeconds >= 0)
    {
        int64_t milliSeconds = microSeconds / 1000;
        seconds = milliSeconds / 1000;
        milliSecond = milliSeconds % 1000;
    }
    else
    {
        int64_t milliSeconds = (microSeconds - 1000 + 1) / 1000;
        seconds = (milliSeconds - 1000 + 1) / 1000;
        milliSecond = milliSeconds % 1000;
        if (milliSecond < 0)
        {
            milliSecond += 1000;
        }
    }

    struct tm timestruct;
    _sysTimeToTimeStruct(seconds, &timestruct, asLocal);

    st->year = timestruct.tm_year + 1900;
    st->month = timestruct.tm_mon + 1;
    st->dayOfWeak = timestruct.tm_wday;
    st->dayOfMonth = timestruct.tm_mday;
    st->hour = timestruct.tm_hour;
    st->minute = timestruct.tm_min;
    st->second = timestruct.tm_sec;
    st->millisecond = milliSecond;
}
