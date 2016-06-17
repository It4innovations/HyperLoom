project(loom-rt)
cmake_minimum_required(VERSION 2.8)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
