#ifndef LIBLOOM_TASKS_RUNTASK_H
#define LIBLOOM_TASKS_RUNTASK_H

#include "libloomw/taskinstance.h"

namespace loom {

class RunTask : public loom::TaskInstance
{
public:
    RunTask(loom::Worker &worker, std::unique_ptr<loom::Task> task);
    ~RunTask();
    void start(loom::DataVector &inputs);


private:

    std::string get_path(const std::string &filename);

    std::shared_ptr<loom::Data> input;

    uv_process_t process;
    uv_pipe_t pipes[2];
    uv_write_t write_request;
    int64_t exit_status;

    static void _on_exit(uv_process_t *process, int64_t exit_status, int term_signal);
    static void _on_close(uv_handle_t *handle);
};

}

#endif // LIBLOOM_TASKS_RUNTASK_H
