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
    : data(nullptr), size(0), file_id(0)
{

}

RawData::~RawData()
{
    if (data != nullptr) {
        if (file_id) {
            munmap(data, size);
        } else {
            delete [] data;
        }
    }
}



char* RawData::init_memonly(size_t size)
{
    assert(data == nullptr);
    assert(file_id == 0);
    this->size = size;
    data = new char[size];
    return data;
}

char* RawData::init_empty_file(Worker &worker, size_t size)
{
    assert(data == nullptr);

    if (file_id == 0) {
        assign_file_id();
    }

    this->size = size;

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

void RawData::assign_file_id()
{
    assert(file_id == 0);
    file_id = file_id_counter++;
}

void RawData::init_from_file(Worker &worker)
{
    assert(data == nullptr);
    if (file_id == 0) {
        assign_file_id();
    }
    size = file_size(get_filename(worker).c_str());
}

std::string RawData::get_filename(Worker &worker) const
{
    assert(file_id);
    std::stringstream s;
    s << worker.get_work_dir() << "data/" << file_id;
    return s.str();
}

void RawData::open(Worker &worker)
{
    assert(file_id);

    int fd = ::open(get_filename(worker).c_str(), O_RDONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        llog->critical("Cannot open data {}", get_filename(worker));
        log_errno_abort("open");
    }
    map(fd, false);
    ::close(fd);
}


void RawData::map(int fd, bool write)
{
    assert(data == nullptr);
    assert(file_id);
    assert(fd >= 0);

    int flags = PROT_READ;
    if (write) {
        flags |= PROT_WRITE;
    }
    data = (char*) mmap(0, size, flags, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        llog->critical("Cannot mmap data file_id={}", file_id);
        log_errno_abort("mmap");
    }
}

std::string RawData::get_info()
{
    return "RawData";
}

/*void RawData::init_message(Worker &worker, loomcomm::Data &msg)
{

}*/

void RawData::serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    buffer.add(data_ptr, get_raw_data(worker), size);
}

RawDataUnpacker::~RawDataUnpacker()
{

}

bool RawDataUnpacker::init(Worker &worker, Connection &connection, const loomcomm::Data &msg)
{
    auto data = std::make_unique<RawData>();
    assert(msg.has_size());
    auto size = msg.size();
    pointer = data->init_empty_file(worker, size);
    this->data = std::move(data);
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
