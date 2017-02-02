#include "resourcem.h"

#include "libloom/log.h"

using loom::base::logger;

loom::ResourceManager::ResourceManager()
    : total_cpus(0), free_cpus(0), zero_cost_slots(0)
{

}

void loom::ResourceManager::init(int n_cpus)
{
    assert(total_cpus == 0);
    if (n_cpus == 0) {
        n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        if (n_cpus <= 0) {
            logger->critical("Cannot detect number of CPUs");
            exit(1);
        }
        logger->debug("Autodetection of CPUs: {}", n_cpus);
    }

    total_cpus = n_cpus;

    this->free_cpus.resize(n_cpus);
    for (int i = 0; i < n_cpus; i++) {
        free_cpus[i] = i;
    }

    zero_cost_slots = n_cpus * 2 + 1;
    logger->info("Number of CPUs for worker: {}", n_cpus);
}

loom::ResourceAllocation loom::ResourceManager::allocate(int n_cpus)
{
    ResourceAllocation result;
    if (n_cpus == 0) {
        if (zero_cost_slots > 0) {
            zero_cost_slots--;
            result.set_valid(true);
        }
        return result;
    }

    int fc = free_cpus.size();
    if (fc >= n_cpus) {
        for (int i = 0; i < n_cpus; i++) {
            fc--;
            result.add_cpu(free_cpus[fc]);
            free_cpus.pop_back();
        }
        result.set_valid(true);
    }
    return result;
}

void loom::ResourceManager::free(loom::ResourceAllocation &ra)
{
    assert(ra.is_valid());
    ra.set_valid(false);
    auto &cpus = ra.get_cpus();
    if (cpus.empty()) {
        zero_cost_slots++;
        return;
    }

    for (auto cpu_id : cpus) {
        free_cpus.push_back(cpu_id);
    }

    assert(free_cpus.size() <= static_cast<size_t>(total_cpus));
}
