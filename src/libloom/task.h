#ifndef LIBLOOM_TASK_H
#define LIBLOOM_TASK_H

#include "types.h"
#include <vector>
#include <string>

namespace loom {

class Worker;

class Task {

public:
    Task(Id id, int task_type, const std::string &config)
        : id(id), task_type(task_type), config(config) {}

    Id get_id() const {
        return id;
    }

    Id get_task_type() const {
        return task_type;
    }

    const std::string& get_config() const {
        return config;
    }

    bool is_ready(const Worker &worker) const;

    void add_input(Id id) {
        inputs.push_back(id);
    }

    const std::vector<Id>& get_inputs() const {
        return inputs;
    }

protected:
    Id id;
    Id task_type;
    std::vector<Id> inputs;
    std::string config;
};

}

#endif // LIBLOOM_TASK_H
