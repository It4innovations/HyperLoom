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

    TaskManager(Server &server);

    /*void add_node(TaskNode &&node) {
        cstate.add_node(std::move(node));
    }*/

    void set_final_node(loom::base::Id id) {
        cstate.set_final_node(id);
    }

    void add_plan(const loomplan::Plan &plan);

    void set_report(bool report) {
        this->report = report;
    }

    void on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc);
    void on_data_transferred(loom::base::Id id, WorkerConnection *wc);
    void on_task_failed(loom::base::Id id, WorkerConnection *wc, const std::string &error_msg);

    bool is_plan_finished() const {
        return cstate.is_finished();
    }

    int get_n_of_data_objects() const {
        return cstate.get_n_data_objects();
    }

    void run_task_distribution();

    loom::base::Id pop_result_client_id(loom::base::Id id) {
        return cstate.pop_result_client_id(id);
    }

    void trash_all_tasks();

    bool check_trash(loom::base::Id id, WorkerConnection *wc, bool success);

private:
    Server &server;
    ComputationState cstate;
    std::unordered_map<loom::base::Id, std::unique_ptr<TaskNode>> trash;
    bool report;

    void distribute_work(const TaskDistribution &distribution);
    void start_task(WorkerConnection *wc, TaskNode &node);
    void remove_node(TaskNode &node);

    void report_task_start(WorkerConnection *wc, const TaskNode &node);
    void report_task_end(WorkerConnection *wc, const TaskNode &node);
};


#endif // LOOM_SERVER_TASKMANAGER_H
