#include "log.h"

#include "uv.h"

#include <errno.h>
#include <stdlib.h>
#include <limits.h>

using namespace loom::base;

namespace loom {
namespace base {
std::shared_ptr<spdlog::logger> logger = spdlog::stdout_logger_mt("net", true);
}}

void loom::base::report_uv_error(int error_code, int line_number, const char *filename)
{
    logger->critical("libuv fail: {} ({}:{})", uv_strerror(error_code), filename, line_number);
    exit(1);
}

void loom::base::log_errno_abort(const char *tmp)
{
    logger->critical("{}: {}", tmp, strerror(errno));
    abort();
}

void loom::base::log_errno_abort(const char *tmp, const char *tmp2)
{
    logger->critical("{}: {} ({})", tmp, strerror(errno), tmp2);
    abort();
}
