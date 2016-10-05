#include "taskstate.h"
#include "plannode.h"

TaskState::TaskState(const PlanNode &node)
    : id(node.get_id()), ref_count(node.get_nexts().size())
{
    if (node.is_result()) {
        inc_ref_counter();
    }
}
