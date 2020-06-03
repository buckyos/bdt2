cmake_minimum_required (VERSION 3.8)

project(sqlite3)

add_definitions(-DBFX_STATIC)

set(SRCS 
sqlite3.c
)

add_library(${PROJECT_NAME} STATIC ${SRCS})