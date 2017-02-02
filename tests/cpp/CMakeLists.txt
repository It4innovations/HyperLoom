set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${PROJECT_SOURCE_DIR}/tests)

add_executable(cpp-test
               $<TARGET_OBJECTS:loom-server-lib>
               test_scheduler.cpp
               test_resourcem.cpp
               main.cpp)

target_link_libraries(cpp-test Catch libloom libloomw ${LIBUV_LIBRARY} pthread)
target_link_libraries(cpp-test ${LPSOLVE_LIBRARY} ${COLAMD_LIBRARY} dl)
target_link_libraries(cpp-test ${PROTOBUF_LIBRARIES})

target_include_directories(cpp-test PUBLIC ${PROJECT_SOURCE_DIR})

add_test(NAME cpp-test COMMAND cpp-test)
