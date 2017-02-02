#ifndef LIBLOOMW_TASK_FACTORY_H
#define LIBLOOMW_TASK_FACTORY_H

#include "data.h"
#include "task.h"
#include "libloom/compat.h"

#include<uv.h>

#include<string>
#include<memory>

namespace loom {

class Worker;
class TaskInstance;
class ResourceAllocation;

/** Abstract class for creating task instances */
class TaskFactory
{
public:
    TaskFactory(const std::string &name)
        : name(name) {}
    virtual ~TaskFactory() {}

    virtual std::unique_ptr<TaskInstance> make_instance(Worker &worker,
                                                        std::unique_ptr<Task> task,
                                                        ResourceAllocation &&ra) = 0;

    const std::string &get_name() const {
        return name;
    }

protected:
    std::string name;
};

template <typename T> class SimpleTaskFactory : public TaskFactory
{
public:
    SimpleTaskFactory(const std::string &name) :
        TaskFactory(name) {}
    std::unique_ptr<TaskInstance> make_instance(Worker &worker,
                                                std::unique_ptr<Task> task,
                                                ResourceAllocation &&ra) override {
        return std::make_unique<T>(worker, std::move(task), std::move(ra));
    }
};

}

#endif // LIBLOOMW_TASK_FACTORY_H
