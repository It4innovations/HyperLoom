#ifndef LIBLOOMW_RESALLOC_H
#define LIBLOOMW_RESALLOC_H

#include <vector>
#include <string>

namespace loom {

class ResourceAllocation
{
public:

    ResourceAllocation();

    void add_cpu(int cpu_id) {
        cpus.push_back(cpu_id);
    }

    const std::vector<int>& get_cpus() const {
        return cpus;
    }

    bool is_valid() const {
        return valid;
    }

    void set_valid(bool value) {
        valid = value;
    }

private:
    bool valid;
    std::vector<int> cpus;
};

std::vector<std::string> taskset_command(const std::vector<int> cpus);

}

#endif // LIBLOOMW_RESOURCEM_H
