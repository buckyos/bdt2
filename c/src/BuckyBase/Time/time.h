#ifndef __BUCKY_BASE_TIME_H__
#define __BUCKY_BASE_TIME_H__

typedef struct
{
	int32_t	year;		// ��ݣ�eg: 2019
	int32_t month;		// �·ݣ�1-12
	int32_t dayOfWeak;	// ���еڼ��죬0=�����죬1=����һ��ȡֵ��Χ0-6
	int32_t dayOfMonth;	// ���еڼ��죬��1��ʼ��ȡֵ��Χ1-31
	int32_t hour;		// Сʱ����0��ʼ��ȡֵ��Χ0-23
	int32_t minute;		// ���ӣ���0��ʼ��ȡֵ��Χ0-59
	int32_t second;		// �룬��0��ʼ��ȡֵ��Χ0-59(�����ڴ������룬�����п��ܻ��1���������60��ֵ)
	int32_t millisecond;	// ���룬��0��ʼ��ȡֵ��Χ0-999
} BfxSystemTime;


// ��ȡ��ǰϵͳʱ��
// local=true:����ʱ��  local=false:UTCʱ��
BFX_API(int64_t) BfxTimeGetNow(bool asLocal);

// UTCʱ��<->����ʱ��
BFX_API(int64_t) BfxUTCTimeToLocalTime(int64_t tm);
BFX_API(int64_t) BfxLocalTimeToUTCTime(int64_t tm);

// SYSTEMTIME<->ʱ��
BFX_API(int64_t) BfxSystemTimeToTime(const BfxSystemTime* lpSystemTime, bool asLocal);
BFX_API(void) BfxTimeToSystemTime(int64_t time, BfxSystemTime* lpSystemTime, bool asLocal);

#endif