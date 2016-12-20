set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_executable(loom-worker
               main.cpp)
target_include_directories(loom-worker PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(loom-worker libloomw libloom ${LIBUV_LIBRARY} pthread)
target_link_libraries(loom-worker ${PROTOBUF_LIBRARIES})
target_link_libraries(loom-worker ${PYTHON_LIBRARIES})
install (TARGETS loom-worker DESTINATION bin)
