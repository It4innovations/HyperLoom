#include "fsutils.h"

#include "libloom/log.h"
#include "uv.h"

#include <errno.h>
#include <stdlib.h>
#include <limits.h>

using namespace loom::base;

static bool is_directory(const char *path)
{
    struct stat info;
    if (!stat(path, &info))
    {
        return info.st_mode & S_IFDIR;

    }
    return false;
}

size_t loom::base::file_size(const char *path)
{
    struct stat info;
    if (stat(path, &info))
    {
        log_errno_abort("file_size");
    }
    return info.st_size;
}


int loom::base::make_path(const char *path, mode_t mode)
{
    int len = strlen(path);
    assert(len < PATH_MAX);
    char tmp[PATH_MAX];
    strncpy(tmp, path, PATH_MAX);

    for (int i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = 0;
            if (!is_directory(tmp)) {
                int r = mkdir(tmp, mode);
                if (r != 0) {
                    return r;
                }
            }
            tmp[i] = '/';
        }
    }

    if (tmp[len - 1] != '/') {
        if (!is_directory(tmp)) {
            int r = mkdir(tmp, mode);
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

bool loom::base::file_exists(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}
