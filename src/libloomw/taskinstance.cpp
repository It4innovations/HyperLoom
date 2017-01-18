#include "taskinstance.h"
#include "worker.h"
#include "utils.h"

#include "libloom/log.h"

#include <sstream>

using namespace loom;
using namespace loom::base;

TaskInstance::~TaskInstance()
{

}

const std::string TaskInstance::get_task_dir()
{
    std::stringstream s;
    s << worker.get_work_dir() << get_id() << '/';
    std::string name = s.str();
    if (has_directory) {
        return name;
    }

    if (mkdir(name.c_str(), S_IRWXU)) {
        logger->critical("Cannot create directory {}", name);
        log_errno_abort("mkdir");
    }

    has_directory = true;
    return name;
}

void TaskInstance::fail(const std::string &error_msg)
{
    worker.task_failed(*this, error_msg);
}

void TaskInstance::fail_libuv(const std::string &error_msg, int error_code)
{
    std::stringstream s;
    s << error_msg << ": " << uv_strerror(error_code);
    fail(s.str());
}

void TaskInstance::finish(const DataPtr &output)
{
   assert(output);
   worker.publish_data(get_id(), output);
   assert(output);
   worker.task_finished(*this, output);
}

void TaskInstance::redirect(std::unique_ptr<TaskDescription> tdesc)
{
    worker.task_redirect(*this, std::move(tdesc));
}