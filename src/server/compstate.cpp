
#include "compstate.h"
#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"
#include "libloom/loomplan.pb.h"

constexpr static double TRANSFER_COST_COEF = 1.0 / (1024 * 1024); // 1MB = 1cost

using namespace loom::base;

ComputationState::ComputationState(Server &server) : server(server)
{
   Dictionary &dictionary = server.get_dictionary();
   slice_task_id = dictionary.find_or_create("loom/base/slice");
   get_task_id = dictionary.find_or_create("loom/base/get");
   dslice_task_id = dictionary.find_or_create("loom/scheduler/dslice");
   dget_task_id = dictionary.find_or_create("loom/scheduler/dget");

   if (server.get_loop()) {
        base_time = uv_now(server.get_loop());
   }
}

void ComputationState::add_node(std::unique_ptr<TaskNode> &&node) {
    auto id = node->get_id();

    if (node->get_inputs().empty()) {
        pending_nodes.insert(node.get());
    }

    auto inputs = node->get_inputs();
    std::sort(inputs.begin(), inputs.end());
    inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());

    for (TaskNode* input_node : inputs) {
        input_node->add_next(node.get());
    }

    nodes.insert(std::make_pair(id, std::move(node)));
}

void ComputationState::set_final_node(Id id)
{
    final_nodes[id] = get_node(id).get_client_id();
}

/*void ComputationState::set_plan(Plan &&plan)
{
    this->plan = std::move(plan);
   auto task_ids = this->plan.get_init_tasks();
   assert(!task_ids.empty());
   add_ready_nodes(task_ids);
}*/

void ComputationState::reserve_new_nodes(size_t size)
{
    nodes.reserve(nodes.size() + size);
}

TaskNode *ComputationState::get_node_ptr(Id id)
{
    auto it = nodes.find(id);
    if (it == nodes.end()) {
        return nullptr;
    }
    return it->second.get();
}

TaskNode& ComputationState::get_node(Id id)
{
    auto it = nodes.find(id);
    if (it == nodes.end()) {
       logger->critical("Cannot find node for id={}", id);
       abort();
    }
    return *it->second;
}

const TaskNode &ComputationState::get_node(Id id) const
{
   auto it = nodes.find(id);
   if (it == nodes.end()) {
      logger->critical("Cannot find node for id={}", id);
      abort();
   }
   return *it->second;
}

void ComputationState::activate_pending_node(TaskNode &node, WorkerConnection *wc)
{   
   auto it = pending_nodes.find(&node);
   assert(it != pending_nodes.end());
   pending_nodes.erase(it);
   node.set_as_running(wc);
}

void ComputationState::remove_node(TaskNode &node)
{
   auto it = nodes.find(node.get_id());
   assert(it != nodes.end());
   nodes.erase(it);
}

void ComputationState::add_ready_nexts(const TaskNode &node)
{
   for (TaskNode *nn : node.get_nexts()) {
      if (is_ready(*nn)) {
         if (nn->get_task_def().policy == TaskPolicy::SCHEDULER) {
            assert(0);
            //expand_node(node);
         } else {            
            add_pending_node(*nn);
         }
      }
   }
}

bool ComputationState::is_finished() const
{
    return nodes.empty();
}

int ComputationState::get_n_data_objects() const
{
    int count = 0;
    for (auto& pair : nodes) {
        if (pair.second->is_computed()) {
            count += 1;
        }
    }
    return count;
}

void ComputationState::add_pending_node(TaskNode &node)
{
   pending_nodes.insert(&node);
}

/*
void ComputationState::expand_node(const PlanNode &node)
{
   loom::base::Id id = node.get_task_type();

   if (id == dslice_task_id) {
      expand_dslice(node);
   } else if (id == dget_task_id) {
      expand_dget(node);
   } else {
      logger->critical("Unknown scheduler task: {}", id);
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

   std::vector<loom::base::Id> inputs = node.get_inputs();
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

   loom::base::Id id_base1 = server.new_id(configs.size());
   loom::base::Id id_base2 = server.new_id(configs.size());

   logger->debug("Expanding 'dslice' id={} length={} pieces={} new_id_base={}",
                 node1.get_id(), length, configs.size(), id_base1);

   PlanNode new_node(node1.get_id(),-1, PlanNode::POLICY_SIMPLE, -1, false,
                     slice_task_id, "", node1.get_inputs());
   make_expansion(configs, new_node, node2, id_base1, id_base2);
}

void ComputationState::expand_dget(const PlanNode &node)
{
   const PlanNode &node1 = node;
   assert(node1.get_nexts().size() == 1);
   // Do a copy again
   const PlanNode &node2 = get_node(node.get_nexts()[0]);

   std::vector<loom::base::Id> inputs = node.get_inputs();
   assert(inputs.size() == 1);
   TaskState &input = get_state(inputs[0]);
   size_t length = input.get_length();

   std::vector<std::string> configs;
   for (size_t i = 0; i < length; i++) {
      configs.push_back(std::string(reinterpret_cast<char*>(&i), sizeof(size_t)));
   }

   loom::base::Id id_base1 = server.new_id(configs.size());
   loom::base::Id id_base2 = server.new_id(configs.size());

   logger->debug("Expanding 'dget' id={} length={} new_id_base={}",
                 node1.get_id(), length, id_base1);

   PlanNode new_node(node1.get_id(),-1, PlanNode::POLICY_SIMPLE, -1, false,
                     get_task_id, "", node1.get_inputs());
   make_expansion(configs, new_node, node2, id_base1, id_base2);
}

void ComputationState::make_expansion(std::vector<std::string> &configs,
                                      const PlanNode &n1,
                                      const PlanNode &n2,
                                      loom::base::Id id_base1,
                                      loom::base::Id id_base2)

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
                  node1.get_policy(), node1.get_n_cpus(), false,
                  node1.get_task_type(), config1, node1.get_inputs());
      t1.set_nexts(std::vector<loom::base::Id>{id_base2});
      plan.add_node(std::move(t1));

      add_pending_task(id_base1);

      PlanNode t2(id_base2, -1,
                  node2.get_policy(), node1.get_n_cpus(), false,
                  node2.get_task_type(), node2.get_config(),
                  std::vector<int>{id_base1});
      t2.set_nexts(node2.get_nexts());
      plan.add_node(std::move(t2));

      ids1.push_back(id_base1);
      id_base1++;
      ids2.push_back(id_base2);
      id_base2++;
   }

   for (loom::base::Id id : node1.get_inputs()) {
      plan.get_node(id).replace_next(node1.get_id(), ids1);
      get_state(id).inc_ref_counter(size - 1);
   }

   for (loom::base::Id id : node2.get_nexts()) {
      plan.get_node(id).replace_input(node2.get_id(), ids2);
   }
}*/


bool ComputationState::is_ready(const TaskNode &node)
{
   for (TaskNode *input_node : node.get_inputs()) {
      if (!input_node->is_computed()) {
         return false;
      }
   }
   return true;
}

static TaskPolicy read_task_policy(loomplan::Task_Policy policy) {
    switch(policy) {
        case loomplan::Task_Policy_POLICY_STANDARD:
            return TaskPolicy::STANDARD;
        case loomplan::Task_Policy_POLICY_SIMPLE:
            return TaskPolicy::SIMPLE;
        case loomplan::Task_Policy_POLICY_SCHEDULER:
            return TaskPolicy::SCHEDULER;
        default:
            loom::base::logger->critical("Invalid task policy");
            exit(1);
    }
}

void ComputationState::add_plan(const loomplan::Plan &plan)
{
    auto task_size = plan.tasks_size();
    loom::base::Id id_base = server.new_id(task_size);
    loom::base::logger->debug("Plan id range from={}, size={}", id_base, task_size);

    std::vector<int> resources;
    loom::base::Id resource_ncpus = server.get_dictionary().find_or_create("loom/resource/cpus");
    auto rr_size = plan.resource_requests_size();
    for (int i = 0; i < rr_size; i++) {
        auto &rr = plan.resource_requests(i);
        assert(rr.resources_size() == 1);
        assert(rr.resources(0).resource_type() == resource_ncpus);
        resources.push_back(rr.resources(0).value());
    }

    reserve_new_nodes(task_size);
    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        auto id = i + id_base;

        TaskDef def;

        def.task_type = pt.task_type();
        def.config = pt.config();
        def.policy = read_task_policy(pt.policy());

        auto inputs_size = pt.input_ids_size();
        for (int j = 0; j < inputs_size; j++) {
            def.inputs.push_back(&get_node(id_base + pt.input_ids(j)));
        }

        int n_cpus = 0;
        if (pt.resource_request_index() != -1) {
            assert(pt.resource_request_index() >= 0);
            assert(pt.resource_request_index() < (int) resources.size());
            n_cpus = resources[pt.resource_request_index()];
        }
        def.n_cpus = n_cpus;
        add_node(std::make_unique<TaskNode>(id, i, std::move(def)));
    }

    final_nodes.reserve(final_nodes.size() + plan.result_ids_size());

    for (int i = 0; i < plan.result_ids_size(); i++)
    {
        set_final_node(plan.result_ids(i) + id_base);
    }
}

void ComputationState::test_ready_nodes(std::vector<Id> ids)
{
    pending_nodes.clear();
    for (auto id : ids) {
        pending_nodes.insert(&get_node(id));
    }
}

Id ComputationState::pop_result_client_id(Id id)
{
    auto it = final_nodes.find(id);
    assert(it != final_nodes.end());
    Id client_id = it->second;
    final_nodes.erase(it);
    return client_id;
}

std::unique_ptr<TaskNode> ComputationState::pop_node(Id id)
{
    std::unique_ptr<TaskNode> result;
    auto it = nodes.find(id);
    assert(it != nodes.end());
    result = std::move(it->second);
    nodes.erase(it);
    return result;
}

void ComputationState::clear_all()
{
    pending_nodes.clear();
    final_nodes.clear();
    nodes.clear();
}

