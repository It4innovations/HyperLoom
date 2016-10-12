#ifndef LOOM_SERVER_COMPSTATE_H
#define LOOM_SERVER_COMPSTATE_H

#include "plan.h"
#include "taskstate.h"

class Server;

using TaskDistribution = std::unordered_map<WorkerConnection*, std::vector<loom::Id>>;

class ComputationState {
    struct WorkerInfo {
        WorkerInfo() : n_tasks(0) {}
        size_t index;
        int n_tasks;
    };
public:
    ComputationState(Server &server);

    void set_plan(Plan &&plan);
    void add_worker(WorkerConnection* wc);

    void set_running_task(WorkerConnection *wc, loom::Id id);

    TaskDistribution compute_initial_distribution();
    TaskDistribution compute_distribution();

    TaskState& get_state(loom::Id id);

    TaskState* get_state_ptr(loom::Id id) {
        auto it = states.find(id);
        if (it != states.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    //TaskState& get_state_or_create(loom::Id id);

    const PlanNode& get_node(loom::Id id) {
        return plan.get_node(id);
    }

    void add_ready_nodes(const std::vector<loom::Id> &ids);
    void set_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc);

    const Plan& get_plan() const {
        return plan;
    }

    void remove_state(loom::Id id);

    bool is_ready(const PlanNode &node);
    void add_ready_nexts(const PlanNode &node);
    bool is_finished() const;

private:
    std::unordered_map<loom::Id, TaskState> states;
    std::unordered_map<WorkerConnection*, WorkerInfo> workers;
    std::unordered_set<loom::Id> pending_tasks;
    Plan plan;
    Server &server;

    loom::Id dslice_task_id;
    loom::Id dget_task_id;

    loom::Id slice_task_id;
    loom::Id get_task_id;

    WorkerConnection *get_best_holder_of_deps(PlanNode *task);
    WorkerConnection *find_best_worker_for_node(PlanNode *task);

    void expand_node(const PlanNode &node);
    void expand_dslice(const PlanNode &node);
    void expand_dget(const PlanNode &node);
    void make_expansion(std::vector<std::string> &configs, const PlanNode &node1,
                        const PlanNode &node2, loom::Id id_base1, loom::Id id_base2);
    void collect_requirements_for_node(WorkerConnection *wc,
                                       const PlanNode &node,
                                       std::unordered_set<loom::Id> &nonlocals);
    size_t task_transfer_cost(const PlanNode &node);
    void add_pending_task(loom::Id id);
    int get_max_cpus();
};


#endif // LOOM_SERVER_COMPSTATE_H
