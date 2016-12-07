set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_library(libloomnet
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
)

target_include_directories(libloomnet PUBLIC ${PROJECT_SOURCE_DIR}/src)
install(TARGETS libloomnet
        ARCHIVE DESTINATION lib)
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/libloomnet
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
