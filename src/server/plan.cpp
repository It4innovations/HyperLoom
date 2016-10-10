#include "plan.h"

#include <algorithm>

#include "libloom/loomplan.pb.h"
#include "libloom/log.h"

static PlanNode::Policy read_task_policy(loomplan::Task_Policy policy) {
    switch(policy) {
        case loomplan::Task_Policy_POLICY_STANDARD:
            return PlanNode::POLICY_STANDARD;
        case loomplan::Task_Policy_POLICY_SIMPLE:
            return PlanNode::POLICY_SIMPLE;
        case loomplan::Task_Policy_POLICY_SCHEDULER:
            return PlanNode::POLICY_SCHEDULER;
        default:
            loom::llog->critical("Invalid task policy");
            exit(1);
    }
}

Plan::Plan()
{

}

Plan::Plan(const loomplan::Plan &plan, loom::Id id_base, loom::Dictionary &dictionary)
{
    std::vector<int> resources;

    loom::Id resource_ncpus = dictionary.find_or_create("loom/resource/cpus");
    auto rr_size = plan.resource_requests_size();
    for (int i = 0; i < rr_size; i++) {
        auto &rr = plan.resource_requests(i);
        assert(rr.resources_size() == 1);
        assert(rr.resources(0).resource_type() == resource_ncpus);
        resources.push_back(rr.resources(0).value());
    }

    auto task_size = plan.tasks_size();
    tasks.reserve(task_size);
    for (int i = 0; i < task_size; i++) {
        const auto& pt = plan.tasks(i);
        auto id = i + id_base;

        std::vector<int> inputs;
        auto inputs_size = pt.input_ids_size();
        for (int j = 0; j < inputs_size; j++) {
            inputs.push_back(id_base + pt.input_ids(j));
        }

        int n_cpus = 0;
        if (pt.resource_request_index() != -1) {
            assert(pt.resource_request_index() >= 0);
            assert(pt.resource_request_index() < (int) resources.size());
            n_cpus = resources[pt.resource_request_index()];
        }
        PlanNode pnode(id, i, read_task_policy(pt.policy()), n_cpus, false,
                       pt.task_type(), pt.config(), std::move(inputs));
        tasks.emplace(std::make_pair(id, std::move(pnode)));
    }

    for (auto& pair : tasks) {
        auto& task = pair.second;
        loom::Id my_id = task.id;
        std::vector<loom::Id> inps(task.get_inputs());
        std::sort(inps.begin(), inps.end());
        loom::Id prev = -1;
        for (loom::Id id : inps) {
            if (prev != id) {
                auto it = tasks.find(id);
                assert(it != tasks.end());
                it->second.nexts.push_back(my_id);
                prev = id;
            }
        }
    }

    for (int i = 0; i < plan.result_ids_size(); i++)
    {                
        auto it = tasks.find(plan.result_ids(i) + id_base);
        assert(it != tasks.end());
        it->second.set_as_result();
    }
}

std::vector<loom::Id> Plan::get_init_tasks() const
{
    std::vector<loom::Id> result;
    foreach_task([&result](const PlanNode &node){
          if (node.get_inputs().empty()) {
            result.push_back(node.id);
       }
    });
    return result;
}
