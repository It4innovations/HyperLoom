#include "log.h"

#include "uv.h"

#include <errno.h>
#include <stdlib.h>
#include <limits.h>

using namespace loom::net;

namespace loom {
namespace net {
std::shared_ptr<spdlog::logger> log = spdlog::stdout_logger_mt("net", true);
}}

void loom::net::report_uv_error(int error_code, int line_number, const char *filename)
{
    log->critical("libuv fail: {} ({}:{})", uv_strerror(error_code), filename, line_number);
    exit(1);
}

void loom::net::log_errno_abort(const char *tmp)
{
    log->critical("{}: {}", tmp, strerror(errno));
    abort();
}

void loom::net::log_errno_abort(const char *tmp, const char *tmp2)
{
    log->critical("{}: {} ({})", tmp, strerror(errno), tmp2);
    abort();
}
