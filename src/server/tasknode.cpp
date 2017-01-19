
#include "tasknode.h"
#include "workerconn.h"
#include "libloom/log.h"

TaskNode::TaskNode(loom::base::Id id, loom::base::Id client_id, TaskDef &&task)
    : id(id), task(std::move(task)), client_id(client_id)
{

}

bool TaskNode::is_computed() const {
    if (!state) {
        return false;
    }
    for(auto &pair : state->workers) {
        if (pair.second == TaskStatus::OWNER) {
            return true;
        }
    }
    return false;
}

WorkerConnection *TaskNode::get_random_owner()
{
    for(auto &pair : state->workers) {
        if (pair.second == TaskStatus::OWNER) {
            return pair.first;
        }
    }
    return nullptr;
}

void TaskNode::create_state()
{
    assert(!state);
    state = std::make_unique<DataState>();
    state->size = 0;
    state->length = 0;
}

bool TaskNode::is_active() const
{
    if (!state) {
        return false;
    }
    for (auto &pair : state->workers) {
        if (pair.second == TaskStatus::RUNNING || pair.second == TaskStatus::TRANSFER) {
            return true;
        }
    }
    return false;
}

void TaskNode::reset_owners()
{
    for (auto &pair : state->workers) {
        if (pair.second == TaskStatus::OWNER) {
            pair.second = TaskStatus::NONE;
        }
    }
}

bool TaskNode::next_finished(TaskNode &node)
{
    size_t index = 0;
    for (TaskNode *next_node : nexts) {
        if (next_node == &node) {
            nexts[index] = nexts[nexts.size() - 1];
            nexts.pop_back();
            return nexts.empty();
        }
        index++;
    }
    assert(0);
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
    wc->add_free_cpus(get_n_cpus());
    set_worker_status(wc, TaskStatus::OWNER);
    state->size = size;
    state->length = length;
}

void TaskNode::set_as_finished_no_check(WorkerConnection *wc, size_t size, size_t length)
{
    set_worker_status(wc, TaskStatus::OWNER);
    state->size = size;
    state->length = length;
}

void TaskNode::set_as_running(WorkerConnection *wc)
{    
    assert(get_worker_status(wc) == TaskStatus::NONE);
    set_worker_status(wc, TaskStatus::RUNNING);
    wc->remove_free_cpus(get_n_cpus());
}

void TaskNode::set_as_transferred(WorkerConnection *wc)
{
    assert(state);
    auto &s = state->workers[wc];
    assert(s == TaskStatus::TRANSFER);
    s = TaskStatus::OWNER;
}
