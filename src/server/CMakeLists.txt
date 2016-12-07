set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_library(loom-server-lib OBJECT
               server.cpp
               server.h
               scheduler.cpp
               scheduler.h
               dummyworker.cpp
               dummyworker.h
               taskmanager.cpp
               taskmanager.h
               workerconn.cpp
               workerconn.h
               freshconn.cpp
               freshconn.h
               clientconn.cpp
               clientconn.h
               compstate.h
               compstate.cpp
               taskstate.h
               taskstate.cpp
               plannode.cpp
               plannode.h
               plan.h
               plan.cpp)

target_include_directories(loom-server-lib PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_executable(loom-server
               $<TARGET_OBJECTS:loom-server-lib>
               main.cpp)

target_link_libraries(loom-server libloomnet libloom ${LIBUV_LIBRARY} pthread)
target_link_libraries(loom-server ${PROTOBUF_LIBRARIES})

install (TARGETS loom-server DESTINATION bin)
