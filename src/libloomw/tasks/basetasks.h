#ifndef LIBLOOM_TASKS_BASICTASKS_H
#define LIBLOOM_TASKS_BASICTASKS_H

#include "libloomw/taskinstance.h"

namespace loom {

class GetTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};


class SliceTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};


class SizeTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};

class LengthTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};

}

#endif // LIBLOOM_TASKS_BASICTASKS_H
