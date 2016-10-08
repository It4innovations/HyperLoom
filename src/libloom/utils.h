#ifndef LOOM_UTILS_H
#define LOOM_UTILS_H

#include <sys/types.h>

namespace loom {

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define UV_CHECK(call) \
    { int _uv_r = (call); \
      if (unlikely(_uv_r)) { \
        loom::report_uv_error(_uv_r, __LINE__, __FILE__); \
      } \
    }

void report_uv_error(int error_code, int line_number, const char *filename) __attribute__ ((noreturn));
void log_errno_abort(const char *tmp)  __attribute__ ((noreturn));
void log_errno_abort(const char *tmp, const char *tmp2)  __attribute__ ((noreturn));


int make_path(const char *path, mode_t mode);
size_t file_size(const char *path);

}

#endif // LOOM_UTILS_H
