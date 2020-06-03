cmake_minimum_required (VERSION 3.4)

set(BDT_C_SRC "../../../../../c")
include(${BDT_C_SRC}/cconfig.cmake)
include_directories(${BDT_C_SRC}/3rd/libuv-1.29.1/include)

add_subdirectory(${BDT_C_SRC}/3rd/mbedtls-2.16.1 mbedtls-2.16.1)
add_subdirectory(${BDT_C_SRC}/3rd/libuv-1.29.1 libuv-1.29.1)
add_subdirectory(${BDT_C_SRC}/3rd/sqlite3-3.29.0 sqlite3-3.29.0)
add_subdirectory(${BDT_C_SRC}/src/BDTCore BDTCore)
add_subdirectory(${BDT_C_SRC}/src/BuckyFramework BuckyFramework)
add_subdirectory(${BDT_C_SRC}/src/BuckyBase BuckyBase)

include_directories(${BDT_C_SRC}/src/BDTCore)
include_directories(${BDT_C_SRC}/src/)

set(JNI_SRCS Global.cpp PeerInfo.cpp Stack.cpp Connection.cpp)
add_library(bdt2-jni SHARED ${JNI_SRCS})

find_library(log-lib log)

target_link_libraries(bdt2-jni BdtCore BuckyBase BuckyFramework mbedtls uv_a sqlite3 ${log-lib})