#ifndef LOOM_SERVER_TASKNODE_H
#define LOOM_SERVER_TASKNODE_H

#include "libloom/types.h"
#include "libloom/compat.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>


enum class TaskPolicy {
  STANDARD = 1,
  SIMPLE = 2,
  SCHEDULER = 3
};

struct TaskDef
{    
    int n_cpus; // TODO: Replace by resource index
    std::vector<loom::base::Id> inputs;
    loom::base::Id task_type;
    std::string config;
    TaskPolicy policy;
};

enum class TaskStatus {
    NONE,
    RUNNING,
    TRANSFER,
    OWNER,
};

inline bool is_planned_owner(TaskStatus status) {
    return status == TaskStatus::OWNER || \
           status == TaskStatus::RUNNING || \
           status == TaskStatus::TRANSFER;
}

class WorkerConnection;
template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;

class TaskNode {

public:

    struct DataState {
        WorkerMap<TaskStatus> workers;
        int ref_counter;
        size_t size;
        size_t length;
    };

    TaskNode(loom::base::Id id, loom::base::Id client_id, TaskDef &&task);

    loom::base::Id get_id() const {
        return id;
    }

    loom::base::Id get_client_id() const {
        return client_id;
    }

    bool has_state() const {
        return state != nullptr;
    }

    size_t get_size() const {
        assert(state);
        return state->size;
    }

    size_t get_length() const {
        assert(state);
        return state->length;
    }

    int get_n_cpus() const {
        return task.n_cpus;
    }

    const TaskDef& get_task_def() const {
        return task;
    }

    const std::vector<loom::base::Id>& get_inputs() const {
        return task.inputs;
    }

    const std::vector<loom::base::Id>& get_nexts() const {
        return nexts;
    }

    bool is_computed() const;
    WorkerConnection* get_random_owner();

    void add_next(loom::base::Id id) {
        nexts.push_back(id);
    }

    TaskStatus get_worker_status(WorkerConnection *wc) {
        if (!state) {
            return TaskStatus::NONE;
        }
        auto i = state->workers.find(wc);
        if (i == state->workers.end()) {
            return TaskStatus::NONE;
        }
        return i->second;
    }

    void set_worker_status(WorkerConnection *wc, TaskStatus status) {
        ensure_state();
        state->workers[wc] = status;
    }

    void ensure_state() {
        if (!state) {
            create_state();
        }
    }

    void create_state();

    template<typename F> void foreach_owner(const F &f) const {
        if (!state) {
            return;
        }
        for(auto &pair : state->workers) {
            if (pair.second == TaskStatus::OWNER) {
                f(pair.first);
            }
        }
    }

    template<typename F> void foreach_worker(const F &f) const {
        if (!state) {
            return;
        }
        for(auto &pair : state->workers) {
            if (pair.second != TaskStatus::NONE) {
                f(pair.first, pair.second);
            }
        }
    }

    int get_ref_counter() const {
        return state->ref_counter;
    }

    void inc_ref_counter(int value) {
        state->ref_counter += value;
    }

    bool dec_ref_counter() {
        state->ref_counter -= 1;
        return state->ref_counter == 0;
    }

    void set_as_finished(WorkerConnection *wc, size_t size, size_t length);
    void set_as_running(WorkerConnection *wc);
    void set_as_transferred(WorkerConnection *wc);

    // For unit testing
    void set_as_finished_no_check(WorkerConnection *wc, size_t size, size_t length);

private:
    loom::base::Id id;
    TaskDef task;
    std::vector<loom::base::Id> nexts;
    loom::base::Id client_id;
    std::unique_ptr<DataState> state;
};


#endif // LOOM_SERVER_TASKNODE_H
