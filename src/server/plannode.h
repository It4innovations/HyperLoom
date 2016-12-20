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

    PlanNode(loom::base::Id id,
             loom::base::Id client_id,
             Policy policy,
             int n_cpus,
             bool result_flag,
             int task_type,
             const std::string &config,
             std::vector<loom::base::Id> &&inputs)
        : id(id), policy(policy), n_cpus(n_cpus), result_flag(result_flag), task_type(task_type),
          inputs(std::move(inputs)), config(config),
          client_id(client_id)
    {}

    PlanNode(loom::base::Id id,
             loom::base::Id client_id,
             Policy policy,
             int n_cpus,
             bool result_flag,
             loom::base::Id task_type,
             const std::string &config,
             const std::vector<loom::base::Id> &inputs)
        : id(id), policy(policy), n_cpus(n_cpus), result_flag(result_flag), task_type(task_type),
          inputs(inputs), config(config),
          client_id(client_id)
    {}

    Policy get_policy() const {
        return policy;
    }

    loom::base::Id get_id() const {
        return id;
    }

    loom::base::Id get_client_id() const {
        return client_id;
    }

    loom::base::Id get_task_type() const {
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

    const std::vector<loom::base::Id>& get_inputs() const {
        return inputs;
    }

    const std::vector<loom::base::Id>& get_nexts() const {
        return nexts;
    }

    void replace_input(loom::base::Id old_input, const std::vector<loom::base::Id> &new_inputs);
    void replace_next(loom::base::Id old_next, const std::vector<loom::base::Id> &new_nexts);

    std::string get_type_name(Server &server);
    std::string get_info(Server &server);

    bool is_result() const {
        return result_flag;
    }

    void set_as_result() {
        result_flag = true;
    }

    int get_n_cpus() const {
        return n_cpus;
    }

private:
    loom::base::Id id;
    Policy policy;
    int n_cpus; // TODO: Replace by resource index
    bool result_flag;
    loom::base::Id task_type;
    std::vector<loom::base::Id> inputs;
    std::vector<loom::base::Id> nexts;
    std::string config;
    loom::base::Id client_id;
};



#endif // LOOM_SERVER_TASKNODE_H
