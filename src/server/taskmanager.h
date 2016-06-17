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

    loom::TaskId translate_task_type(const std::string &item) {
        return _translate(task_types, item);
    }

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

    void distribute_work(TaskNode::Vector &tasks);

    static int _translate(std::vector<std::string> &table, const std::string &item);
};


#endif // LOOM_SERVER_TASKMANAGER_H
