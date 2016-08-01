#ifndef LIBLOOM_TASKS_ARRAYTASKS_H
#define LIBLOOM_TASKS_ARRAYTASKS_H

#include "libloom/taskinstance.h"

class ArrayMakeTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};

#endif // LIBLOOM_TASKS_ARRAYTASKS_H
