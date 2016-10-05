#ifndef LOOM_SERVER_TASKMANAGER_H
#define LOOM_SERVER_TASKMANAGER_H

#include "compstate.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>

class Server;
class WorkerConnection;

/** This class is responsible for planning tasks on workers */
class TaskManager
{

public:

    struct WorkerLoad {
        WorkerLoad(WorkerConnection &connection, PlanNode::Vector &&tasks)
            : connection(connection), tasks(tasks) {}

        WorkerConnection &connection;
        PlanNode::Vector tasks;
    };
    typedef std::vector<WorkerLoad> WorkDistribution;

    TaskManager(Server &server);

    void add_plan(Plan &&plan);

    const Plan& get_plan() const {
        return cstate.get_plan();
    }

    void on_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc);
    void register_worker(WorkerConnection *wc);

private:    
    Server &server;
    ComputationState cstate;

    void distribute_work(TaskDistribution &distribution);
    void start_task(WorkerConnection *wc, loom::Id task_id);
    void remove_state(TaskState &state);
};


#endif // LOOM_SERVER_TASKMANAGER_H
