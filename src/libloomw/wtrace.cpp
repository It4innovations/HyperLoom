
#include "wtrace.h"

#include <libloom/log.h>

#include <unistd.h>
#include <sys/sysinfo.h>

using namespace loom;

double read_cpu_time(long hz)
{
    static bool error_reported = false;

    std::ifstream f("/proc/stat");

    std::string ignore;
    long user;
    long nice;
    long sys;
    f >> ignore >> user >> nice >> sys;
    if (f.fail()) {
        if (!error_reported) {
            error_reported = true;
            base::logger->warn("Parsing /proc/stat failed");
        }
    }
    return static_cast<double>(user + nice + sys) / hz;
}

int read_mem_usage_percent()
{
    struct sysinfo mem_info;
    if (sysinfo (&mem_info) < 0) {
        return 0;
    }

    double total = mem_info.totalram - mem_info.freeram;
    return static_cast<int>(total / mem_info.totalram * 100);
}

WorkerTrace::WorkerTrace(uv_loop_t *loop)
    : Trace(loop)
{
    hz = sysconf(_SC_CLK_TCK);
    if (hz < 0) {
        base::logger->warn("sysconf(_SC_CLK_TCK) failed; setting 100 as default");
        hz = 100;
    }
    base::logger->debug("Setting HZ value to: {}", hz);

    n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_cpus < 1) {
        n_cpus = 1;
    }

    last_monitoring_time = base_time;
    last_cpu_time = read_cpu_time(hz);
}

void WorkerTrace::trace_task_started(const Task &task)
{
    trace_time();
    entry('S', task.get_id());
}

void WorkerTrace::trace_task_finished(const Task &task)
{
    trace_time();
    entry('F', task.get_id());
}

void WorkerTrace::trace_monitoring()
{
    auto now = uv_now(loop);
    double delta = (now - last_monitoring_time);
    if (delta < 0.001) {
        return;
    }
    last_monitoring_time = now;

    // divide 10.000 = divide by (1000 (because of ms) * 100 because of percent)
    delta /= 100000;
    delta *= n_cpus;

    double new_cpu_time = read_cpu_time(hz);
    int percent = static_cast<int>((new_cpu_time - last_cpu_time) / delta);
    last_cpu_time = new_cpu_time;

    trace_time();
    entry('M', percent, read_mem_usage_percent());
}

void WorkerTrace::trace_send(base::Id id, size_t size, const std::string &target)
{
    trace_time();
    entry('D', id, size, target);
}

void WorkerTrace::trace_receive(base::Id id, size_t size, const std::string &target)
{
    trace_time();
    entry('R', id, size, target);
}
