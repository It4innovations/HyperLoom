#ifndef LIBLOOMNET_LOG_H
#define LIBLOOMNET_LOG_H

#include <sys/types.h>
#include "spdlog/spdlog.h"

namespace loom {
namespace net {

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define UV_CHECK(call) \
    { int _uv_r = (call); \
      if (unlikely(_uv_r)) { \
        loom::net::report_uv_error(_uv_r, __LINE__, __FILE__); \
      } \
    }

void report_uv_error(int error_code, int line_number, const char *filename) __attribute__ ((noreturn));
void log_errno_abort(const char *tmp)  __attribute__ ((noreturn));
void log_errno_abort(const char *tmp, const char *tmp2)  __attribute__ ((noreturn));

extern std::shared_ptr<spdlog::logger> log;

}
}

#endif // LOOM_UTILS_H
