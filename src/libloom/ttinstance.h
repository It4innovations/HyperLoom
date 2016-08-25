#ifndef LOOM_TTINSTANCE_H
#define LOOM_TTINSTANCE_H

#include "data.h"
#include "taskinstance.h"

#include<uv.h>

#include<string>
#include<memory>
#include<vector>

namespace loom {

/** Base class for task instance with thread support */
class ThreadTaskInstance : public TaskInstance
{
public:

    ThreadTaskInstance(Worker &worker, std::unique_ptr<Task> task)
        : TaskInstance(worker, std::move(task))
    {
        work.data = this;
    }

    virtual ~ThreadTaskInstance();
    
    virtual void start(DataVector &input_data);

    /** Method to decide if input is sufficiently big to use thread
     *  default implementation just returns true
     */
    virtual bool run_in_thread(DataVector &input_data);

protected:    

    /** This method is called outside of main thread if run_in_thread has returned true
     *  IMPORTANT: It can read only member variable "inputs" and calls method "set_error"
     *  All other things are not thread-safe!
     *  In case of error, call set_error and return nullptr
     */
    virtual std::shared_ptr<Data> run() = 0;
    void set_error(std::string &error_message);

    DataVector inputs;
    uv_work_t work;
    std::shared_ptr<Data> result;
    std::string error_message;

    static void _work_cb(uv_work_t *req);
    static void _after_work_cb(uv_work_t *req, int status);
};

}

#endif // LOOM_TASKINSTANCE_H
