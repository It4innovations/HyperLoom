set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${PROJECT_SOURCE_DIR}/tests)

add_executable(server-test
               $<TARGET_OBJECTS:loom-server-lib>
               test_scheduler.cpp
               main.cpp)

target_link_libraries(server-test Catch libloom libloomw ${LIBUV_LIBRARY} pthread)
target_link_libraries(server-test ${LPSOLVE_LIBRARY} ${COLAMD_LIBRARY} dl)
target_link_libraries(server-test ${PROTOBUF_LIBRARIES})

target_include_directories(server-test PUBLIC ${PROJECT_SOURCE_DIR})

add_test(NAME server-test COMMAND server-test)
