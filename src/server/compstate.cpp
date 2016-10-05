
#include "compstate.h"
#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"

#include <limits.h>

ComputationState::ComputationState(Server &server) : server(server)
{
    loom::Dictionary &dictionary = server.get_dictionary();
    slice_task_id = dictionary.find_or_create("loom/base/slice");
    get_task_id = dictionary.find_or_create("loom/base/get");
    dslice_task_id = dictionary.find_or_create("loom/scheduler/dslice");
    dget_task_id = dictionary.find_or_create("loom/scheduler/dget");
}

void ComputationState::set_plan(Plan &&plan)
{
    this->plan = std::move(plan);
}

TaskDistribution ComputationState::compute_initial_distribution()
{
    auto task_ids = plan.get_init_tasks();
    add_ready_nodes(task_ids);
    return compute_distribution();
}

void ComputationState::add_worker(WorkerConnection* wconn)
{
    workers[wconn] = WorkerInfo();
}

void ComputationState::set_running_task(WorkerConnection *wc, loom::Id id)
{
    TaskState &state = get_state_or_create(id);
    auto it = pending_tasks.find(id);
    assert(it != pending_tasks.end());
    pending_tasks.erase(it);

    assert(state.get_worker_status(wc) == TaskState::NONE);
    state.set_worker_status(wc, TaskState::RUNNING);
    workers[wc].n_tasks++;
}

void ComputationState::set_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    TaskState &state = get_state(id);
    assert(state.get_worker_status(wc) == TaskState::RUNNING);
    state.set_worker_status(wc, TaskState::OWNER);
    state.set_size(size);
    state.set_length(length);
}

void ComputationState::remove_state(loom::Id id)
{
    auto it = states.find(id);
    assert(it != states.end());
    states.erase(it);
}

void ComputationState::add_ready_nexts(const PlanNode &node)
{
    for (loom::Id id : node.get_nexts()) {
        const PlanNode &node = get_node(id);
        if (is_ready(node)) {
            if (node.get_policy() == PlanNode::POLICY_SCHEDULER) {
                expand_node(node);
            } else {
                pending_tasks.insert(id);
            }
        }
    }
}

void ComputationState::expand_node(const PlanNode &node)
{
    loom::Id id = node.get_task_type();

    if (id == dslice_task_id) {
        expand_dslice(node);
    } else if (id == dget_task_id) {
        expand_dget(node);
    } else {
        loom::llog->critical("Unknown scheduler task: {}", id);
        exit(1);
    }
}

void ComputationState::expand_dslice(const PlanNode &node)
{
    size_t n_cpus = 0;
    for (auto& pair : workers) {
        n_cpus += pair.first->get_resource_cpus();
    }
    assert(n_cpus);

    const PlanNode &node1 = node;
    assert(node1.get_nexts().size() == 1);
    // Do a copy again
    const PlanNode &node2 = get_node(node.get_nexts()[0]);

    std::vector<loom::Id> inputs = node.get_inputs();
    assert(inputs.size() == 1);
    TaskState &input = get_state(inputs[0]);
    size_t length = input.get_length();

    size_t slice_size = round(static_cast<double>(length) / (n_cpus * 4));
    // * 4 is just an heuristic, we probably need a better adjustment
    if (slice_size == 0) {
        slice_size = 1;
    }

    size_t i = 0;
    std::vector<std::string> configs;
    while (i < length) {
        size_t indices[2];
        indices[0] = i;
        indices[1] = i + slice_size;
        if (indices[1] > length) {
            indices[1] = length;
        }
        i = indices[1];
        configs.push_back(std::string(reinterpret_cast<char*>(&indices), sizeof(size_t) * 2));
    }

    loom::Id id_base1 = server.new_id(configs.size());
    loom::Id id_base2 = server.new_id(configs.size());

    loom::llog->debug("Expanding 'dslice' id={} length={} pieces={} new_id_base={}",
                      node1.get_id(), length, configs.size(), id_base1);

    PlanNode new_node(node1.get_id(),-1, PlanNode::POLICY_SIMPLE, false,
                      slice_task_id, "", node1.get_inputs());
    make_expansion(configs, new_node, node2, id_base1, id_base2);
}

void ComputationState::expand_dget(const PlanNode &node)
{
    const PlanNode &node1 = node;
    assert(node1.get_nexts().size() == 1);
    // Do a copy again
    const PlanNode &node2 = get_node(node.get_nexts()[0]);

    std::vector<loom::Id> inputs = node.get_inputs();
    assert(inputs.size() == 1);
    TaskState &input = get_state(inputs[0]);
    size_t length = input.get_length();

    std::vector<std::string> configs;
    for (size_t i = 0; i < length; i++) {
        configs.push_back(std::string(reinterpret_cast<char*>(&i), sizeof(size_t)));
    }

    loom::Id id_base1 = server.new_id(configs.size());
    loom::Id id_base2 = server.new_id(configs.size());

    loom::llog->debug("Expanding 'dget' id={} length={} new_id_base={}",
                      node1.get_id(), length, id_base1);

    PlanNode new_node(node1.get_id(),-1, PlanNode::POLICY_SIMPLE, false,
                      get_task_id, "", node1.get_inputs());
    make_expansion(configs, new_node, node2, id_base1, id_base2);
}

void ComputationState::make_expansion(std::vector<std::string> &configs,
                                      const PlanNode &n1,
                                      const PlanNode &n2,
                                      loom::Id id_base1,
                                      loom::Id id_base2)

{
    PlanNode node1 = n1; // Make copy
    PlanNode node2 = n2; // Make copy
    plan.remove_node(node1.get_id());
    plan.remove_node(node2.get_id());

    size_t size = configs.size();
    std::vector<int> ids1;
    ids1.reserve(size);

    std::vector<int> ids2;
    ids2.reserve(size);

    for (std::string &config1 : configs) {
        PlanNode t1(id_base1, -1,
                    node1.get_policy(), false,
                    node1.get_task_type(), config1, node1.get_inputs());
        t1.set_nexts(std::vector<loom::Id>{id_base2});
        plan.add_node(std::move(t1));

        pending_tasks.insert(id_base1);

        PlanNode t2(id_base2, -1,
                    node2.get_policy(), false,
                    node2.get_task_type(), node2.get_config(),
                    std::vector<int>{id_base1});
        t2.set_nexts(node2.get_nexts());
        plan.add_node(std::move(t2));

        ids1.push_back(id_base1);
        id_base1++;
        ids2.push_back(id_base2);
        id_base2++;
    }

    for (loom::Id id : node1.get_inputs()) {
        plan.get_node(id).replace_next(node1.get_id(), ids1);
        get_state(id).inc_ref_counter(size - 1);
    }

    for (loom::Id id : node2.get_nexts()) {
        plan.get_node(id).replace_input(node2.get_id(), ids2);
    }
}


bool ComputationState::is_ready(const PlanNode &node)
{
    for (loom::Id id : node.get_inputs()) {
        if (states.find(id) == states.end()) {
            return false;
        }
        if (get_state(id).get_first_owner() == nullptr) {
            return false;
        }
    }
    return true;
}

TaskDistribution ComputationState::compute_distribution()
{
    TaskDistribution result;
    if (pending_tasks.empty()) {
        return result;
    }

    size_t n_workers = workers.size();

    if (n_workers == 0) {
        return result;
    }

    std::vector<WorkerConnection*> wc(n_workers, NULL);

    int i = 0;
    for (auto &pair : workers) {
        wc[i] = pair.first;
        i++;
    }

    for (loom::Id id : pending_tasks) {
        const PlanNode &node = get_node(id);
        int index;
        if (node.get_inputs().size() == 1) {
            index = node.get_inputs()[0];
        } else {
            index = id;
        }
        result[wc[index % n_workers]].push_back(id);
    }
    return result;
}

void ComputationState::add_ready_nodes(std::vector<loom::Id> &ids)
{
    for (loom::Id id : ids) {
        pending_tasks.insert(id);
    }
}
