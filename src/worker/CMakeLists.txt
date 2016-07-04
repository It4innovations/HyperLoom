set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_executable(loom-worker
               loomrun.pb.cc
               loomrun.pb.h
               runtask.cpp
               runtask.h
               basictasks.cpp
               basictasks.h
               main.cpp)
target_include_directories(loom-worker PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(loom-worker libloom uv protobuf pthread)
