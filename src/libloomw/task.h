#ifndef LIBLOOMW_TASK_H
#define LIBLOOMW_TASK_H

#include "libloom/types.h"

#include <vector>
#include <string>


namespace loom {

class Worker;

/** Class that repsents one task */
class Task {

public:
    Task(base::Id id, int task_type, const std::string &config, int n_cpus)
        : id(id), task_type(task_type), config(config), n_cpus(n_cpus) {}

    Task(base::Id id, int task_type, std::string &&config, int n_cpus)
        : id(id), task_type(task_type), config(std::move(config)), n_cpus(n_cpus) {}

    base::Id get_id() const {
        return id;
    }

    base::Id get_task_type() const {
        return task_type;
    }

    const std::string& get_config() const {
        return config;
    }

    bool is_ready(const Worker &worker) const;

    void add_input(base::Id id) {
        inputs.push_back(id);
    }

    int get_n_cpus() const {
        return n_cpus;
    }

    const std::vector<base::Id>& get_inputs() const {
        return inputs;
    }

protected:
    base::Id id;
    base::Id task_type;
    std::vector<base::Id> inputs;
    std::string config;
    int n_cpus;
};

}

#endif // LIBLOOMW_TASK_H
