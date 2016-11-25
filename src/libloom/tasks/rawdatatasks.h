#ifndef LIBLOOM_TASKS_RAWDATATASKS_H
#define LIBLOOM_TASKS_RAWDATATASKS_H

#include "libloom/ttinstance.h"
#include "libloom/threadjob.h"

namespace loom {

class ConstTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class MergeJob : public loom::ThreadJob
{
public:
    using ThreadJob::ThreadJob;

    bool check_run_in_thread();
    std::shared_ptr<loom::Data> run();
};


class OpenTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class SplitTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};

}

#endif // LIBLOOM_TASKS_RAWDATATASKS_H
