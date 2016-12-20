#include "externfile.h"

#include "../utils.h"
#include "../log.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


using namespace loom;

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

std::string loom::ExternFile::get_info()
{
    return "<ExternFile '" + filename + "'>";
}

size_t ExternFile::serialize(Worker &worker, loom::net::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    assert(0); // TODO
}

std::string ExternFile::get_filename() const
{
    return filename;
}

void ExternFile::open()
{
    llog->debug("Opening extern file {}", filename);
    int fd = ::open(filename.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        net::log_errno_abort("open");
    }

    size = lseek(fd, (size_t)0, SEEK_END);
    if (size == (size_t) -1) {
        net::log_errno_abort("lseek");
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
        llog->critical("Cannot mmap '{}' size={}", filename, size);
        net::log_errno_abort("mmap");
    }
}
