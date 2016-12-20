#include "log.h"

namespace loom {
    std::shared_ptr<spdlog::logger> llog = spdlog::stdout_logger_mt("loom", true);
}
