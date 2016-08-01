#ifndef LIBLOOM_TASKS_RUNTASK_H
#define LIBLOOM_TASKS_RUNTASK_H

#include "libloom/taskinstance.h"

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

    static void _on_exit(uv_process_t *process, int64_t exit_status, int term_signal);
    /*static void _on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void _buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void _on_write(uv_write_t* req, int status);*/
    static void _on_close(uv_handle_t *handle);
};


#endif // LIBLOOM_TASKS_RUNTASK_H
