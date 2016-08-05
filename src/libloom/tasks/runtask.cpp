
#include "runtask.h"

#include "libloom/worker.h"
#include "libloom/data/rawdata.h"
#include "libloom/data/array.h"
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

   assert(msg.map_inputs_size() <= static_cast<int>(inputs.size()));

   for (int i = 0; i < msg.map_inputs_size(); i++) {
      auto& input = *inputs[i];
      std::string path = get_path(msg.map_inputs(i));
      std::string filename = input->get_filename();
      assert(!filename.empty());
      llog->debug("Creating symlink of '{}'", msg.map_inputs(i));
      if (symlink(filename.c_str(), path.c_str())) {
         log_errno_abort("symlink");
      }

      /* stdin */
      if (msg.map_inputs(i) == "+in") {
         assert(stdio[0].flags == UV_IGNORE);
         int stdin_fd = ::open(path.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
         if (stdin_fd < 0) {
            log_errno_abort("open +in:");
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

   if (r) {
       fail_libuv("uv_spawn", r);
       return;
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

   int output_size = msg.map_outputs_size();

   if (output_size == 0) {
      *msg.add_map_outputs() = "+out";
   }

   if (output_size == 1) {
      std::shared_ptr<Data> data_ptr = std::make_shared<RawData>();
      RawData& data = static_cast<RawData&>(*data_ptr);
      data.assign_filename(task->worker);

      std::string path = task->get_path(msg.map_outputs(0));
      std::string data_path = data.get_filename();
      llog->debug("Returning file '{}'' as result", msg.map_outputs(0));
      if (unlikely(rename(path.c_str(),
                          data_path.c_str()))) {
         llog->critical("Cannot move {} to {}",
                        path, data_path);
         log_errno_abort("rename");
      }
      data.init_from_file(task->worker);
      task->finish(data_ptr);
      return;
   }


   auto items = std::make_unique<std::shared_ptr<Data>[]>(output_size);

   for (int i = 0; i < output_size; i++) {

      items[i] = std::make_shared<RawData>();
      RawData& data = static_cast<RawData&>(*items[i]);
      data.assign_filename(task->worker);
      std::string path = task->get_path(msg.map_outputs(i));
      std::string data_path = data.get_filename();
      llog->debug("Storing file '{}'' as index={}", msg.map_outputs(i), i);
      //data->create(task->worker, 10);
      if (unlikely(rename(path.c_str(),
                          data_path.c_str()))) {
         llog->critical("Cannot move {} to {}",
                        path, data_path);
         log_errno_abort("rename");
      }
      data.init_from_file(task->worker);
   }

   std::shared_ptr<Data> output = std::make_shared<Array>(output_size, std::move(items));
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