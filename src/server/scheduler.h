#ifndef LOOM_SERVER_SCHEDULER_H
#define LOOM_SERVER_SCHEDULER_H

#include "compstate.h"

using TaskDistribution = std::unordered_map<WorkerConnection*, std::vector<loom::Id>>;

class Scheduler
{

    struct SUnit { // Scheduling Unit
        int n_cpus;
        int64_t bonus_score;
        int64_t expected_size;
        std::vector<size_t> inputs;
        std::vector<size_t> next_inputs;
        std::vector<loom::Id> nexts;
        std::vector<loom::Id> ids;        
    };

    struct Worker {
        int free_cpus;
        int max_cpus;
        WorkerConnection *wc;
    };

    struct UW {
        size_t unit_index;
        size_t worker_index;
    };

    struct DataObj {
        size_t size;
        std::vector<size_t> owners;
    };

public:
    Scheduler(ComputationState &cstate);

    TaskDistribution schedule();

private:


    std::vector<Worker> workers;
    std::vector<SUnit> s_units;
    std::vector<DataObj> data;


    bool find_best(UW &uw);
    void apply_uw(const UW &uw);
    void create_derived_units();

    static Scheduler::SUnit merge_units(const SUnit &u1, const SUnit &u2);
};


#endif
