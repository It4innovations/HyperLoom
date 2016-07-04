#ifndef LOOM_TASKINSTANCE_H
#define LOOM_TASKINSTANCE_H

#include "data.h"
#include "task.h"

#include<uv.h>

#include<string>
#include<memory>
#include<vector>

namespace loom {

class Worker;
class Data;

typedef std::vector<std::shared_ptr<Data>*> DataVector;

class TaskInstance
{
public:
    const int INTERNAL_JOB_ID = -2;

    TaskInstance(Worker &worker, std::unique_ptr<Task> task)
        : worker(worker), task(std::move(task))
    {

    }

    virtual ~TaskInstance() {}
    
    int get_id() const {
        return task->get_id();
    }

    const std::vector<Id>& get_inputs() {
        return task->get_inputs();
    }

    const std::string get_task_dir();

    virtual void start(DataVector &input_data) = 0;

protected:

    void finish(std::unique_ptr<Data> output);
    void finish_without_data();

    Worker &worker;
    std::unique_ptr<Task> task;
    bool has_directory;
};

}

#endif // LOOM_TASKINSTANCE_H
