
#include "threadjob.h"
#include "worker.h"

using namespace loom;

ThreadJob::ThreadJob(Worker &worker, Task &task)
    : task(task), work_dir(worker.get_work_dir())
{
}

ThreadJob::~ThreadJob()
{

}

void ThreadJob::set_error(const std::string &error_message)
{
    this->error_message = error_message;
}

void ThreadJob::set_redirect(std::unique_ptr<TaskDescription> tredirect)
{
    task_redirect = std::move(tredirect);
}
