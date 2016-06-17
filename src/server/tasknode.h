#ifndef LOOM_SERVER_TASKNODE_H
#define LOOM_SERVER_TASKNODE_H

#include <libloom/types.h>

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

class WorkerConnection;

class TaskNode {
public:

    typedef std::vector<TaskNode*> Vector;

    enum State {
        WAITING,
        RUNNING,
        FINISHED
    };

    TaskNode(loom::Id id, int task_type, const std::string &config)
        : state(WAITING), id(id), task_type(task_type), config(config) {}

    bool is_ready() {
        assert(state == WAITING);
        for (TaskNode *task : inputs) {
            if (task->state != FINISHED) {
                return false;
            }
        }
        return true;
    }

    void collect_ready_nexts(std::vector<TaskNode*> &ready)
    {
        for (TaskNode *task : nexts) {
            if (task->is_ready()) {
                ready.push_back(task);
            }
        }
    }

    void add_input(TaskNode *task) {
        inputs.push_back(task);
    }

    void add_next(TaskNode *task) {
        nexts.push_back(task);
    }

    void set_state(State state) {
        this->state = state;
    }

    bool is_waiting() const {
        return state == WAITING;
    }

    loom::Id get_id() const {
        return id;
    }

    loom::TaskId get_task_type() const {
        return task_type;
    }

    const std::string& get_config() const {
        return config;
    }

    void add_owner(WorkerConnection *wconn) {
        owners.push_back(wconn);
    }

    const auto& get_owners() const {
        return owners;
    }

    bool is_owner(WorkerConnection &wconn) {
        return std::find(owners.begin(), owners.end(), &wconn) != owners.end();
    }

    void set_finished() {
        assert(state == RUNNING);
        state = FINISHED;
    }

    const auto& get_inputs() {
        return inputs;
    }

private:
    State state;
    loom::Id id;
    loom::TaskId task_type;
    Vector inputs;
    Vector nexts;
    std::string config;

    std::vector<WorkerConnection*> owners;
};



#endif // LOOM_SERVER_TASKNODE_H
