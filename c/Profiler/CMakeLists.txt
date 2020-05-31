cmake_minimum_required (VERSION 3.8)

project(BdtCoreProfiler)

add_definitions(-DBFX_STATIC)

include_directories(../src/ ../src/BDTCore ../3rd/mbedtls-2.16.1/include ../3rd/sqlite3-3.29.0  ../3rd/openssl_1_1_1-stable/include)

set(BDT_CORE_ROOT ../src/BDTCore)

aux_source_directory(${BDT_CORE_ROOT} BASE_SRC)
aux_source_directory(./Global GLOBAL_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/Protocol PROTOCOL_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/Interface INTERFACE_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/History HISTORY_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/SnClient SN_CLIENT_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/Tunnel TUNNEL_SRCS)
aux_source_directory(${BDT_CORE_ROOT}/Connection CONNECTION_SRCS)

set(SRCS 
${BASE_SRC} 
${GLOBAL_SRCS}
${PROTOCOL_SRCS}
${HISTORY_SRCS}
${INTERFACE_SRCS} 
${SN_CLIENT_SRCS}
${TUNNEL_SRCS} 
${CONNECTION_SRCS} 
${PACKAGE_CONNECTION_SRCS}
main.c
)

add_executable(${PROJECT_NAME} ${SRCS})

#IF(MSVC)
#target_link_libraries(${PROJECT_NAME} Crypt32 libcrypto)
#ELSE()
#target_link_libraries(${PROJECT_NAME} crypto.a)
#ENDIF()

target_link_libraries(${PROJECT_NAME} BuckyBase BuckyFramework mbedtls uv_a sqlite3)