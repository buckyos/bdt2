cmake_minimum_required (VERSION 3.8)

project(frameworkTest)

add_definitions(-DBFX_STATIC)

include_directories(../../src/)
include_directories(../../src/LibuvFramework/)
aux_source_directory(. SRCS)

#add_executable(${PROJECT_NAME} frameworkTest.c)
#add_executable(${PROJECT_NAME} udptest.c)
add_executable(${PROJECT_NAME} timer.c)
target_link_libraries(${PROJECT_NAME} BdtCore BuckyBase mbedtls uv_a)