#ifndef LOOM_WORKER_BASICTASKS_H
#define LOOM_WORKER_BASICTASKS_H

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

#endif // LOOM_WORKER_BASICTASKS_H
