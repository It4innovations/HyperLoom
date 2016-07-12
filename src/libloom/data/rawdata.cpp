#include "rawdata.h"

#include "../log.h"
#include "../utils.h"
#include "../worker.h"

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
    llog->debug("Disposing raw data filename={}", filename);

    if (filename.empty()) {
        assert(data == nullptr);
    } else {
        if (munmap(data, size)) {
            log_errno_abort("munmap");
        }
        if (unlink(filename.c_str())) {
            log_errno_abort("unlink");
        }
    }
}



/*char* RawData::init_memonly(size_t size)
{
    assert(data == nullptr);
    assert(file_id == 0);
    this->size = size;
    data = new char[size];
    return data;
}*/

char* RawData::init_empty_file(Worker &worker, size_t size)
{
    assert(data == nullptr);

    if (filename.empty()) {
        assign_filename(worker);
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

void RawData::assign_filename(Worker &worker)
{
    assert(filename.empty());
    int file_id = file_id_counter++;
    std::stringstream s;
    s << worker.get_work_dir() << "data/" << file_id;
    filename = s.str();
}

void RawData::init_from_file(Worker &worker)
{
    assert(data == nullptr);
    if (filename.empty()) {
        assign_filename(worker);
    }
    size = file_size(filename.c_str());
}

std::string RawData::get_filename() const
{
    return filename;
}

void RawData::open(Worker &worker)
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
}


void RawData::map(int fd, bool write)
{
    assert(data == nullptr);
    assert(!filename.empty());
    assert(fd >= 0);

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
   return "RawData";
}

void RawData::init_message(Worker &worker, loomcomm::Data &msg) const
{
   msg.set_arg0_u64(size);
}

void RawData::serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    buffer.add(data_ptr, get_raw_data(worker), size);
}

RawDataUnpacker::~RawDataUnpacker()
{

}

bool RawDataUnpacker::init(Worker &worker, Connection &connection, const loomcomm::Data &msg)
{
    this->data = std::make_shared<RawData>();
    RawData &data = static_cast<RawData&>(*this->data);
    assert(msg.has_arg0_u64());
    auto size = msg.arg0_u64();
    pointer = data.init_empty_file(worker, size);
    if (size == 0) {
        return true;
    }
    connection.set_raw_read(size);
    return false;
}

void RawDataUnpacker::on_data_chunk(const char *data, size_t size)
{
    memcpy(pointer, data, size);
    pointer += size;
}

bool RawDataUnpacker::on_data_finish(Connection &connection)
{
    return true;
}
