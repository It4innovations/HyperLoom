#include "utils.h"

#include "log.h"
#include "uv.h"

#include <errno.h>
#include <stdlib.h>
#include <limits.h>

using namespace loom;

void loom::report_uv_error(int error_code, int line_number, const char *filename)
{
    llog->critical("libuv fail: {} ({}:{})", uv_strerror(error_code), filename, line_number);
    exit(1);
}


static bool is_directory(const char *path)
{
    struct stat info;
    if (!stat(path, &info))
    {
        return info.st_mode & S_IFDIR;

    }
    return false;
}

size_t loom::file_size(const char *path)
{
    struct stat info;
    if (stat(path, &info))
    {
        log_errno_abort("file_size");
    }
    return info.st_size;
}


int loom::make_path(const char *path, mode_t mode)
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

void loom::log_errno_abort(const char *tmp)
{
    llog->critical("{}: {}", tmp, strerror(errno));
    exit(1);
}
