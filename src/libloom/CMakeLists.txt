set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_library(libloom
            data/externfile.cpp
            data/externfile.h
            data/rawdata.h
            data/rawdata.cpp
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
            unpacking.cpp
            unpacking.h
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
            compat.h
            log.h
            log.cpp
            types.h
            utils.h
            utils.cpp)

target_include_directories(libloom PUBLIC ${PROJECT_SOURCE_DIR}/src)

