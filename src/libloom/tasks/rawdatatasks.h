#ifndef LIBLOOM_TASKS_RAWDATATASKS_H
#define LIBLOOM_TASKS_RAWDATATASKS_H

#include "libloom/ttinstance.h"

namespace loom {

class ConstTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


class MergeTask : public loom::ThreadTaskInstance
{
public:
    using ThreadTaskInstance::ThreadTaskInstance;

    bool run_in_thread(loom::DataVector &input_data);
protected:
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
