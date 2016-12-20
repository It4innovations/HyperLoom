#include "task.h"
#include "worker.h"

using namespace loom;

bool loom::Task::is_ready(const Worker &worker) const
{
    for (auto id : inputs) {
        if (!worker.has_data(id)) {
            return false;
        }
    }
    return true;
}
