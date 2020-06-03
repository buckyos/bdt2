cmake_minimum_required (VERSION 3.8)

project(packageConn)

add_definitions(-DBFX_STATIC)

include_directories(../../src/)
include_directories(../../3rd/)
include_directories(../../src/BDTCore)

aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

IF(MSVC)
target_link_libraries(${PROJECT_NAME} Crypt32)
ENDIF()

target_link_libraries(${PROJECT_NAME} BdtCore BuckyBase BuckyFramework mbedtls uv_a sqlite3)