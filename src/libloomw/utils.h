#ifndef LIBLOOMW_UTILS_H
#define LIBLOOMW_UTILS_H

#include <sys/types.h>
#include <libloom/log.h>

#include "libloom/socket.h"
#include "libloom/sendbuffer.h"

namespace loom {

int make_path(const char *path, mode_t mode);
size_t file_size(const char *path);

}

#endif // LIBLOOMW_UTILS_H
