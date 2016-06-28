set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall")
add_library(loom-server-lib OBJECT
               server.cpp
               server.h
               dummyworker.cpp
               dummyworker.h
               taskmanager.cpp
               taskmanager.h
               tasknode.cpp
               tasknode.h
               workerconn.cpp
               workerconn.h
               freshconn.cpp
               freshconn.h
               clientconn.cpp
               clientconn.h
               resendtask.cpp
               resendtask.h)

target_include_directories(loom-server-lib PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_executable(loom-server
               $<TARGET_OBJECTS:loom-server-lib>
               main.cpp)

target_link_libraries(loom-server libloom uv protobuf pthread)
