cmake_minimum_required (VERSION 3.8)

project(bdt2)

add_definitions(-DBFX_STATIC)

include_directories(../../src/)
include_directories(../../3rd/)
include_directories(../../src/BDTCore)

aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

IF(MSVC)
#target_link_libraries(${PROJECT_NAME} Crypt32 libcrypto)
target_link_libraries(${PROJECT_NAME} Shlwapi)
ELSE()
#target_link_libraries(${PROJECT_NAME} crypto.a)
ENDIF()

target_link_libraries(${PROJECT_NAME} BdtCore BuckyBase BuckyFramework mbedtls uv_a sqlite3)