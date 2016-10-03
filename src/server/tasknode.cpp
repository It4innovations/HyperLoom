#include "tasknode.h"
#include "server.h"

#include <sstream>
#include <iomanip>


void TaskNode::replace_input(loom::Id old_input, const std::vector<loom::Id> &new_inputs)
{
    auto i = std::find(inputs.begin(), inputs.end(), old_input);
    assert (i != inputs.end());
    auto i2 = inputs.erase(i);
    inputs.insert(i2, new_inputs.begin(), new_inputs.end());
}

void TaskNode::replace_next(loom::Id old_next, const std::vector<loom::Id> &new_nexts)
{
    auto i = std::find(nexts.begin(), nexts.end(), old_next);
    assert (i != nexts.end());
    nexts.erase(i);
    nexts.insert(nexts.end(), new_nexts.begin(), new_nexts.end());
}

std::string TaskNode::get_type_name(Server &server)
{
    return server.get_dictionary().translate(task_type);
}

std::string TaskNode::get_info(Server &server)
{
    std::stringstream s;
    s << "T[" << id << "/" << client_id;
    s << " task=" << get_type_name(server);
    s << " config(" << config.size() << ")=";
    size_t sz = config.size();
    const size_t limit = sz > 8 ? 8 : sz;
    for (size_t i = 0; i < limit; i++) {
        s << std::hex << std::setfill('0') << std::setw(2) << (int) config[i];
    }
    if (sz > limit) {
        s << "...";
    }
    s << "]";
    return s.str();
}
