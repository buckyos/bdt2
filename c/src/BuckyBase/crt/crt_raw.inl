#include "../BuckyBase.h"
#include "./crt.h"

static int _stricmp(const char* dst, const char* src) {
    int f, l;

    do {
        if (((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z')) {
            f -= 'A' - 'a';
        }

        if (((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z')) {
            l -= 'A' - 'a';
        }
    } while (f && (f == l));

    return(f - l);
}

static int _strnicmp(const char* dst, const char* src, size_t count) {
    if (count > 0) {
        int f;
        int l;

        do {
            if (((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z')) {
                f -= 'A' - 'a';
            }

            if (((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z')) {
                l -= 'A' - 'a';
            }

        } while (--count && f && (f == l));

        return (f - l);
    } 

    return 0;
}