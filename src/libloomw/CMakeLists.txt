set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
add_library(libloomw
            data/externfile.cpp
            data/externfile.h
            data/rawdata.h
            data/rawdata.cpp
            data/array.h
            data/array.cpp
            data/index.h
            data/index.cpp
            tasks/basetasks.cpp
            tasks/basetasks.h
            tasks/rawdatatasks.cpp
            tasks/rawdatatasks.h
            tasks/arraytasks.cpp
            tasks/arraytasks.h
            tasks/runtask.cpp
            tasks/runtask.h
            tasks/python_wrapper.cpp
            tasks/python_wrapper.h
            tasks/python.cpp
            tasks/python.h
            tasks/loomrun.pb.cc
            tasks/loomrun.pb.h
            worker.cpp
            worker.h
            taskinstance.cpp
            taskinstance.h
            threadjob.cpp
            threadjob.h
            ttinstance.h
            taskfactory.h
            dictionary.cpp
            dictionary.h
            data.cpp
            data.h
            unpacking.cpp
            unpacking.h
            interconnect.h
            interconnect.cpp
            task.cpp
            task.h
            taskdesc.h
            loomcomm.pb.h
            loomcomm.pb.cc
            loomplan.pb.h
            loomplan.pb.cc
            types.h
            config.cpp
            config.h
            utils.h
            utils.cpp)

target_include_directories(libloomw PUBLIC ${PROJECT_SOURCE_DIR}/src)
install(TARGETS libloomw
        ARCHIVE DESTINATION lib)
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/libloomw
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
