
#include "tasknode.h"
#include "workerconn.h"
#include "libloom/log.h"

#include <sstream>

TaskNode::TaskNode(loom::base::Id id, TaskDef &&task)
    : id(id),
      task(std::move(task)),
      size(0),
      length(0),
      remaining_inputs(0)
{

}

void TaskNode::reset_result_flag()
{
   assert(is_result());
   task.flags.reset(static_cast<size_t>(TaskDefFlags::RESULT));
}

WorkerConnection *TaskNode::get_random_owner()
{
    for(auto &pair : workers) {
        if (pair.second == TaskStatus::OWNER) {
            return pair.first;
        }
    }
    return nullptr;
}

bool TaskNode::is_active() const
{
    for (auto &pair : workers) {
        if (pair.second == TaskStatus::RUNNING || pair.second == TaskStatus::TRANSFER) {
            return true;
        }
    }
    return false;
}

void TaskNode::reset_owners()
{
    for (auto &pair : workers) {
        if (pair.second == TaskStatus::OWNER) {
            pair.second = TaskStatus::NONE;
        }
    }
}

bool TaskNode::next_finished(TaskNode &node)
{
    auto it = nexts.find(&node);
    assert(it != nexts.end());
    nexts.erase(it);
    return nexts.empty();
}

void TaskNode::set_as_none(WorkerConnection *wc)
{
    auto status = get_worker_status(wc);
    if (status == TaskStatus::RUNNING) {
        wc->add_free_cpus(get_n_cpus());
    }
    set_worker_status(wc, TaskStatus::NONE);
}

void TaskNode::set_as_finished(WorkerConnection *wc, size_t size, size_t length)
{
    assert(get_worker_status(wc) == TaskStatus::RUNNING);
    wc->free_resources(*this);
    set_worker_status(wc, TaskStatus::OWNER);
    this->size = size;
    this->length = length;
    flags.set(static_cast<size_t>(TaskNodeFlags::FINISHED));
}

void TaskNode::set_as_loaded(WorkerConnection *wc, size_t size, size_t length) {
    assert(get_worker_status(wc) == TaskStatus::LOADING);
    set_worker_status(wc, TaskStatus::OWNER);
    this->size = size;
    this->length = length;
    flags.set(static_cast<size_t>(TaskNodeFlags::FINISHED));
}

void TaskNode::set_as_loading(WorkerConnection *wc) {
    set_worker_status(wc, TaskStatus::LOADING);
}

void TaskNode::set_as_finished_no_check(WorkerConnection *wc, size_t size, size_t length)
{
    set_worker_status(wc, TaskStatus::OWNER);
    this->size = size;
    this->length = length;
    flags.set(static_cast<size_t>(TaskNodeFlags::FINISHED));
}

std::string TaskNode::debug_str() const
{
   std::stringstream s;
   s << "<Node id=" << id;
   for(auto &pair : workers) {
      s << ' ' << pair.first->get_address() << ':' << static_cast<int>(pair.second);
   }
   s << '>';
   return s.str();
}

bool TaskNode::_slow_is_ready() const
{
   for (TaskNode *input_node : task.inputs) {
      if (!input_node->is_computed()) {
           return false;
      }
   }
   return true;
}

void TaskNode::set_as_running(WorkerConnection *wc)
{
    assert(get_worker_status(wc) == TaskStatus::NONE);
    set_worker_status(wc, TaskStatus::RUNNING);
    wc->remove_free_cpus(get_n_cpus());
}

void TaskNode::set_as_transferred(WorkerConnection *wc)
{
    auto &s = workers[wc];
    assert(s == TaskStatus::TRANSFER);
    s = TaskStatus::OWNER;
}
