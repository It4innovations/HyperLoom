#ifndef LOOM_SERVER_COMPSTATE_H
#define LOOM_SERVER_COMPSTATE_H

#include "tasknode.h"

#include <unordered_set>

namespace loomplan {
class Plan;
}

class Server;

class ComputationState {
public:

    ComputationState(Server &server);

    void add_node(std::unique_ptr<TaskNode> &&node);
    void set_final_node(loom::base::Id id);
    void add_worker(WorkerConnection* wc);

    void reserve_new_nodes(size_t size);

    TaskNode* get_node_ptr(loom::base::Id id);
    TaskNode& get_node(loom::base::Id id);
    const TaskNode& get_node(loom::base::Id id) const;

    void add_ready_nexts(const TaskNode &node);

    bool has_pending_nodes() const {
        return !pending_nodes.empty();
    }

    const std::unordered_set<TaskNode*>& get_pending_tasks() const {
        return pending_nodes;
    }

    void activate_pending_node(TaskNode &node, WorkerConnection *wc);

    void remove_node(TaskNode &node);

    bool is_ready(const TaskNode &node);
    bool is_finished() const;

    uint64_t get_base_time() const {
        return base_time;
    }

    bool is_result(loom::base::Id id) const {
        return final_nodes.find(id) != final_nodes.end();
    }

    int get_n_data_objects() const;

    void add_plan(const loomplan::Plan &plan);
    void test_ready_nodes(std::vector<loom::base::Id> ids);

    loom::base::Id pop_result_client_id(loom::base::Id id);

    Server& get_server() const {
        return server;
    }

    template<typename F> void foreach_node(const F &f) {
        for (auto &pair : nodes) {
            f(pair.second);
        }
    }

    std::unique_ptr<TaskNode> pop_node(loom::base::Id id);
    void clear_all();

private:    

    std::unordered_map<loom::base::Id, std::unique_ptr<TaskNode>> nodes;
    std::unordered_set<TaskNode*> pending_nodes;
    std::unordered_map<loom::base::Id, loom::base::Id> final_nodes; // task_id -> client_id

    Server &server;

    uint64_t base_time;

    loom::base::Id dslice_task_id;
    loom::base::Id dget_task_id;

    loom::base::Id slice_task_id;
    loom::base::Id get_task_id;


    /*void expand_node(const PlanNode &node);
    void expand_dslice(const PlanNode &node);
    void expand_dget(const PlanNode &node);
    void make_expansion(std::vector<std::string> &configs, const PlanNode &node1,
                        const PlanNode &node2, loom::base::Id id_base1, loom::base::Id id_base2);*/
    /*void collect_requirements_for_node(WorkerConnection *wc,
                                       const PlanNode &node,
                                       std::unordered_set<loom::base::Id> &nonlocals);*/
    int get_max_cpus();
    void add_pending_node(TaskNode &node);
};


#endif // LOOM_SERVER_COMPSTATE_H
