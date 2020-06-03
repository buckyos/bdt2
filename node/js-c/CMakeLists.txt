cmake_minimum_required (VERSION 3.4)

project(bdt2-js-c)

add_definitions(-DBFX_STATIC -DNAPI_DISABLE_CPP_EXCEPTIONS)

include_directories(${CMAKE_JS_INC})
include_directories(./)
include_directories(../../c/src/BDTCore)
include_directories(../../c/src/)

aux_source_directory(. SRCS)

add_library(${PROJECT_NAME} SHARED ${SRCS} ${CMAKE_JS_SRC})

execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} BdtCore BuckyFramework BuckyBase mbedtls sqlite3)
