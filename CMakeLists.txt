project(loom-rt)
cmake_minimum_required(VERSION 2.8)
enable_testing()

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

find_library(LPSOLVE_LIBRARY
        NAMES lpsolve55)
mark_as_advanced(LPSOLVE_LIBRARY)

find_library(COLAMD_LIBRARY
        NAMES colamd)
mark_as_advanced(COLAMD_LIBRARY)

	#PATHS ${LPSOLVE_LIBRARIES} ${LPSOLVE_LIB_TRIALPATH}
	#PATH_SUFFIXES lp_solve)

add_subdirectory(src)
add_subdirectory(tests)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
