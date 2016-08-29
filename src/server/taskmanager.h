#ifndef LOOM_SERVER_TASKMANAGER_H
#define LOOM_SERVER_TASKMANAGER_H

#include "tasknode.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>

namespace loomplan {
class Plan;
}

class Server;

/** This class is responsible for planning tasks on workers */
class TaskManager
{

public:

    struct WorkerLoad {
        WorkerLoad(WorkerConnection &connection, TaskNode::Vector &&tasks)
            : connection(connection), tasks(tasks) {}

        WorkerConnection &connection;
        TaskNode::Vector tasks;
    };
    typedef std::vector<WorkerLoad> WorkDistribution;

    TaskManager(Server &server);

    void add_plan(const loomplan::Plan &plan, bool distribute=true);

    void on_task_finished(TaskNode &task);

    WorkDistribution compute_distribution(TaskNode::Vector &tasks);

    TaskNode& get_task(loom::Id id) {
        return *tasks[id];
    }


private:    
    Server &server;
    std::unordered_map<loom::Id, std::unique_ptr<TaskNode>> tasks;
    std::unordered_set<loom::Id> results;
    std::vector<std::string> task_types;

    loom::Id dslice_task_id;
    loom::Id dget_task_id;

    loom::Id slice_task_id;
    loom::Id get_task_id;



    void distribute_work(TaskNode::Vector &tasks);
    void expand_scheduler_task(TaskNode *node, TaskNode::Vector &tasks);
    void expand_dslice(TaskNode *node, TaskNode::Vector &tasks, size_t length, TaskNode *next);
    void expand_dget(TaskNode *node, TaskNode::Vector &tasks, size_t length, TaskNode *next);
    void dynamic_expand_helper(
            const TaskNode::Vector &inputs,
            TaskNode *next,
            loom::Id task_type_id,
            std::string &config,
            TaskNode::Vector &tasks1,
            TaskNode::Vector &tasks2);
};


#endif // LOOM_SERVER_TASKMANAGER_H
