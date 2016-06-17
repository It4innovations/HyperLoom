#ifndef LIBLOOM_LOG
#define LIBLOOM_LOG

#include "spdlog/spdlog.h"

namespace loom {
    extern std::shared_ptr<spdlog::logger> llog;
}

#endif
