project(loom-rt)
cmake_minimum_required(VERSION 2.8)
enable_testing()

file (STRINGS "version" LOOM_VERSION)

message(STATUS "Loom version: " ${LOOM_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# LibUV
find_path(LIBUV_INCLUDE_DIR uv.h
	  HINTS ${PC_LIBUV_INCLUDEDIR} ${PC_LIBUV_INCLUDE_DIRS}
	  ${LIMIT_SEARCH})

find_library(LIBUV_LIBRARY
             NAMES libuv uv)

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)

# Protobuffers
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Python
find_package(PythonLibs 3.4 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

# argp
find_package(argp)
include_directories(${ARGP_INCLUDE_PATH})

add_subdirectory(pb)
add_subdirectory(src)
add_subdirectory(tests)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
