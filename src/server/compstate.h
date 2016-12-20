#ifndef LOOM_SERVER_COMPSTATE_H
#define LOOM_SERVER_COMPSTATE_H

#include "plan.h"
#include "taskstate.h"

class Server;

class ComputationState {
public:

    struct WorkerInfo {
        int free_cpus;
    };

    ComputationState(Server &server);

    void set_plan(Plan &&plan);
    void add_worker(WorkerConnection* wc);

    void set_running_task(const PlanNode &node, WorkerConnection *wc);

    /*TaskDistribution compute_initial_distribution();
    TaskDistribution compute_distribution();*/

    TaskState& get_state(loom::base::Id id);

    TaskState* get_state_ptr(loom::base::Id id) {
        auto it = states.find(id);
        if (it != states.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    //TaskState& get_state_or_create(loom::base::Id id);

    const PlanNode& get_node(loom::base::Id id) {
        return plan.get_node(id);
    }

    void add_ready_nodes(const std::vector<loom::base::Id> &ids);
    void set_task_finished(const PlanNode& node, size_t size, size_t length, WorkerConnection *wc);

    bool has_pending_tasks() const {
        return !pending_tasks.empty();
    }

    const std::unordered_set<loom::base::Id>& get_pending_tasks() const {
        return pending_tasks;
    }

    const Plan& get_plan() const {
        return plan;
    }

    void remove_state(loom::base::Id id);

    bool is_ready(const PlanNode &node);
    void add_ready_nexts(const PlanNode &node);
    bool is_finished() const;

    uint64_t get_base_time() const {
        return base_time;
    }

    const std::unordered_map<WorkerConnection*, WorkerInfo>&
    get_worker_info() const {
        return workers;
    }

private:
    std::unordered_map<loom::base::Id, TaskState> states;
    std::unordered_map<WorkerConnection*, WorkerInfo> workers;
    std::unordered_set<loom::base::Id> pending_tasks;
    Plan plan;
    Server &server;

    uint64_t base_time;

    loom::base::Id dslice_task_id;
    loom::base::Id dget_task_id;

    loom::base::Id slice_task_id;
    loom::base::Id get_task_id;


    WorkerConnection *get_best_holder_of_deps(PlanNode *task);
    WorkerConnection *find_best_worker_for_node(PlanNode *task);

    void expand_node(const PlanNode &node);
    void expand_dslice(const PlanNode &node);
    void expand_dget(const PlanNode &node);
    void make_expansion(std::vector<std::string> &configs, const PlanNode &node1,
                        const PlanNode &node2, loom::base::Id id_base1, loom::base::Id id_base2);
    void collect_requirements_for_node(WorkerConnection *wc,
                                       const PlanNode &node,
                                       std::unordered_set<loom::base::Id> &nonlocals);
    size_t task_transfer_cost(const PlanNode &node);
    void add_pending_task(loom::base::Id id);
    int get_max_cpus();
};


#endif // LOOM_SERVER_COMPSTATE_H
