#ifndef LOOM_SERVER_TRACE_H
#define LOOM_SERVER_TRACE_H

#include "workerconn.h"
#include "tasknode.h"

#include <libloom/trace.h>

class ServerTrace : public loom::base::Trace
{
public:
    using Trace::Trace;

    void trace_worker(WorkerConnection *wc);
    void trace_scheduler_start();
    void trace_scheduler_end();
};

#endif
