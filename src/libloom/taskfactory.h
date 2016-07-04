#ifndef LOOM_TASK_FACTORY_H
#define LOOM_TASK_FACTORY_H

#include "data.h"
#include "task.h"
#include "compat.h"

#include<uv.h>

#include<string>
#include<memory>

namespace loom {

class Worker;
class TaskInstance;

class TaskFactory
{
public:
    TaskFactory(const std::string &name)
        : name(name) {}
    virtual ~TaskFactory() {}

    virtual std::unique_ptr<TaskInstance> make_instance(Worker &worker, std::unique_ptr<Task> task) = 0;

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
    std::unique_ptr<TaskInstance> make_instance(Worker &worker, std::unique_ptr<Task> task) {
        return std::make_unique<T>(worker, std::move(task));
    }
};

}

#endif // LOOM_TASK_FACTORY_H
