#include "task.h"
#include "worker.h"

using namespace loom;

bool Task::is_ready(base::Id finished_id)
{
    if (unresolved_set.find(finished_id) != unresolved_set.end()) {
        assert(n_unresolved > 0);
        n_unresolved--;
        if (n_unresolved == 0) {
            // Let's clean the unresolved set, task may live for a while after this step
            unresolved_set = std::unordered_set<base::Id>();
            return true;
        }
    }
    return false;
}

void Task::set_unresolved_set(std::unordered_set<base::Id> &&set)
{
    assert(n_unresolved == 0 && unresolved_set.empty());
    n_unresolved = set.size();
    unresolved_set = std::move(set);
}
