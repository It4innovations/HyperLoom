#ifndef LIBLOOMW_TASKREDIRECT_H
#define LIBLOOMW_TASKREDIRECT_H


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

#endif // LIBLOOMW_TASKREDIRECT_H
