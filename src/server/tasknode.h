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

enum class TaskDefFlags : size_t {
  RESULT
};

enum class TaskNodeFlags : size_t {
    FINISHED,
    CHECKPOINT,
    PLANNED,
    FLAGS_COUNT
};

struct TaskDef
{
    int n_cpus; // TODO: Replace by resource index
    std::vector<TaskNode*> inputs;
    loom::base::Id task_type;
    std::string config;
    std::bitset<1> flags;
    std::string checkpoint_path;
};

enum class TaskStatus {
    NONE,
    RUNNING,
    TRANSFER,
    LOADING,
    OWNER,
};


template<typename T> using WorkerMap = std::unordered_map<WorkerConnection*, T>;


class TaskNode {

public:

    TaskNode(loom::base::Id id, TaskDef &&task);

    loom::base::Id get_id() const {
        return id;
    }

    inline bool is_result() const {
        return task.flags.test(static_cast<size_t>(TaskDefFlags::RESULT));
    }

    bool is_computed() const {
        return flags.test(static_cast<size_t>(TaskNodeFlags::FINISHED));
    }

    inline bool has_checkpoint() const {
        return flags.test(static_cast<size_t>(TaskNodeFlags::CHECKPOINT));
    }

    bool has_defined_checkpoint() const {
        return !task.checkpoint_path.empty();
    }

    void set_checkpoint() {
        flags.set(static_cast<size_t>(TaskNodeFlags::CHECKPOINT));
    }

    bool is_planned() const {
        return flags.test(static_cast<size_t>(TaskNodeFlags::PLANNED));
    }

    void set_planned() {
        flags.set(static_cast<size_t>(TaskNodeFlags::PLANNED));
    }

    void reset_result_flag();

    inline size_t get_size() const {
        return size;
    }

    inline size_t get_length() const {
        return length;
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

    bool is_active() const;
    WorkerConnection* get_random_owner();

    void add_next(TaskNode *node) {
        nexts.insert(node);
    }

    TaskStatus get_worker_status(WorkerConnection *wc) {
        auto i = workers.find(wc);
        if (i == workers.end()) {
            return TaskStatus::NONE;
        }
        return i->second;
    }

    void set_worker_status(WorkerConnection *wc, TaskStatus status) {
        workers[wc] = status;
    }

    void set_remaining_inputs(int value) {
        remaining_inputs = value;
    }

    int get_remaining_inputs() const {
        return remaining_inputs;
    }

    inline bool input_is_ready(TaskNode *node) {
        assert(remaining_inputs > 0);
        return --remaining_inputs == 0;
    }

    inline bool is_ready() const {
       return remaining_inputs == 0;
    }

    template<typename F> inline void foreach_owner(const F &f) const {
        for(auto &pair : workers) {
            if (pair.second == TaskStatus::OWNER) {
                f(pair.first);
            }
        }
    }

    template<typename F> inline void foreach_worker(const F &f) const {
        for(auto &pair : workers) {
            if (pair.second != TaskStatus::NONE) {
                f(pair.first, pair.second);
            }
        }
    }

    void reset_owners();

    const WorkerMap<TaskStatus>& get_workers() const {
        return workers;
    }

    bool next_finished(TaskNode &);

    void set_as_finished(WorkerConnection *wc, size_t size, size_t length);
    void set_as_loaded(WorkerConnection *wc, size_t size, size_t length);
    void set_as_running(WorkerConnection *wc);
    void set_as_loading(WorkerConnection *wc);
    void set_as_transferred(WorkerConnection *wc);
    void set_as_none(WorkerConnection *wc);

    // For unit testing
    void set_as_finished_no_check(WorkerConnection *wc, size_t size, size_t length);
    void set_not_needed() {
        flags.reset(static_cast<size_t>(TaskNodeFlags::PLANNED));
        flags.reset(static_cast<size_t>(TaskNodeFlags::FINISHED));
    }

    std::string debug_str() const;

private:

    // Declaration
    loom::base::Id id;
    TaskDef task;
    std::unordered_multiset<TaskNode*> nexts;

    // Runtime info
    std::bitset<3> flags;
    WorkerMap<TaskStatus> workers;
    size_t size;
    size_t length;
    size_t remaining_inputs;
    bool _slow_is_ready() const;
};


#endif // LOOM_SERVER_TASKNODE_H
