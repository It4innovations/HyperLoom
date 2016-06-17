set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall")

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${PROJECT_SOURCE_DIR}/tests)

add_executable(server-test
               $<TARGET_OBJECTS:loom-server-lib>
               scheduler.cpp
               main.cpp)
target_link_libraries(server-test Catch  libloom uv protobuf pthread)
target_include_directories(server-test PUBLIC ${PROJECT_SOURCE_DIR})

add_test(NAME server-test COMMAND server-test)
