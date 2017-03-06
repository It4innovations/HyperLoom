#ifndef LIBLOOMW_TASK_H
#define LIBLOOMW_TASK_H

#include "libloom/types.h"

#include <vector>
#include <string>
#include <unordered_set>


namespace loom {

class Worker;

/** Class that repsents one task */
class Task {

public:
    Task(base::Id id, int task_type, const std::string &config, int n_cpus)
        : id(id), task_type(task_type), config(config), n_cpus(n_cpus), n_unresolved(0) {}

    Task(base::Id id, int task_type, std::string &&config, int n_cpus)
        : id(id), task_type(task_type), config(std::move(config)), n_cpus(n_cpus), n_unresolved(0) {}

    base::Id get_id() const {
        return id;
    }

    base::Id get_task_type() const {
        return task_type;
    }

    const std::string& get_config() const {
        return config;
    }

    bool is_ready(base::Id finished_id);
    bool is_ready() const {
        return n_unresolved == 0;
    }

    void add_input(base::Id id) {
        inputs.push_back(id);
    }

    int get_n_cpus() const {
        return n_cpus;
    }

    const std::vector<base::Id>& get_inputs() const {
        return inputs;
    }

    void set_unresolved_set(std::unordered_set<base::Id> &&set);

protected:
    base::Id id;
    base::Id task_type;
    std::vector<base::Id> inputs;
    std::string config;
    int n_cpus;
    size_t n_unresolved;
    std::unordered_set<base::Id> unresolved_set;
};

}

#endif // LIBLOOMW_TASK_H
