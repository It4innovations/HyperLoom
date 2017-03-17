#ifndef LOOM_SERVER_TASKMANAGER_H
#define LOOM_SERVER_TASKMANAGER_H

#include "compstate.h"
#include "scheduler.h"

#include <vector>
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

    loom::base::Id add_plan(const loom::pb::comm::Plan &plan);

    void on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc);
    void on_data_transferred(loom::base::Id id, WorkerConnection *wc);
    void on_task_failed(loom::base::Id id, WorkerConnection *wc, const std::string &error_msg);

    int get_n_of_data_objects() const {
        return cstate.get_n_data_objects();
    }

    TaskNode* get_node_ptr(loom::base::Id id) {
        return cstate.get_node_ptr(id);
    }

    void run_task_distribution();

    void trash_all_tasks();
    void release_node(TaskNode *node);


private:
    Server &server;
    ComputationState cstate;    

    void distribute_work(const TaskDistribution &distribution);
    void start_task(WorkerConnection *wc, TaskNode &node);
    void remove_node(TaskNode &node);
};


#endif // LOOM_SERVER_TASKMANAGER_H
