#ifndef LOOM_WORKER_BASICTASKS_H
#define LOOM_WORKER_BASICTASKS_H

#include "libloom/taskinstance.h"

class ConstTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class MergeTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class OpenTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class LineSplitTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


#endif // LOOM_WORKER_BASICTASKS_H
