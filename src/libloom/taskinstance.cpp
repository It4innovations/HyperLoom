#include "taskinstance.h"
#include "worker.h"

#include "utils.h"
#include "log.h"

#include <sstream>

using namespace loom;

const std::string TaskInstance::get_task_dir()
{
    std::stringstream s;
    s << worker.get_work_dir() << get_id() << '/';
    std::string name = s.str();
    if (has_directory) {
        return name;
    }

    if (mkdir(name.c_str(), S_IRWXU)) {
        llog->critical("Cannot create directory {}", name);
        log_errno_abort("mkdir");
    }

    has_directory = true;
    return name;
}

void TaskInstance::finish(std::unique_ptr<Data> output)
{
    assert (output->get_id() == get_id());
    worker.publish_data(std::move(output));
    worker.task_finished(*this);
}

void TaskInstance::finish_without_data()
{
    worker.task_finished(*this);
}
