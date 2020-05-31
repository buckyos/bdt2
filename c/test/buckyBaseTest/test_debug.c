#include "../../src/BuckyBase/BuckyBase.h"


void testDebug() {

    BfxPathCharType main[BFX_PATH_MAX];
    int ret = BfxGetModulePath(testDebug, main, sizeof(main) / sizeof(BfxPathCharType));
    assert(ret > 0);

    BfxGetExecutablePath(main, sizeof(main) / sizeof(BfxPathCharType));
    assert(ret > 0);
}