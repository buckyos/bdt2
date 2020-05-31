cmake_minimum_required (VERSION 3.8)

include(../../../../../../src/cconfig.cmake)

SET (ENABLE_PROGRAMS 0)

add_subdirectory(../../../../../../3rd/mbedtls-2.16.1)
add_subdirectory(../../../../../../3rd/libuv-1.29.1)
add_subdirectory(../../../../../../src/BuckyBase)
add_subdirectory(../../../../../../src/BdtCore)
add_subdirectory(../../../../../../src/BuckyFramework)
add_subdirectory(../../../../../../src/LibuvFramework)

project(js2c)

add_definitions(-DBFX_STATIC)

include_directories(../../../../../../src/)
aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} BuckyBase mbedtls uv_a BuckyFramework node)