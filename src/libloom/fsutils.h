#ifndef LIBLOOM_FSUTILS_H
#define LIBLOOM_FSUTILS_H

#include <sys/types.h>
#include <libloom/log.h>

#include "libloom/socket.h"
#include "libloom/sendbuffer.h"

namespace loom {
namespace base {

int make_path(const char *path, mode_t mode);
size_t file_size(const char *path);
bool file_exists(const char *path);

}
}

#endif // LIBLOOM_FSUTILS_H
