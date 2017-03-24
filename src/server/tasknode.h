#ifndef LOOM_SERVER_TASKNODE_H
#define LOOM_SERVER_TASKNODE_H

#include "libloom/types.h"
#include "libloom/compat.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <bitset>

class WorkerConnection;
class TaskNode;

enum class TaskFlags : size_t {
  RESULT
};

struct TaskDef
{
    int n_cpus; // TODO: Replace by resource index
    std::vector<TaskNode*> inputs;
    loom::base::Id task_type;
    std::string config;
    std::bitset<1> flags;
};

enum class TaskStatus {
    NONE,
    RUNNING,
    TRANSFER,
    OWNER,
};


template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;


class TaskNode {

public:

    struct RuntimeState {
        WorkerMap<TaskStatus> workers;
        size_t size;
        size_t length;
        size_t remaining_inputs;
    };

    TaskNode(loom::base::Id id, TaskDef &&task);

    loom::base::Id get_id() const {
        return id;
    }

    bool has_state() const {
        return state != nullptr;
    }

    inline bool is_result() const {
        return task.flags.test(static_cast<size_t>(TaskFlags::RESULT));
    }

    void reset_result_flag();

    inline size_t get_size() const {
        //assert(state);
        return state->size;
    }

    inline size_t get_length() const {
        //assert(state);
        return state->length;
    }

    int get_n_cpus() const {
        return task.n_cpus;
    }

    const TaskDef& get_task_def() const {
        return task;
    }

    const std::vector<TaskNode*>& get_inputs() const {
        return task.inputs;
    }

    const std::unordered_multiset<TaskNode*>& get_nexts() const {
        return nexts;
    }

    bool is_computed() const;
    bool is_active() const;
    WorkerConnection* get_random_owner();

    void add_next(TaskNode *node) {
        nexts.insert(node);
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

    inline void ensure_state() {
        if (!state) {
            create_state();
        }
    }

    inline bool input_is_ready(TaskNode *node) {
         if (!state) {
            create_state(node);
        }
        assert(state->remaining_inputs > 0);
        return --state->remaining_inputs == 0;
    }

    inline bool is_ready() const {
       if (!state) {
          return _slow_is_ready();
       }
       return state->remaining_inputs == 0;
    }

    void create_state(TaskNode *just_finishing_input = nullptr);

    template<typename F> inline void foreach_owner(const F &f) const {
        if (!state) {
            return;
        }
        for(auto &pair : state->workers) {
            if (pair.second == TaskStatus::OWNER) {
                f(pair.first);
            }
        }
    }

    template<typename F> inline void foreach_worker(const F &f) const {
        if (!state) {
            return;
        }
        for(auto &pair : state->workers) {
            if (pair.second != TaskStatus::NONE) {
                f(pair.first, pair.second);
            }
        }
    }

    void reset_owners();

    const WorkerMap<TaskStatus>& get_workers() const {
        return state->workers;
    }

    bool next_finished(TaskNode &);

    void set_as_finished(WorkerConnection *wc, size_t size, size_t length);
    void set_as_running(WorkerConnection *wc);
    void set_as_transferred(WorkerConnection *wc);
    void set_as_none(WorkerConnection *wc);

    // For unit testing
    void set_as_finished_no_check(WorkerConnection *wc, size_t size, size_t length);

    std::string debug_str() const;

private:
    loom::base::Id id;
    TaskDef task;
    std::unordered_multiset<TaskNode*> nexts;
    std::unique_ptr<RuntimeState> state;

    bool _slow_is_ready() const;
};


#endif // LOOM_SERVER_TASKNODE_H
