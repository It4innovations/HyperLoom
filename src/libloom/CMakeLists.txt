set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_library(libloom
    compat.h
    socket.cpp
    socket.h
    sendbuffer.h
    sendbuffer.cpp
    listener.cpp
    listener.h
    log.cpp
    log.h
    pbutils.h
    pbutils.cpp
    dictionary.cpp
    dictionary.h
    types.h
    fsutils.cpp
    fsutils.h
    trace.cpp
    trace.h
)

target_include_directories(libloom PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(libloom ${PROTOBUF_LIBRARIES} ${LIBUV_LIBRARY} pthread)
target_link_libraries(libloom pb_comm)

install(TARGETS libloom
        ARCHIVE DESTINATION lib)
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/libloomnet
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
