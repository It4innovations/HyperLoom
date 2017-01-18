
#include "runtask.h"

#include "libloomw/worker.h"
#include "libloomw/data/rawdata.h"
#include "libloomw/data/array.h"
#include "libloom/log.h"
#include "loomrun.pb.h"
#include "libloomw/utils.h"

#include <sstream>
#include <fstream>

using namespace loom;
using namespace loom::base;

RunTask::RunTask(Worker &worker, std::unique_ptr<Task> task)
   : TaskInstance(worker, std::move(task)), exit_status(0)
{    
}

RunTask::~RunTask()
{
}

void RunTask::start(DataVector &inputs)
{
   std::string run_dir = worker.get_run_dir(get_id());
   if (make_path(run_dir.c_str(), S_IRWXU)) {
      logger->critical("Cannot make {}", run_dir);
      log_errno_abort("make_path");
   }

   /* Parse config */
   loomrun::Run msg;
   msg.ParseFromString(task->get_config());

   uv_process_options_t options;
   memset(&options, 0, sizeof(options));
   options.exit_cb = _on_exit;

   auto args_size = msg.args_size();
   if (args_size == 0) {
      fail("No arguments provided");
      return;
   }

   /* Streams */
   uv_stdio_container_t stdio[3];
   options.stdio = stdio;
   stdio[0].flags = UV_IGNORE;
   stdio[1].flags = UV_IGNORE;
   options.stdio_count = 3;

   assert(msg.map_inputs_size() <= static_cast<int>(inputs.size()));

   std::unordered_map<std::string, std::string> variables;

   for (int i = 0; i < msg.map_inputs_size(); i++) {
      const std::string &name = msg.map_inputs(i);

      if (!name.empty() && name[0] == '$') {
         const char *raw_data = inputs[i]->get_raw_data();
         assert(raw_data);
         variables[name] = std::string(raw_data, inputs[i]->get_size());
         continue;
      }

      std::string path = get_path(name);
      std::string filename = inputs[i]->get_filename();
      assert(!filename.empty());
      if (!task->get_inputs().empty()) {
        logger->debug("Creating symlink of '{}' for input id={} filename={}",
                    msg.map_inputs(i), task->get_inputs()[i], filename);
      } else {
          logger->debug("Creating symlink of '{}' for filename={}",
                      msg.map_inputs(i), filename);
      }
      if (symlink(filename.c_str(), path.c_str())) {
         log_errno_abort("symlink");
      }

      /* stdin */
      if (name == "+in") {
         assert(stdio[0].flags == UV_IGNORE);
         int stdin_fd = ::open(path.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
         if (stdin_fd < 0) {
            log_errno_abort("open +in");
         }
         stdio[0].flags = UV_INHERIT_FD;
         stdio[0].data.fd = stdin_fd;
      }
   }


   for (int i = 0; i < msg.map_outputs_size(); i++) {
      if (msg.map_outputs(i) == "+out") {
         assert(stdio[1].flags == UV_IGNORE);
         std::string filename = get_path(msg.map_outputs(i));
         int stdout_fd = ::open(filename.c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
         if (stdout_fd < 0) {
            log_errno_abort("open +out:");
         }
         stdio[1].flags = UV_INHERIT_FD;
         stdio[1].data.fd = stdout_fd;
      }
   }


   /* Setup args */   
   options.file = msg.args(0).c_str();

   char *args[args_size + 1];
   for (int i = 0; i < args_size; i++) {
      const std::string &arg = msg.args(i);
      if (!arg.empty() && arg[0] == '$') { // Variable check
         auto it = variables.find(arg);
         if (it != variables.end()) {
            args[i] = const_cast<char*>(it->second.c_str());
            continue;
         }
      }
      args[i] = const_cast<char*>(arg.c_str());
   }
   args[args_size] = NULL;
   options.args = args;
   std::string work_dir = worker.get_run_dir(get_id());
   options.cwd = work_dir.c_str();

   if (logger->level() == spdlog::level::debug) {
      std::stringstream s;
      s << msg.args(0);
      for (int i = 1; i < args_size; i++) {
         s << ' ' << msg.args(i);
      }
      logger->debug("Running command {}", s.str());
   }


   int stderr_fd = ::open(get_path("+err").c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
   if (stderr_fd < 0) {
      log_errno_abort("open +err:");
   }

   stdio[2].flags = UV_INHERIT_FD;
   stdio[2].data.fd = stderr_fd;

   int r;
   r = uv_spawn(worker.get_loop(), &process, &options);
   process.data = this;

   /* Cleanup */
   if (stdio[0].flags == UV_INHERIT_FD) {
      close(stdio[0].data.fd);
   }
   if (stdio[1].flags == UV_INHERIT_FD) {
      close(stdio[1].data.fd);
   }

   close(stderr_fd);

   if (r) {
       fail_libuv(std::string("spawning '") + options.file + "'", r);
       return;
   }
}

std::string RunTask::get_path(const std::string &filename)
{
   return worker.get_run_dir(get_id()) + filename;
}

void RunTask::_on_exit(uv_process_t *process, int64_t exit_status, int term_signal)
{
   RunTask *task = static_cast<RunTask*>(process->data);
   task->exit_status = exit_status;
   uv_close((uv_handle_t*) process, _on_close);
}

bool RunTask::rename_output(const std::string &output, const std::string &data_path)
{
    std::string path = get_path(output);
    if (unlikely(rename(path.c_str(),
                        data_path.c_str()))) {
       logger->error("Cannot move {} to {}", path, data_path);
       std::stringstream s;
       s << "Output '" << output << "' error: " << strerror(errno);
       s << "\nStderr:\n";
       read_stderr(s);

       fail(s.str());
       return false;
    }
    return true;
}

void RunTask::read_stderr(std::stringstream &s)
{
    std::ifstream err(get_path("+err").c_str());
    assert(err.is_open());

    int max_buffer_size = 128 * 1024;
    auto buffer = std::make_unique<char[]>(max_buffer_size);
    err.read(buffer.get(), max_buffer_size);
    s.write(buffer.get(), err.gcount());
    if (err.gcount() == max_buffer_size) {
        s << "\n ... stderr truncated by Loom";
    }
}

void RunTask::_on_close(uv_handle_t *handle)
{
   RunTask *task = static_cast<RunTask*>(handle->data);
   logger->debug("Process id={} finished (exit_status={})", task->get_id(), task->exit_status);

   if (task->exit_status) {
       std::stringstream s;
       s << "Program terminated with status " << task->exit_status;
       s << "\nStderr:\n";
       task->read_stderr(s);
       task->fail(s.str());
       return;
   }

   loomrun::Run msg;
   msg.ParseFromString(task->task->get_config());
   std::unique_ptr<Data> result;

   int output_size = msg.map_outputs_size();

   if (output_size == 0) {
      *msg.add_map_outputs() = "+out";
   }

   if (output_size == 1) {
      logger->debug("Returning file '{}'' as result", msg.map_outputs(0));
      auto data = std::make_shared<RawData>();
      data->assign_filename(task->worker.get_work_dir());
      if (!task->rename_output(msg.map_outputs(0), data->get_filename())) {
          return;
      }
      data->init_from_file(task->worker.get_work_dir());
      task->finish(data);
      return;
   }


   auto items = std::make_unique<DataPtr[]>(output_size);

   for (int i = 0; i < output_size; i++) {
      logger->debug("Storing file '{}'' as index={}", msg.map_outputs(i), i);
      auto data = std::make_shared<RawData>();
      data->assign_filename(task->worker.get_work_dir());
      if (!task->rename_output(msg.map_outputs(i), data->get_filename())) {
            return;
      }
      data->init_from_file(task->worker.get_work_dir());
      items[i] = std::move(data);
   }
   DataPtr output = std::make_shared<Array>(output_size, std::move(items));
   task->finish(output);
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
        logger->critical("Invalid read in task id={}", task->get_id());
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
