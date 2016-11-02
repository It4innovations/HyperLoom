#ifndef LIBLOOM_TASKREDIRECT_H
#define LIBLOOM_TASKREDIRECT_H


#include "data.h"
#include <string.h>

namespace loom {

struct TaskDescription
{
    std::string task_type;
    std::string config;
    DataVector inputs;
};

}

#endif // LIBLOOM_TASKREDIRECT_H
