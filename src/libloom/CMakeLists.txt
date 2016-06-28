set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall")
add_library(libloom
            connection.cpp
            connection.h
            worker.cpp
            worker.h
            taskinstance.cpp
            taskinstance.h
            taskfactory.h
            databuilder.cpp
            databuilder.h
            data.cpp
            data.h
            interconnect.h
            interconnect.cpp
            task.cpp
            task.h
            sendbuffer.h
            sendbuffer.cpp
            loomcomm.pb.h
            loomcomm.pb.cc
            loomplan.pb.h
            loomplan.pb.cc
            log.h
            log.cpp
            types.h
            utils.h
            utils.cpp)
target_include_directories(libloom PUBLIC ${PROJECT_SOURCE_DIR}/src)

