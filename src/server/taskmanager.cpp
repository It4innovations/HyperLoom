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

}

void TaskManager::add_plan(const loomplan::Plan &plan, bool distribute)
{
    int tt_size = plan.task_types_size();
    int type_task_translation[tt_size];

    for (int i = 0; i < tt_size; i++) {
        type_task_translation[i] = translate_task_type(plan.task_types(i));
    }

    auto task_size = plan.tasks_size();
    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        tasks[i] = std::make_unique<TaskNode>(
            i, type_task_translation[pt.task_type()], pt.config());
    }

    std::vector<TaskNode*> ready_tasks;

    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        auto& t = tasks[i];
        auto inputs_size = pt.input_ids_size();

        if (inputs_size == 0) {
            ready_tasks.push_back(t.get());
            continue;
        }

        for (int j = 0; j < inputs_size; j++) {
            auto id = pt.input_ids(j);
            auto &task = tasks[id];
            t->add_input(task.get());

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
        results.insert(plan.result_ids(i));
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
    } else {
        llog->debug("Job id={} finished", id);
    }
    std::vector<TaskNode*> ready;
    task.collect_ready_nexts(ready);
    distribute_work(ready);
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

int TaskManager::_translate(std::vector<std::string> &table, const std::string &item)
{
    auto it = std::find(table.begin(), table.end(), item);
    if (it == table.end()) {
        int result = table.size();
        table.push_back(item);
        return result;
    }
    return std::distance(table.begin(), it);
}
