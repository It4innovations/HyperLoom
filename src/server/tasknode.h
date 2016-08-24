#ifndef LOOM_SERVER_TASKNODE_H
#define LOOM_SERVER_TASKNODE_H

#include <libloom/types.h>

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

class WorkerConnection;
class Server;

/** A task in the task graph in the server */
class TaskNode {
public:

    typedef std::vector<TaskNode*> Vector;

    enum State {
        WAITING,
        RUNNING,
        FINISHED
    };

    TaskNode(loom::Id id, loom::Id client_id, int task_type, const std::string &config)
        : state(WAITING), id(id), ref_count(0), task_type(task_type),
          config(config),
          size(0), length(0), client_id(client_id)
    {}

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

    void set_inputs(const TaskNode::Vector &inputs) {
        this->inputs = inputs;
    }

    void add_input(TaskNode *task) {
        inputs.push_back(task);
    }

    void add_next(TaskNode *task) {
        nexts.push_back(task);
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

    void set_state(State state) {
        this->state = state;
    }

    bool is_waiting() const {
        return state == WAITING;
    }

    loom::Id get_id() const {
        return id;
    }

    loom::Id get_client_id() const {
        return client_id;
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

    const std::vector<WorkerConnection*>& get_owners() const {
        return owners;
    }

    bool is_owner(WorkerConnection &wconn) {
        return std::find(owners.begin(), owners.end(), &wconn) != owners.end();
    }

    void set_finished(size_t size, size_t length) {
        assert(state == RUNNING);
        state = FINISHED;
        this->size = size;
        this->length = length;
    }

    const Vector& get_inputs() const {
        return inputs;
    }

    const Vector& get_nexts() const {
        return nexts;
    }

    size_t get_length() const {
        return length;
    }

    void replace_input(TaskNode *old_input, const std::vector<TaskNode*> &new_inputs);

    std::string get_type_name(Server &server);
    std::string get_info(Server &server);


private:
    State state;
    loom::Id id;
    int ref_count;
    loom::TaskId task_type;
    Vector inputs;
    Vector nexts;
    std::string config;

    std::vector<WorkerConnection*> owners;

    size_t size;
    size_t length;

    loom::Id client_id;
};



#endif // LOOM_SERVER_TASKNODE_H
