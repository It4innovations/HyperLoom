#ifndef LIBLOOMW_TASKINSTANCE_H
#define LIBLOOMW_TASKINSTANCE_H

#include "data.h"
#include "task.h"
#include "taskdesc.h"
#include "resalloc.h"

#include<uv.h>

#include<string>
#include<memory>
#include<vector>

namespace loom {

class Worker;
class Data;

/** Base class for task instance - an actual state of computation of a task */
class TaskInstance
{
public:
    const int INTERNAL_JOB_ID = -2;

    TaskInstance(Worker &worker, std::unique_ptr<Task> &&task, ResourceAllocation &&ra)
        : worker(worker), task(std::move(task)), resource_alloc(std::move(ra))
    {

    }

    virtual ~TaskInstance();

    base::Id get_id() const {
        return task->get_id();
    }

    Task& get_task() const {
        return *task;
    }

    const std::vector<base::Id>& get_inputs() {
        return task->get_inputs();
    }

    const ResourceAllocation& get_resource_alloc() const {
        return resource_alloc;
    }

    ResourceAllocation& get_resource_alloc() {
        return resource_alloc;
    }

    ResourceAllocation pop_resource_alloc();
    const std::string get_task_dir();
    virtual void start(DataVector &input_data) = 0;

protected:
    void fail(const std::string &error_msg);
    void fail_libuv(const std::string &error_msg, int error_code);
    void finish(const DataPtr &output);
    void redirect(std::unique_ptr<TaskDescription> tdesc);


    Worker &worker;
    std::unique_ptr<Task> task;
    bool has_directory;
    ResourceAllocation resource_alloc;
};

}

#endif // LIBLOOMW_TASKINSTANCE_H
