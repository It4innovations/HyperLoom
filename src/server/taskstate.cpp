#include "taskstate.h"
#include "plannode.h"
#include "workerconn.h"

#include <sstream>

TaskState::TaskState(const PlanNode &node)
    : id(node.get_id()), ref_count(node.get_nexts().size())
{
    if (node.is_result()) {
        inc_ref_counter();
    }
}

std::string TaskState::get_info() const
{
   std::stringstream s;
   s << "[TS id=" << id << " size=" << size;
   for (auto &pair : workers) {
      s << " " << pair.first->get_address() << ":";
      s << pair.second;
   }
   s << "]";
   return s.str();
}
