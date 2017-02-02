#ifndef LIBLOOMW_RESOURCEM_H
#define LIBLOOMW_RESOURCEM_H

#include "resalloc.h"

namespace loom {

class ResourceManager
{
public:
    ResourceManager();
    void init(int n_cpus);

    int get_total_cpus() const {
        return total_cpus;
    }

    ResourceAllocation allocate(int n_cpus);
    void free(ResourceAllocation &ra);

private:
    int total_cpus;
    std::vector<int> free_cpus;
    int zero_cost_slots;
};


}

#endif // LIBLOOMW_RESOURCEM_H
