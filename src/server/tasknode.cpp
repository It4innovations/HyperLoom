#include "tasknode.h"
#include "server.h"

#include <sstream>
#include <iomanip>


void TaskNode::replace_input(TaskNode *old_input, const std::vector<TaskNode *> &new_inputs)
{
    auto i = std::find(inputs.begin(), inputs.end(), old_input);
    assert (i != inputs.end());
    auto i2 = inputs.erase(i);
    inputs.insert(i2, new_inputs.begin(), new_inputs.end());

    // Update next in new_inputs
    for (TaskNode *n : new_inputs) {
        n->add_next(this);
    }

    // Update next in old_input
    auto i3 = std::find(old_input->nexts.begin(), old_input->nexts.end(), this);
    assert (i3 != inputs.end());
    old_input->nexts.erase(i3);
}

std::string TaskNode::get_type_name(Server &server)
{
    return server.get_dictionary().translate(task_type);
}

std::string TaskNode::get_info(Server &server)
{
    std::stringstream s;
    s << "T[" << id << "/" << client_id << " refs=" << ref_count;
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
