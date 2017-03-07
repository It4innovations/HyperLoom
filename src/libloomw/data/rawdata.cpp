#include "rawdata.h"

#include "libloom/log.h"
#include "libloom/sendbuffer.h"
#include "libloom/fsutils.h"
#include "../worker.h"


#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

using namespace loom;
using namespace loom::base;

RawData::RawData()
    : data(nullptr), size(0), is_mmap(false)
{
}

RawData::~RawData()
{
    logger->debug("Disposing raw data filename={} size={}", filename, size);

    if (filename.empty()) {
        if (data) {
            assert(!is_mmap);
            delete [] data;
            data = nullptr;
        }
    } else {
        if (data) {
            if (!is_mmap) {
                delete [] data;
                data = nullptr;
            } else {
                if (munmap(data, size)) {
                    log_errno_abort("munmap");
                }
            }
        }
        if (unlink(filename.c_str())) {
            log_errno_abort("unlink");
        }
    }
}

std::string RawData::get_type_name() const
{
    return "loom/data";
}

size_t RawData::get_size() const {
    return size;
}

const char *RawData::get_raw_data() const
{
    std::lock_guard<std::mutex> lock(mutex);
    if (data == nullptr) {
        assert(is_mmap);
        open();
    }
    return data;
}

bool RawData::has_raw_data() const {
    return true;
}

char* RawData::init_empty(Globals &globals, size_t size)
{
    assert(data == nullptr);
    assert(filename.empty());
    this->size = size;
    is_mmap = size > MMAP_MIN_SIZE;
    if (!is_mmap) {
        data = new char[size];
        return data;
    }

    filename = globals.create_data_filename();

    int fd = ::open(filename.c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        logger->critical("Cannot open data {} for writing", filename);
        log_errno_abort("open");
    }

    if (size > 0) {
        if (lseek(fd, size - 1, SEEK_SET) == -1) {
            log_errno_abort("lseek");
        }
        if (write(fd, "", 1) != 1) {
            log_errno_abort("write");
        }
        map(fd, true);
    }
    ::close(fd);
    return data;
}

std::string RawData::assign_filename(Globals &globals)
{
    assert(filename.empty());
    filename = globals.create_data_filename();
    return filename;
}

void RawData::init_from_file()
{
    assert(!filename.empty());
    assert(data == nullptr);
    is_mmap = true;
    size = file_size(filename.c_str());
}

std::string RawData::map_as_file(Globals &globals) const
{
    std::lock_guard<std::mutex> lock(mutex);
    if (filename.empty()) {
        assert (data);
        assert (!is_mmap);
        filename = globals.create_data_filename();
        std::ofstream f(filename.c_str());
        f.write(data, size);
    }
    return filename;
}

void RawData::open() const
{
    if (size == 0) {
        return;
    }
    assert(!filename.empty());
    int fd = ::open(filename.c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        logger->critical("Cannot open data {}", filename);
        log_errno_abort("open");
    }
    map(fd, false);
    ::close(fd);
    assert(data);
}


void RawData::map(int fd, bool write) const
{
    assert(data == nullptr);
    assert(!filename.empty());
    assert(fd >= 0);

    if (size == 0) {
        return;
    }

    int flags = PROT_READ;
    if (write) {
        flags |= PROT_WRITE;
    }
    data = (char*) mmap(0, size, flags, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        logger->critical("Cannot mmap data filename={}", filename);
        log_errno_abort("mmap");
    }
}

std::string RawData::get_info() const
{
    return "RawData file=" + filename;
}

void RawData::init_from_string(Globals &globals, const std::string &str)
{
    auto size = str.size();
    char *mem = init_empty(globals, size);
    memcpy(mem, str.c_str(), size);
}

void RawData::init_from_mem(Globals &globals, const void *ptr, size_t size)
{
    char *mem = init_empty(globals, size);
    memcpy(mem, ptr, size);
}

size_t RawData::serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const
{
    buffer.add(std::make_unique<base::SizeBufferItem>(size));
    buffer.add(std::make_unique<DataBufferItem>(data_ptr, get_raw_data(), size));
    return 1;
}

RawDataUnpacker::RawDataUnpacker(Worker &worker) : ptr(nullptr), globals(worker.get_globals())
{

}

RawDataUnpacker::~RawDataUnpacker()
{

}

DataUnpacker::Result RawDataUnpacker::get_initial_mode()
{
    return STREAM;
}

DataUnpacker::Result RawDataUnpacker::on_stream_data(const char *data, size_t size, size_t remaining)
{
    if (ptr == nullptr) {
        auto obj = std::make_shared<RawData>();
        size_t total_size = size + remaining;
        ptr = obj->init_empty(globals, total_size);
        memcpy(ptr, data, size);
        ptr += size;
        result = obj;
    } else {
        memcpy(ptr, data, size);
        ptr += size;
    }

    if (remaining == 0) {
        return FINISHED;
    } else {
        return STREAM;
    }
}

DataPtr RawDataUnpacker::finish()
{
    return result;
}
