#ifndef LOOM_SERVER_TASKNODE_H
#define LOOM_SERVER_TASKNODE_H

#include <libloom/types.h>

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

class Plan;
class Server;
class WorkerConnection;

/** A task in the task graph in the server */
class PlanNode {
public:
    friend class Plan;
    typedef std::vector<PlanNode*> Vector;

    enum Policy {
        POLICY_STANDARD,
        POLICY_SIMPLE,
        POLICY_SCHEDULER
    };

    PlanNode(loom::Id id,
             loom::Id client_id,
             Policy policy,
             bool result_flag,
             int task_type,
             const std::string &config,
             std::vector<loom::Id> &&inputs)
        : id(id), policy(policy), result_flag(result_flag), task_type(task_type),
          inputs(std::move(inputs)), config(config),
          client_id(client_id)
    {}

    PlanNode(loom::Id id,
             loom::Id client_id,
             Policy policy,
             bool result_flag,
             int task_type,
             const std::string &config,
             const std::vector<loom::Id> &inputs)
        : id(id), policy(policy), result_flag(result_flag), task_type(task_type),
          inputs(inputs), config(config),
          client_id(client_id)
    {}

    Policy get_policy() const {
        return policy;
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

    void set_nexts(const std::vector<int> &nexts) {
        this->nexts = nexts;
    }

    void set_nexts(std::vector<int> &&nexts) {
        this->nexts = nexts;
    }

    const std::vector<loom::Id>& get_inputs() const {
        return inputs;
    }

    const std::vector<loom::Id>& get_nexts() const {
        return nexts;
    }

    void replace_input(loom::Id old_input, const std::vector<loom::Id> &new_inputs);
    void replace_next(loom::Id old_next, const std::vector<loom::Id> &new_nexts);

    std::string get_type_name(Server &server);
    std::string get_info(Server &server);

    bool is_result() const {
        return result_flag;
    }

    void set_as_result() {
        result_flag = true;
    }


private:
    loom::Id id;
    Policy policy;
    bool result_flag;
    loom::TaskId task_type;
    std::vector<loom::Id> inputs;
    std::vector<loom::Id> nexts;
    std::string config;
    loom::Id client_id;
};



#endif // LOOM_SERVER_TASKNODE_H
