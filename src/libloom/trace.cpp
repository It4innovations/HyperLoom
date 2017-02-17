#include "trace.h"

loom::base::Trace::Trace(uv_loop_t *loop)
{
    this->loop = loop;
    if (loop) {
        base_time = uv_now(loop);
        last_timestamp = base_time - 1;
    } else {
        base_time = 0;
        last_timestamp = 0;
    }
    stream << std::hex;
}

bool loom::base::Trace::open(const std::string &filename)
{
    stream.close();
    stream.open(filename);
    return stream.is_open();
}

void loom::base::Trace::trace_time()
{
    uint64_t now = uv_now(loop);
    if (now == last_timestamp) {
        return;
    }
    last_timestamp = now;
    stream << "T " << now - base_time << std::endl;
}
