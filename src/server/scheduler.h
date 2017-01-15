#ifndef LOOM_SERVER_SCHEDULER_H
#define LOOM_SERVER_SCHEDULER_H

#include "compstate.h"

using TaskDistribution = std::unordered_map<WorkerConnection*, std::vector<TaskNode*>>;
TaskDistribution schedule(const ComputationState &cstate);

#endif
