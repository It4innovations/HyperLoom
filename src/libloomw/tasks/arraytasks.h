#ifndef LIBLOOM_TASKS_ARRAYTASKS_H
#define LIBLOOM_TASKS_ARRAYTASKS_H

#include "libloomw/taskinstance.h"

namespace loom {

class ArrayMakeTask : public loom::TaskInstance
{
public:
    using TaskInstance::TaskInstance;
    void start(loom::DataVector &inputs) override;
};

}

#endif // LIBLOOM_TASKS_ARRAYTASKS_H
