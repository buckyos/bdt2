cmake_minimum_required (VERSION 3.8)

project(BuckyFramework)

add_definitions(-DBFX_STATIC)

#add_definitions(-DBFX_FRAMEWORK_NATIVE)
#set(BFX_FRAMEWORK_NATIVE 1)


add_definitions(-DBFX_FRAMEWORK_UV)
set(BFX_FRAMEWORK_UV 1)

include_directories(../../3rd/libuv-1.29.1/src)
include_directories(../BuckyBase)
include_directories(../BDTCore)
include_directories(../)

aux_source_directory(. SRCS)
aux_source_directory(./net/uv SRCS)
aux_source_directory(./thread/common SRCS)
aux_source_directory(./framework SRCS)

if(BFX_FRAMEWORK_NATIVE)
	aux_source_directory(./thread/native SRCS)
	aux_source_directory(./thread/native/timer SRCS)
	if(BFX_OS_WIN)
		aux_source_directory(./thread/native/win SRCS)
	else()
		aux_source_directory(./thread/native/posix SRCS)
	endif()
endif()

if(BFX_FRAMEWORK_UV)
aux_source_directory(./thread/uv SRCS)
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS})