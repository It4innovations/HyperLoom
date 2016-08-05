#ifndef LIBLOOM_TASKS_BASICTASKS_H
#define LIBLOOM_TASKS_BASICTASKS_H

#include "libloom/taskinstance.h"

class GetTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class SliceTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};

#endif // LIBLOOM_TASKS_BASICTASKS_H