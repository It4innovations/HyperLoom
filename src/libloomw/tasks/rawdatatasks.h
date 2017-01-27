#ifndef LIBLOOM_TASKS_RAWDATATASKS_H
#define LIBLOOM_TASKS_RAWDATATASKS_H

#include "libloomw/ttinstance.h"
#include "libloomw/threadjob.h"

namespace loom {

class ConstTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};


class MergeJob : public loom::ThreadJob
{
public:
    using ThreadJob::ThreadJob;

    bool check_run_in_thread();
    DataPtr run() override;
};


class OpenTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};


class SplitTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};


class SaveJob : public loom::ThreadJob
{
public:
    using ThreadJob::ThreadJob;
    DataPtr run() override;
};


}

#endif // LIBLOOM_TASKS_RAWDATATASKS_H
