#include "externfile.h"

#include "../utils.h"
#include "libloom/log.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


using namespace loom;
using namespace loom::base;

std::string ExternFile::get_type_name() const
{
    return "loom/file";
}

ExternFile::ExternFile(const std::string &filename)
    : data(nullptr), filename(filename)
{
    open();
}

ExternFile::~ExternFile()
{
    if (data) {
        munmap(data, size);
    }
}

size_t ExternFile::get_size() const {
    return size;
}

const char *ExternFile::get_raw_data() const
{
    return data;
}

bool ExternFile::has_raw_data() const {
    return true;
}

std::string loom::ExternFile::get_info() const
{
    return "<ExternFile '" + filename + "'>";
}

size_t ExternFile::serialize(Worker &worker, loom::base::SendBuffer &buffer, DataPtr &data_ptr) const
{
    assert(0); // TODO
}

std::string ExternFile::get_filename() const
{
    return filename;
}

void ExternFile::open()
{
    logger->debug("Opening extern file {}", filename);
    int fd = ::open(filename.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        base::log_errno_abort("open");
    }

    size = lseek(fd, (size_t)0, SEEK_END);
    if (size == (size_t) -1) {
        base::log_errno_abort("lseek");
    }
    lseek(fd, 0, SEEK_SET);


    map(fd, false);
    ::close(fd);
}

void ExternFile::map(int fd, bool write)
{
    assert(data == nullptr);
    assert(fd >= 0);

    int flags = PROT_READ;
    if (write) {
        flags |= PROT_WRITE;
    }
    data = (char*) mmap(0, size, flags, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        logger->critical("Cannot mmap '{}' size={}", filename, size);
        base::log_errno_abort("mmap");
    }
}
