#include "data.h"
#include "worker.h"
#include "log.h"

#include <sstream>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


using namespace loom;

Data::Data(int id)
    : id(id), data(nullptr), size(0), in_file(false)
{

}

Data::~Data()
{
    if (data != nullptr) {
        if (in_file) {
            munmap(data, size);
        } else {
            delete [] data;
        }
    }
}

char* Data::init_memonly(size_t size)
{
    assert(data == nullptr);
    this->size = size;
    in_file = false;
    data = new char[size];
    return data;
}

char* Data::init_empty_file(Worker &worker, size_t size)
{
    assert(data == nullptr);
    this->size = size;
    in_file = true;

    int fd = ::open(get_filename(worker).c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        llog->critical("Cannot open data {} for writing", get_filename(worker));
        log_errno_abort("open");
    }

    if (size > 0) {
        if (!lseek(fd, size - 1, SEEK_SET)) {
            log_errno_abort("lseek");
        }
        if (write(fd, "", 1) != 1) {
            log_errno_abort("write");
        }
    }
    map(fd, true);
    ::close(fd);

    return data;
}

void Data::init_from_file(Worker &worker)
{
    assert(data == nullptr);
    in_file = true;
    size = file_size(get_filename(worker).c_str());
}

std::string Data::get_filename(Worker &worker) const
{
    std::stringstream s;
    s << worker.get_work_dir() << "data/" << id;
    return s.str();
}

void Data::make_symlink(Worker &worker, const std::string &path) const
{
    assert(in_file);
    if (symlink(get_filename(worker).c_str(), path.c_str())) {
        log_errno_abort("symlink");
    }
}

void Data::open(Worker &worker)
{
    assert(in_file);

    int fd = ::open(get_filename(worker).c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        llog->critical("Cannot open data {}", get_filename(worker));
        log_errno_abort("open");
    }
    struct stat finfo;
    memset(&finfo, 0, sizeof(finfo));
    if (fstat(fd, &finfo) == -1)
    {
        log_errno_abort("fstat");
    }
    size = finfo.st_size;
    map(fd, false);
    ::close(fd);
}


void Data::map(int fd, bool write)
{
    assert(data == nullptr);
    assert(in_file);
    assert(fd >= 0);

    int flags = PROT_READ;
    if (write) {
        flags |= PROT_WRITE;
    }
    data = (char*) mmap(0, size, flags, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        log_errno_abort("mmap");
    }
}
