#ifndef LIBLOOM_TRACE_H
#define LIBLOOM_TRACE_H

#include <fstream>
#include <uv.h>

namespace loom {
namespace base {

class Trace
{
public:
    Trace(uv_loop_t *loop);
    bool open(const std::string &filename);
    void trace_time();
    void flush() {
        stream.flush();
    }

    template<typename First, typename... Rest>
    void entry(First&& value, Rest &&...rest)
    {
        stream << std::forward<First>(value);
        return _entry(std::forward<Rest>(rest)...);
    }

protected:

    template<typename First, typename... Rest>
    void _entry(First&& value, Rest &&...rest)
    {
        stream << ' ' << std::forward<First>(value);
        return _entry(std::forward<Rest>(rest)...);
    }

    void _entry() {
        stream << std::endl;
    }


    uv_loop_t *loop;
    uint64_t base_time;
    uint64_t last_timestamp;
    std::ofstream stream;
};

}
}

#endif // LIBLOOM_TYPES_H
