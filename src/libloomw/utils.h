#ifndef LOOM_UTILS_H
#define LOOM_UTILS_H

#include <sys/types.h>
#include <libloom/log.h>

#include "libloom/socket.h"
#include "libloom/sendbuffer.h"

namespace loom {

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

void report_uv_error(int error_code, int line_number, const char *filename) __attribute__ ((noreturn));
void log_errno_abort(const char *tmp)  __attribute__ ((noreturn));
void log_errno_abort(const char *tmp, const char *tmp2)  __attribute__ ((noreturn));


int make_path(const char *path, mode_t mode);
size_t file_size(const char *path);

}

#endif // LOOM_UTILS_H
