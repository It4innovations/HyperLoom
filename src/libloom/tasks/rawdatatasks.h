#ifndef LIBLOOM_TASKS_RAWDATATASKS_H
#define LIBLOOM_TASKS_RAWDATATASKS_H

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


class SplitTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs);
};


#endif // LIBLOOM_TASKS_RAWDATATASKS_H
