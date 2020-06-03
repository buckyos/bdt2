cmake_minimum_required (VERSION 3.8)

project(BdtCore)

add_definitions(-DBFX_STATIC)

include_directories(../ ../../3rd/mbedtls-2.16.1/include ../../3rd/sqlite3-3.29.0 ../../3rd/openssl_1_1_1-stable/include)

aux_source_directory(. BASE_SRC)
aux_source_directory(./Global GLOBAL_SRCS)
aux_source_directory(./Protocol PROTOCOL_SRCS)
aux_source_directory(./History HISTORY_SRCS)
aux_source_directory(./History/SqliteStorage SQLITE_STORAGE_SRCS)
aux_source_directory(./Interface INTERFACE_SRCS)
aux_source_directory(./SnClient SN_CLIENT_SRCS)
aux_source_directory(./Tunnel TUNNEL_SRCS)
aux_source_directory(./Connection CONNECTION_SRCS)
set(SRCS 
${BASE_SRC} 
${GLOBAL_SRCS}
${PROTOCOL_SRCS}
${HISTORY_SRCS}
${INTERFACE_SRCS} 
${SN_CLIENT_SRCS}
${TUNNEL_SRCS} 
${CONNECTION_SRCS} 
${SQLITE_STORAGE_SRCS}
)

add_library(${PROJECT_NAME} STATIC ${SRCS})