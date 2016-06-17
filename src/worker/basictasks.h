#ifndef LOOM_WORKER_BASICTASKS_H
#define LOOM_WORKER_BASICTASKS_H

#include "libloom/taskinstance.h"

class ConstTask : public loom::TaskInstance
{
public:
    ConstTask(loom::Worker &worker, std::unique_ptr<loom::Task> task);
    void start(loom::DataVector &inputs);
};


class MergeTask : public loom::TaskInstance
{
public:
    MergeTask(loom::Worker &worker, std::unique_ptr<loom::Task> task);
    void start(loom::DataVector &inputs);
};


#endif // LOOM_WORKER_BASICTASKS_H
