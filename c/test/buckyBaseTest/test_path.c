#include "../src/BuckyBase/BuckyBase.h"

void testPath() {

    BfxPathCharType temp[BFX_PATH_MAX];
    const BfxPathCharType* path = BFX_PATH_LITERAL("C:\\name\\name2\\.\\name3\\name4");
   
    bool ret = BfxPathCanonicalize(temp, path);
    assert(ret);

    ret = BfxPathRemoveFileSpec(temp);
    assert(ret);
    assert(BfxPathCompare(temp, BFX_PATH_LITERAL("C:\\name\\name2\\name3")) == 0);

    BfxPathCharType temp2[BFX_PATH_MAX];
    BfxPathJoin(temp2, temp, BFX_PATH_LITERAL("..\\name5"));
    assert(BfxPathCompare(temp2, BFX_PATH_LITERAL("C:\\name\\name2\\name5")) == 0);

    BfxPathStringCat(temp2, BFX_PATH_MAX, BFX_PATH_LITERAL(".blog.cfg"));

    {
        BfxPathCharType temp[12];
        BfxPathJoin(temp, BFX_PATH_LITERAL("C:\\"), NULL);
        BfxPathStringCat(temp, 12, BFX_PATH_LITERAL(".blog.cfg"));
        BLOG_INFO("%ls", temp);
    }

    {
        BfxPathCharType temp[16];
        BfxPathJoin(temp, BFX_PATH_LITERAL("C:\\"), NULL);
        BfxPathStringCat(temp, 16, BFX_PATH_LITERAL(".blog.cfg"));
        BLOG_INFO("%ls", temp);
    }
}