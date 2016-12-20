#include "rawdata.h"

#include "../log.h"
#include "../utils.h"
#include "../worker.h"
#include "libloomnet/sendbuffer.h"

#include <sstream>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

using namespace loom;

size_t RawData::file_id_counter = 1;

RawData::RawData()
    : data(nullptr), size(0)
{
}

RawData::~RawData()
{
    llog->debug("Disposing raw data filename={} size={}", filename, size);

    if (filename.empty()) {
        assert(data == nullptr);
    } else {
        if (size > 0 && munmap(data, size)) {
            log_errno_abort("munmap");
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

char* RawData::init_empty(const std::string &work_dir, size_t size)
{
    assert(data == nullptr);

    if (filename.empty()) {
        assign_filename(work_dir);
    }

    this->size = size;

    int fd = ::open(filename.c_str(), O_CREAT | O_RDWR,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        llog->critical("Cannot open data {} for writing", filename);
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

void RawData::assign_filename(const std::string &work_dir)
{
    assert(filename.empty());
    int file_id = file_id_counter++;
    std::stringstream s;
    s << work_dir << "data/" << file_id;
    filename = s.str();
}

void RawData::init_from_file(const std::string &work_dir)
{
    assert(data == nullptr);
    if (filename.empty()) {
        assign_filename(work_dir);
    }
    size = file_size(filename.c_str());
}

std::string RawData::get_filename() const
{
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
        llog->critical("Cannot open data {}", filename);
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
        llog->critical("Cannot mmap data filename={}", filename);
        log_errno_abort("mmap");
    }
}

std::string RawData::get_info()
{
    return "RawData file=" + filename;
}

void RawData::init_from_string(const std::string &work_dir, const std::string &str)
{
    auto size = str.size();
    char *mem = init_empty(work_dir, size);
    memcpy(mem, str.c_str(), size);
}

void RawData::init_from_mem(const std::string &work_dir, const void *ptr, size_t size)
{
    char *mem = init_empty(work_dir, size);
    memcpy(mem, ptr, size);
}

size_t RawData::serialize(Worker &worker, loom::net::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    buffer.add(std::make_unique<net::SizeBufferItem>(size));
    buffer.add(std::make_unique<DataBufferItem>(data_ptr, get_raw_data(), size));
    return 1;
}

RawDataUnpacker::RawDataUnpacker(Worker &worker) : ptr(nullptr), worker_dir(worker.get_work_dir())
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
        ptr = obj->init_empty(worker_dir, total_size);
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

std::shared_ptr<Data> RawDataUnpacker::finish()
{
   return result;
}
