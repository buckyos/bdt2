cmake_minimum_required (VERSION 3.4)
project(bdt2-js)

add_definitions(-DNAPI_VERSION=4)
#add_definitions(-DNAPI_EXPERIMENTAL)

include(../c/cconfig.cmake)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
include_directories(../c/3rd/libuv-1.29.1/include)

add_subdirectory(../c/3rd/mbedtls-2.16.1 mbedtls-2.16.1)
add_subdirectory(../c/3rd/sqlite3-3.29.0 sqlite3-3.29.0)
add_subdirectory(../c/src/BuckyBase BuckyBase)
add_subdirectory(../c/src/BuckyFramework BuckyFramework)
add_subdirectory(../c/src/BDTCore BDTCore)
add_subdirectory(./js-c)

