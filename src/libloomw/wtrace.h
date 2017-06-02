#ifndef LIBLOOMW_WTRACE_H
#define LIBLOOMW_WTRACE_H
#define LOOM_SERVER_TRACE_H

#include <libloom/trace.h>
#include "task.h"

namespace loom {

class WorkerTrace : public loom::base::Trace
{
public:
    WorkerTrace(uv_loop_t *loop);
    void trace_task_started(const Task &task);
    void trace_task_finished(const Task &task);
    void trace_monitoring();
    void trace_send(base::Id id, size_t size, const std::string &target);
    void trace_receive(base::Id id, size_t size, const std::string &target);

private:
    uint64_t last_monitoring_time;
    double last_cpu_time;
    long hz;
    int n_cpus;
};

}

#endif // LIBLOOMW_WTRACE_H
