#ifndef LIBLOOMW_TTINSTANCE_H
#define LIBLOOMW_TTINSTANCE_H

#include "data.h"
#include "taskinstance.h"
#include "worker.h"
#include "libloom/log.h"

#include<uv.h>

#include<string>
#include<memory>
#include<vector>

namespace loom {

/** Base class for task instance with thread support */
template<typename T> class ThreadTaskInstance : public TaskInstance
{
public:

    ThreadTaskInstance(Worker &worker, std::unique_ptr<Task> task)
        : TaskInstance(worker, std::move(task)), job(worker, *this->task)
    {
        work.data = this;
    }

    virtual void start(DataVector &input_data) override {
        job.set_inputs(input_data);
        if (job.check_run_in_thread()) {
            UV_CHECK(uv_queue_work(worker.get_loop(), &work, _work_cb, _after_work_cb));
        } else {
            _work_cb(&work);
            _after_work_cb(&work, 0);
        }
    }

protected:
    uv_work_t work;
    T job;
    DataPtr result;

    static void _work_cb(uv_work_t *req)
    {
        ThreadTaskInstance *ttinstance = static_cast<ThreadTaskInstance*>(req->data);
        ttinstance->result = ttinstance->job.run();
    }

    static void _after_work_cb(uv_work_t *req, int status)
    {
        UV_CHECK(status);
        ThreadTaskInstance *ttinstance = static_cast<ThreadTaskInstance*>(req->data);
        bool redirect = ttinstance->job.get_task_redirect();
        const std::string &err = ttinstance->job.get_error_message();
        if (err.empty()) {
            if (ttinstance->result && !redirect) {
                ttinstance->finish(ttinstance->result);
            } else if (!ttinstance->result && redirect) {
                ttinstance->redirect(std::move(ttinstance->job.get_task_redirect()));
            } else {
                 ttinstance->fail("ThreadTaskInstace::run returned nullptr or incosistent returned values");
            }
        } else {
            ttinstance->fail(err);
        }
    }

};

}

#endif // LIBLOOMW_TASKINSTANCE_H
