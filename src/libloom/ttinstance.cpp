#include "ttinstance.h"

#include "worker.h"

using namespace loom;

void ThreadTaskInstance::start(DataVector &input_data)
{
    this->inputs = input_data;
    if (run_in_thread(input_data)) {
        UV_CHECK(uv_queue_work(worker.get_loop(), &work, _work_cb, _after_work_cb));
    } else {
        _work_cb(&work);
        _after_work_cb(&work, 0);
    }
}

bool ThreadTaskInstance::run_in_thread(DataVector &input_data)
{
    return true;
}

void ThreadTaskInstance::set_error(const std::string &error_message)
{
    this->error_message = error_message;
}

void ThreadTaskInstance::_work_cb(uv_work_t *req)
{
    ThreadTaskInstance *ttinstance = static_cast<ThreadTaskInstance*>(req->data);
    ttinstance->result = ttinstance->run();
}

void ThreadTaskInstance::_after_work_cb(uv_work_t *req, int status)
{
    UV_CHECK(status);
    ThreadTaskInstance *ttinstance = static_cast<ThreadTaskInstance*>(req->data);
    if (ttinstance->error_message.empty()) {
        if (ttinstance->result) {
            ttinstance->finish(ttinstance->result);
        } else {
            ttinstance->fail("ThreadTaskInstace::run has returned nullptr");
        }
    } else {
        ttinstance->fail(ttinstance->error_message);
    }
}

ThreadTaskInstance::~ThreadTaskInstance()
{
}
