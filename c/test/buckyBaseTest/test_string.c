#include "../src/BuckyBase/BuckyBase.h"


void testString() {
    sds mystring = sdsnew("Hello World!");
    printf("%s\n", mystring);
    BLOG_INFO("%s\n", mystring);

    sds v = sdsdup(mystring);

    v = sdscat(v, "bucky");
    printf("%s\n", v);

    sdsfree(mystring);
    sdsfree(v);
}
