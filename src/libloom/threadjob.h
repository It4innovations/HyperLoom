#ifndef LIBLOOM_THREADJOB_H
#define LIBLOOM_THREADJOB_H

#include "data.h"
#include "task.h"
#include "taskdesc.h"

namespace loom {

/** Base class for task instance with thread support */
class ThreadJob
{
public:

    ThreadJob(Worker &worker, Task &task);
    virtual ~ThreadJob();

    /** Method to decide if input is sufficiently big to use thread
     *  default implementation just returns true
     */
    virtual bool check_run_in_thread() {
        return true;
    }

    virtual std::shared_ptr<Data> run() = 0;

    void set_inputs(DataVector &input_data) {
        this->inputs = input_data;
    }

    const std::string& get_error_message() {
        return error_message;
    }

    std::unique_ptr<TaskDescription>& get_task_redirect() {
        return task_redirect;
    }


protected:


    void set_error(const std::string &error_message);
    void set_redirect(std::unique_ptr<TaskDescription> tredirect);

    Task &task;
    DataVector inputs;
    std::string error_message;
    std::unique_ptr<TaskDescription> task_redirect;
    const std::string &work_dir;

    static void _work_cb(uv_work_t *req);
    static void _after_work_cb(uv_work_t *req, int status);
};

}

#endif // LIBLOOM_THREADJOB_H
