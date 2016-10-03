#ifndef LOOM_SERVER_COMPSTATE_H
#define LOOM_SERVER_COMPSTATE_H

#include "plan.h"

#include <unordered_map>

class WorkerConnection;
template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;
class ComputationState;
class Server;

class TaskState {

public:
    enum WStatus {
        NONE,
        READY,
        RUNNING,
        OWNER,
    };

    TaskState(const TaskNode &node);

    loom::Id get_id() const {
        return id;
    }

    void inc_ref_counter(int count = 1) {
        ref_count += count;
    }

    bool dec_ref_counter() {
        return --ref_count <= 0;
    }

    int get_ref_counter() const {
        return ref_count;
    }

    WorkerMap<WStatus>& get_workers() {
        return workers;
    }

    size_t get_size() const {
        return size;
    }

    void set_size(size_t size) {
        this->size = size;
    }

    size_t get_length() const {
        return length;
    }

    void set_length(size_t length) {
        this->length = length;
    }

    WStatus get_worker_status(WorkerConnection *wc) {
        auto i = workers.find(wc);
        if (i == workers.end()) {
            return NONE;
        }
        return i->second;
    }

    WorkerConnection *get_first_owner() {
        for (auto &p : workers) {
            if (p.second == OWNER) {
                return p.first;
            }
        }
        return nullptr;
    }

    void set_worker_status(WorkerConnection *wc, WStatus ws) {
        workers[wc] = ws;
    }

    template<typename F> void foreach_owner(F f) {
        for(auto &pair : workers) {
            if (pair.second == OWNER) {
                f(pair.first);
            }
        }
    }

private:
    loom::Id id;
    WorkerMap<WStatus> workers;
    int ref_count;
    size_t size;
    size_t length;
};

struct WorkerInfo {
    WorkerInfo() : n_tasks(0) {}
    int n_tasks;
};

using TaskDistribution = std::unordered_map<WorkerConnection*, std::vector<loom::Id>>;

class ComputationState {
public:
    ComputationState(Server &server);

    void set_plan(Plan &&plan);
    void add_worker(WorkerConnection* wc);

    void set_running_task(WorkerConnection *wc, loom::Id id);

    TaskDistribution compute_initial_distribution();
    TaskDistribution compute_distribution();

    TaskState& get_state(loom::Id id) {
        auto it = states.find(id);
        assert(it != states.end());
        return it->second;
    }

    TaskState* get_state_ptr(loom::Id id) {
        auto it = states.find(id);
        if (it != states.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    TaskState& get_state_or_create(loom::Id id) {
        auto it = states.find(id);
        if (it == states.end()) {
            auto p = states.emplace(std::make_pair(id, TaskState(get_node(id))));
            it = p.first;
        }
        return it->second;
    }

    const TaskNode& get_node(loom::Id id) {
        return plan.get_node(id);
    }

    void add_ready_nodes(std::vector<loom::Id> &ids);
    void set_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc);

    const Plan& get_plan() const {
        return plan;
    }

    void remove_state(loom::Id id);

    bool is_ready(const TaskNode &node);
    void add_ready_nexts(const TaskNode &node);

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

    WorkerConnection *get_best_holder_of_deps(TaskNode *task);
    WorkerConnection *find_best_worker_for_node(TaskNode *task);

    void expand_node(const TaskNode &node);
    void expand_dslice(const TaskNode &node);
    void expand_dget(const TaskNode &node);
    void make_expansion(std::vector<std::string> &configs, const TaskNode &node1,
                        const TaskNode &node2, loom::Id id_base1, loom::Id id_base2);
};


#endif // LOOM_SERVER_COMPSTATE_H
