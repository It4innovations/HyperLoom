#include "externfile.h"

#include "../utils.h"
#include "../log.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


using namespace loom;

ExternFile::ExternFile(const std::string &filename)
    : data(nullptr), filename(filename)
{
    size = file_size(filename.c_str());
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

void ExternFile::serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
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
        log_errno_abort("open");
    }
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
        log_errno_abort("mmap");
    }
}
