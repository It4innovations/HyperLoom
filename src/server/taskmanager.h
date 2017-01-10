#ifndef LOOM_SERVER_TASKMANAGER_H
#define LOOM_SERVER_TASKMANAGER_H

#include "compstate.h"
#include "scheduler.h"

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

    void add_plan(Plan &&plan, bool report);

    const Plan& get_plan() const {
        return cstate.get_plan();
    }

    void on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc);
    void on_data_transfered(loom::base::Id id, WorkerConnection *wc);
    void register_worker(WorkerConnection *wc);

    bool is_plan_finished() const {
        return cstate.is_finished();
    }

    int get_n_of_data_objects() const {
        return cstate.get_n_data_objects();
    }

    void run_task_distribution();

private:    
    Server &server;
    ComputationState cstate;

    void distribute_work(const TaskDistribution &distribution);
    void start_task(WorkerConnection *wc, loom::base::Id task_id);
    void remove_state(TaskState &state);

    bool report;
    void report_task_start(WorkerConnection *wc, const PlanNode &node);
    void report_task_end(WorkerConnection *wc, const PlanNode &node);
};


#endif // LOOM_SERVER_TASKMANAGER_H
