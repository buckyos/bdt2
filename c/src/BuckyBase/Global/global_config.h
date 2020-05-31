
#ifndef __BUCKYBASEGLOBALCONFIG_H__
#define __BUCKYBASEGLOBALCONFIG_H__

// 三个标准宏，用以区分当前的版本
// BFX_DEBUG           // 调试版
// BFX_RELEASE         // 日志版
// BFX_PRODUCTRELEASE  // 发布版
// ！！！上面三个宏不可同时定义！！！

// 使用者可以根据需要在外部定义对应的config.h，会通过下面三个步骤来确定include哪个标准config
// 1. 如果发现已经定义了BFX_CONFIG_HEADER宏，那么不再尝试include标准config文件
// 2. 如果发现已经定义了BFX_CONFIG_HEADER_INCLUDEFILE宏，那么会尝试include该头文件
// 3. 1和2都不符合，那么使用标准的config.h


// 如果没有定义任何一个预定义宏，那么根据编译器的DEBUG宏，来判断是不是调试版本，并定义BFX_DEBUG宏
#if !defined(BFX_DEBUG) || !defined(BFX_RELEASE) || !defined(BFX_PRODUCTRELEASE)
    #if defined(DEBUG) || defined(_DEBUG)
        #define BFX_DEBUG   1
    #endif // DEBUG
#endif

//如果不是调试版，那么根据判断是开发版(release)还是发布版(productrelease)
#ifndef BFX_DEBUG
    #define BFX_NODEBUG     1
    
    // 如果没有预定义BFX_RELEASE或者BFX_PRODUCTRELEASE宏，那么通过是否预定义了BFX_LOG判断是不是日志版Release
    #if !defined(BFX_RELEASE) || !defined(BFX_PRODUCTRELEASE)
        #ifdef BFX_LOG
            #define BFX_RELEASE         1
        #else // !BFX_LOG
            #define BFX_PRODUCTRELEASE  1
        #endif // !BFX_LOG
    #endif
    
    #if defined(BFX_RELEASE) && defined(BFX_PRODUCTRELEASE)
        #error  BFX_RELEASE and BFX_PRODUCTRELEASE defined at same time!
    #endif

#else // !BFX_DEBUG
    #if defined(BFX_RELEASE) || defined(BFX_PRODUCTRELEASE)
        #error BFX_RELEASE or BFX_PRODUCTRELEASE defined with BFX_DEBUG!
    #endif // BFX_RELEASE || BFX_PRODUCTRELEASE
#endif // !BFX_DEBUG


#ifndef BFX_CONFIG_HEADER
    #ifdef BFX_CONFIG_HEADER_INCLUDEFILE
        #include BFX_CONFIG_HEADER_INCLUDEFILE
    #else // include标准的config
        #if defined(BFX_DEBUG) || defined(BFX_RELEASE)
            //#include "./BaseReleaseConfig.h"
			
        #else
            //#include "./BaseProductReleaseConfig.h"
        #endif
    #endif
#endif // !BFX_CONFIG_HEADER


#endif // __BUCKYBASEGLOBALCONFIG_H__  