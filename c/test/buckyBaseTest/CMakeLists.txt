cmake_minimum_required (VERSION 3.8)

project(buckyBaseTest)

add_definitions(-DBFX_STATIC)

include_directories(../../3rd/openssl_1_1_1-stable/include)
include_directories(../../src/)
include_directories(../../src/BDTCore)

aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

#IF(MSVC)
#target_link_libraries(${PROJECT_NAME} Crypt32 libcrypto)
#ELSE()
#target_link_libraries(${PROJECT_NAME} crypto.a)
#ENDIF()

target_link_libraries(${PROJECT_NAME} mbedtls uv_a sqlite3 BuckyBase BuckyFramework BdtCore)