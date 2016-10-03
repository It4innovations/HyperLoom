#ifndef LOOM_SERVER_PLAN_H
#define LOOM_SERVER_PLAN_H

#include "tasknode.h"
#include <unordered_map>
#include <unordered_set>
#include <memory.h>

namespace loomplan {
class Plan;
}

class Plan {

public:
    Plan();
    Plan(const loomplan::Plan &plan, loom::Id id_base);

    template<typename F> void foreach_task(F f) const {
        for (auto &pair : tasks) {
            f(pair.second);
        }
    }

    const TaskNode& get_node(loom::Id id) const {
        auto it = tasks.find(id);
        assert(it != tasks.end());
        return it->second;
    }

    TaskNode& get_node(loom::Id id) {
        auto it = tasks.find(id);
        assert(it != tasks.end());
        return it->second;
    }

    size_t get_size() const {
        return tasks.size();
    }

    std::vector<loom::Id> get_init_tasks() const;

    template<typename T>
    void add_node(T&& task) {
        tasks.emplace(std::make_pair(task.get_id(), std::forward<T>(task)));
    }

    void remove_node(loom::Id id) {
        auto it = tasks.find(id);
        assert(it != tasks.end());
        tasks.erase(it);
    }

private:
    std::unordered_map<loom::Id, TaskNode> tasks;
};

#endif // LOOM_SERVER_PLAN_H
