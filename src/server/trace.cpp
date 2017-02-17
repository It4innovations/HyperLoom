
#include "trace.h"

/*void ServerTrace::trace_task_end(const TaskNode &node, WorkerConnection *wc)
{
    trace_time();
    entry("E", node.get_id(), wc->get_worker_id());
}


void ServerTrace::trace_task_start(const TaskNode &node, WorkerConnection *wc)
{
    trace_time();
    entry("S", node.get_id(), wc->get_worker_id(), node.get_client_id());
}*/

void ServerTrace::trace_worker(WorkerConnection *wc)
{
    entry("WORKER", wc->get_worker_id(), wc->get_address());
}

void ServerTrace::trace_scheduler_start()
{
    trace_time();
    entry('s');
}


void ServerTrace::trace_scheduler_end()
{
    trace_time();
    entry('e');
}
