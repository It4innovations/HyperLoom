#ifndef LOOM_WORKER_ARRAYTASKS_H
#define LOOM_WORKER_ARRAYTASKS_H

#include "libloom/taskinstance.h"

class ArrayMakeTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class ArrayGetTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


#endif // LOOM_WORKER_ARRAYTASKS_H
