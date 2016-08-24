#include "taskmanager.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/loomplan.pb.h"
#include "libloom/log.h"

#include <algorithm>
#include <limits.h>

using namespace loom;

TaskManager::TaskManager(Server &server)
    : server(server)
{
    auto &dictionary = server.get_dictionary();
    dslice_task_id = dictionary.find_or_create("loom/scheduler/dslice");
    slice_task_id = dictionary.find_or_create("loom/base/slice");
}

static TaskNode::TaskMode read_task_mode(loomplan::Task_Mode mode) {
    switch(mode) {
        case loomplan::Task_Mode_MODE_STANDARD:
            return TaskNode::MODE_STANDARD;
        case loomplan::Task_Mode_MODE_SIMPLE:
            return TaskNode::MODE_SIMPLE;
        case loomplan::Task_Mode_MODE_SCHEDULER:
            return TaskNode::MODE_SCHEDULER;
        default:
            llog->critical("Invalid task mode");
            exit(1);
    }
}

void TaskManager::add_plan(const loomplan::Plan &plan, bool distribute)
{
    auto task_size = plan.tasks_size();
    int id_base = server.new_id(task_size);
    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        auto id = i + id_base;
        tasks[id] = std::make_unique<TaskNode>(
            id, i, read_task_mode(pt.mode()), pt.task_type(), pt.config());
    }

    std::vector<TaskNode*> ready_tasks;

    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        auto& t = tasks[id_base + i];
        auto inputs_size = pt.input_ids_size();

        if (inputs_size == 0) {
            ready_tasks.push_back(t.get());
            continue;
        }

        for (int j = 0; j < inputs_size; j++) {
            auto id = pt.input_ids(j);
            auto &task = tasks[id_base + id];
            t->add_input(task.get());
            task->inc_ref_counter();

            // Put next if there is not such
            int j2;
            for (j2 = 0; j2 < j; j2++) {
               if (id == pt.input_ids(j2)) {
                  break;
               }
            }
            if (j2 == j) {
               task->add_next(t.get());
            }
        }
    }

    for (int i = 0; i < plan.result_ids_size(); i++)
    {
        auto id = plan.result_ids(i);
        assert (0 <= id && id < task_size);
        id = id_base + id;
        tasks[id]->inc_ref_counter();
        results.insert(id);
    }

    for (auto &t : tasks) {
        assert(t.second->get_ref_counter() > 0);
    }

    if (distribute) {
        distribute_work(ready_tasks);
    }
}

void TaskManager::on_task_finished(TaskNode &task)
{
    loom::Id id = task.get_id();
    if (results.find(id) != results.end()) {
        llog->debug("Job id={} [RESULT] finished", id);
        const auto &owners = task.get_owners();
        assert(owners.size());
        owners[0]->send_data(id, server.get_dummy_worker().get_address(), true);

        if (task.dec_ref_counter()) {
            assert(task.get_ref_counter() == 0);
            for (WorkerConnection *owner : task.get_owners()) {
                owner->remove_data(task.get_id());
            }
        }
    } else {
        llog->debug("Job id={} finished (length={})", id, task.get_length());
    }

    for (TaskNode *input : task.get_inputs()) {
        if (input->dec_ref_counter()) {
            assert(input->get_ref_counter() == 0);
            for (WorkerConnection *owner : input->get_owners()) {
                owner->remove_data(input->get_id());
            }
        }
    }

    std::vector<TaskNode*> ready;
    task.collect_ready_nexts(ready);

    bool dslice = false;
    for (TaskNode *node : ready) {
        if (node->get_task_type() == dslice_task_id) {
            dslice = true;
            break;
        }
    }

    if (dslice) {
        std::vector<TaskNode*> new_ready;
        for (TaskNode *node : ready) {
            if (node->get_task_type() == dslice_task_id) {
                expand_dslice(node, new_ready);
            } else {
                new_ready.push_back(node);
            }
        }
        ready = std::move(new_ready);
    }

    distribute_work(ready);
}

void TaskManager::expand_dslice(TaskNode *node, TaskNode::Vector &tasks)
{
    size_t slice_count = 0;
    for (auto &w : server.get_connections()) {
        slice_count += w->get_resource_cpus();
    }
    slice_count *= 4;
    assert (slice_count > 0);

    assert (node->get_inputs().size() == 1); // Todo generalize
    TaskNode *input = node->get_inputs()[0];
    size_t length = input->get_length();


    size_t slice_size = round(static_cast<double>(length) / slice_count);
    if (slice_size == 0) {
        slice_size = 1;
    }

    size_t i = 0;
    slice_count = 0;

    assert (node->get_nexts().size() == 1); // Todo generalize
    TaskNode *next = node->get_nexts()[0];

    if (llog->level() >= spdlog::level::debug) {
        llog->debug("Expanding dslice dslice id={}; follow id={}",
                    node->get_id(), next->get_type_name(server));
    }
    std::vector<TaskNode*> new_tasks;
    while (i < length) {
        loom::Id new_id = server.new_id(2);
        size_t indices[2];
        indices[0] = i;
        indices[1] = i + slice_size;
        if (indices[1] > length) {
            indices[1] = length;
        }
        i = indices[1];
        std::string config(reinterpret_cast<char*>(&indices), sizeof(size_t) * 2);

        auto new_slice = std::make_unique<TaskNode>(new_id, -1, TaskNode::MODE_SIMPLE, slice_task_id, config);
        new_slice->set_inputs(node->get_inputs());
        tasks.push_back(new_slice.get());

        auto new_task = std::make_unique<TaskNode>(
            new_id + 1, -1, next->get_mode(), next->get_task_type(), next->get_config());
        new_task->add_input(new_slice.get());
        new_slice->inc_ref_counter();
        new_tasks.push_back(new_slice.get());
        this->tasks[new_id] = std::move(new_slice);
        slice_count += 1;
    }

    llog->debug("length = {}; slice_count={}; slice_size={}", length, slice_count, slice_size);

    for (TaskNode *n : next->get_nexts()) {
        n->replace_input(next, new_tasks);
    }

    for (TaskNode *n : node->get_inputs()) {
        n->inc_ref_counter(slice_count - 1);
    }
}

struct _TaskInfo {
    int priority;
    TaskNode::Vector new_tasks;
};

TaskManager::WorkDistribution TaskManager::compute_distribution(TaskNode::Vector &tasks)
{
    WorkDistribution distribution;
    auto& connections = server.get_connections();
    if (connections.size() == 0) {
        // There are no workers
        return distribution;
    }

    std::unordered_map<WorkerConnection*, _TaskInfo> map;

    for (auto &connection : connections) {
        int size = connection->get_tasks().size();
        int cpus = connection->get_resource_cpus();
        map[connection.get()].priority = size - cpus;
    }

    for (TaskNode* task : tasks) {
        auto &inputs = task->get_inputs();
        WorkerConnection *found = nullptr;
        int best_priority = INT_MAX;

        for (TaskNode *inp : inputs) {

            auto &owners = inp->get_owners();
            for (WorkerConnection *owner : owners) {
                auto &info = map[owner];
                if (info.priority < best_priority) {
                    best_priority = info.priority;
                    found = owner;
                }
            }
        }

        if (best_priority >= 0) {
            for (auto &i : map) {
                _TaskInfo &info = i.second;
                if (info.priority < best_priority) {
                    best_priority = info.priority;
                    found = i.first;
                }
            }
        }

        auto &info = map[found];
        info.new_tasks.push_back(task);
        info.priority += 1;
    }

    /*std::sort(connections.begin(), connections.end(),
        [](auto &a, auto& b) -> bool
    {
        return a->get_tasks().size() < b->get_tasks().size();
    });*/


    //distribution.reserve(map.size());
    for (auto& pair : map) {
        if (!pair.second.new_tasks.empty()) {
            distribution.emplace_back(WorkerLoad{*pair.first, std::move(pair.second.new_tasks)});
        }
    }
    return distribution;
}

void TaskManager::distribute_work(TaskNode::Vector &tasks)
{
    if (tasks.size() == 0) {
        return;
    }    
    llog->debug("Distributing {} task(s)", tasks.size());

    WorkDistribution distribution = compute_distribution(tasks);
    for (WorkerLoad& load : distribution) {
        for (TaskNode *task : load.tasks) {
            for (TaskNode *input : task->get_inputs()) {
                if (!input->is_owner(load.connection)) {
                    assert(input->get_owners().size() >= 1);
                    WorkerConnection *owner = input->get_owners()[0];
                    owner->send_data(input->get_id(), load.connection.get_address(), false);
                    input->add_owner(&load.connection);
                }
            }
            load.connection.send_task(task);
        }
    }
}
