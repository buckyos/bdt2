#ifndef __BUCKY_BASE_STRING_H__
#define __BUCKY_BASE_STRING_H__
#include "../Global/basic_type.h"
#if defined(EMBEDDED_UBUNTU_12)
#include <stdarg.h>
#endif

#define BFX_STRING_ON_STACK_LENGTH 64

typedef union tagBfxStringData {
	uint8_t str[BFX_STRING_ON_STACK_LENGTH];
	uint8_t* pstr;
} BfxStringData;

typedef struct BfxString {
	uint16_t length;
	BfxStringData data;
}BfxString;


// sds相关接口
typedef char* sds;

sds sdsnewlen(const void* init, size_t initlen);
sds sdsnew(const char* init);
sds sdsempty(void);
sds sdsdup(const sds s);
void sdsfree(sds s);
sds sdsgrowzero(sds s, size_t len);
sds sdscatlen(sds s, const void* t, size_t len);
sds sdscat(sds s, const char* t);
sds sdscatsds(sds s, const sds t);
sds sdscpylen(sds s, const char* t, size_t len);
sds sdscpy(sds s, const char* t);

sds sdscatvprintf(sds s, const char* fmt, va_list ap);
sds sdscatprintf(sds s, const char* fmt, ...);

sds sdscatfmt(sds s, char const* fmt, ...);
sds sdstrim(sds s, const char* cset);
void sdsrange(sds s, ssize_t start, ssize_t end);
void sdsupdatelen(sds s);
void sdsclear(sds s);
int sdscmp(const sds s1, const sds s2);
sds* sdssplitlen(const char* s, ssize_t len, const char* sep, int seplen, int* count);
void sdsfreesplitres(sds* tokens, int count);
void sdstolower(sds s);
void sdstoupper(sds s);
sds sdsfromlonglong(long long value);
sds sdscatrepr(sds s, const char* p, size_t len);
sds* sdssplitargs(const char* line, int* argc);
sds sdsmapchars(sds s, const char* from, const char* to, size_t setlen);
sds sdsjoin(char** argv, int argc, char* sep);
sds sdsjoinsds(sds* argv, int argc, const char* sep, size_t seplen);


#endif // __BUCKY_BASE_STRING_H__