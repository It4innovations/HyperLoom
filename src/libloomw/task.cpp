#include "task.h"
#include "worker.h"

using namespace loom;

bool loom::Task::is_ready(const Worker &worker) const
{
    if (n_unresolved > 0) {
        return false;
    } else {
        return true;
    }
}
