
#include "runtask.h"

#include "libloom/worker.h"
#include "libloom/data/rawdata.h"
#include "libloom/log.h"
#include "loomrun.pb.h"

#include <sstream>

using namespace loom;

RunTask::RunTask(Worker &worker, std::unique_ptr<Task> task)
    : TaskInstance(worker, std::move(task))
{    
}

RunTask::~RunTask()
{
}

void RunTask::start(DataVector &inputs)
{
    std::string run_dir = worker.get_run_dir(get_id());
    if (make_path(run_dir.c_str(), S_IRWXU)) {
        llog->critical("Cannot make {}", run_dir);
        log_errno_abort("make_path");
    }

    /* Parse config */
    loomrun::Run msg;
    msg.ParseFromString(task->get_config());

    uv_process_options_t options;
    memset(&options, 0, sizeof(options));
    options.exit_cb = _on_exit;

    /* Setup args */
    auto args_size = msg.args_size();
    assert(args_size > 0);
    options.file = msg.args(0).c_str();

    char *args[args_size + 1];
    for (int i = 0; i < args_size; i++) {
        args[i] = const_cast<char*>(msg.args(i).c_str());
    }
    args[args_size] = NULL;
    options.args = args;
    std::string work_dir = worker.get_run_dir(get_id());
    options.cwd = work_dir.c_str();

    if (llog->level() == spdlog::level::debug) {
        std::stringstream s;
        s << msg.args(0);
        for (int i = 1; i < args_size; i++) {
            s << ' ' << msg.args(i);
        }
        llog->debug("Running command {}", s.str());
    }

    /* Streams */
    uv_stdio_container_t stdio[2];
    options.stdio = stdio;
    stdio[0].flags = UV_IGNORE;
    stdio[1].flags = UV_IGNORE;
    options.stdio_count = 2;


    int input_size = inputs.size();
    for (int i = 0; i < msg.maps_size(); i++) {
        auto& map = msg.maps(i);

        if (map.input_index() != -2) {
            assert(map.input_index() >= 0 && map.input_index() < input_size);
            auto& input = *inputs[map.input_index()];
            std::string path = get_path(map.filename());
            std::string filename = input->get_filename(worker);
            llog->alert("FILENAME = {} {}", filename, (unsigned long) &input);
            assert(!filename.empty());
            llog->debug("Creating symlink of '{}'", map.filename());
            if (symlink(filename.c_str(), path.c_str())) {
                log_errno_abort("symlink");
            }

            /* stdin */
            if (map.filename() == "+in") {
                assert(stdio[0].flags == UV_IGNORE);
                int stdin_fd = ::open(path.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
                if (stdin_fd < 0) {
                    log_errno_abort("open +in:");
                }
                stdio[0].flags = UV_INHERIT_FD;
                stdio[0].data.fd = stdin_fd;
            }
        }

        /* stdout */
        if (map.output_index() != -2 && map.filename() == "+out") {
            assert(stdio[1].flags == UV_IGNORE);
            std::string filename = get_path(map.filename());
            int stdout_fd = ::open(filename.c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
            if (stdout_fd < 0) {
                log_errno_abort("open +out:");
            }
            stdio[1].flags = UV_INHERIT_FD;
            stdio[1].data.fd = stdout_fd;
        }
    }


    UV_CHECK(uv_spawn(worker.get_loop(), &process, &options));
    process.data = this;

    /*    if (stdin_index >= 0) {
        input = *inputs[stdin_index];
        uv_buf_t buf = input->get_uv_buf(worker);
        UV_CHECK(uv_write(&write_request, (uv_stream_t*) &pipes[0], &buf, 1, _on_write));
        write_request.data = this;
    } else {
        ref_counter -= 1;
    }

    UV_CHECK(uv_read_start((uv_stream_t*) &pipes[1], _buf_alloc, _on_read));*/

    /* Cleanup */
    if (stdio[0].flags == UV_INHERIT_FD) {
        close(stdio[0].data.fd);
    }
    if (stdio[1].flags == UV_INHERIT_FD) {
        close(stdio[1].data.fd);
    }
}

std::string RunTask::get_path(const std::string &filename)
{
    return worker.get_run_dir(get_id()) + filename;
}

void RunTask::_on_exit(uv_process_t *process, int64_t exit_status, int term_signal)
{
    //RunTask *task = static_cast<RunTask*>(process->data);
    assert(exit_status == 0);
    uv_close((uv_handle_t*) process, _on_close);
}

void RunTask::_on_close(uv_handle_t *handle)
{
    RunTask *task = static_cast<RunTask*>(handle->data);
    llog->debug("Process id={} finished.", task->get_id());
    loomrun::Run msg;
    msg.ParseFromString(task->task->get_config());
    std::unique_ptr<Data> result;
    for (int i = 0; i < msg.maps_size(); i++) {
        auto& map = msg.maps(i);
        int index = map.output_index();
        if (index == -2) {
            continue;
        }
        auto data = std::make_unique<RawData>();
        data->assign_file_id();

        std::string path = task->get_path(map.filename());
        std::string data_path = data->get_filename(task->worker);
        llog->debug("Storing file '{}'' as index={}", map.filename(), i);
        //data->create(task->worker, 10);
        if (unlikely(rename(path.c_str(),
                            data_path.c_str()))) {
            llog->critical("Cannot move {} to {}",
                           path, data_path);
            log_errno_abort("rename");
        }
        data->init_from_file(task->worker);
        task->finish(std::move(data));
        return;
    }
}

/*
void RunTask::_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    RunTask *task = static_cast<RunTask*>(stream->data);
    if (nread == UV_EOF) {
        if (buf->base) {
            delete[] buf->base;
        }
        uv_close((uv_handle_t*)stream, _on_close);
        return;
    }
    if (nread < 0) {
        llog->critical("Invalid read in task id={}", task->get_id());
        exit(1);
    }

    if (buf->base) {
        task->output->add(buf->base, nread);
        delete[] buf->base;
    }
}


void RunTask::_buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

void RunTask::_on_write(uv_write_t *req, int status)
{
    UV_CHECK(status);
    uv_close((uv_handle_t*) req->handle, _on_close);
}
*/
