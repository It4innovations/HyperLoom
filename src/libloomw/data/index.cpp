
#include "index.h"
#include "rawdata.h"

#include "libloom/log.h"
#include "../worker.h"

using namespace loom;
using namespace loom::base;

Index::Index(Worker &worker, DataPtr &data, size_t length, std::unique_ptr<size_t[]> indices)
    : worker(worker), data(data), length(length), indices(std::move(indices))
{

}

Index::~Index()
{
   logger->debug("Disposing index");
}

std::string Index::get_type_name() const
{
   return "loom/index";
}

size_t Index::get_length() const
{
    return length;
}

size_t Index::get_size() const
{
    return data->get_size() + sizeof(size_t) * (length + 1);
}

std::string Index::get_info() const
{
    return "Index";
}

DataPtr Index::get_at_index(size_t index) const
{
    assert (index < length);
    size_t addr = indices[index];
    const char *p1 = this->data->get_raw_data();
    p1 += addr;
    size_t size = indices[index + 1] - addr;

    auto data = std::make_shared<RawData>();
    char *p2 = data->init_empty(worker.get_work_dir(), size);
    memcpy(p2, p1, size);
    return data;
}

DataPtr Index::get_slice(size_t from, size_t to) const
{
    if (from > length) {
        from = length;
    }

    if (to > length) {
        to = length;
    }

    if (from > to) {
        from = to;
    }

    size_t from_addr = indices[from];
    size_t to_addr = indices[to];

    const char *p1 = this->data->get_raw_data();
    p1 += from_addr;

    size_t size = to_addr - from_addr;

    auto data = std::make_shared<RawData>();
    char *p2 = data->init_empty(worker.get_work_dir(), size);
    memcpy(p2, p1, size);
    return data;
}

size_t Index::serialize(Worker &worker, loom::base::SendBuffer &buffer, DataPtr &data_ptr) const
{
    logger->critical("Index::serialize_data");
    abort();
}

/*bool IndexUnpacker::init(Worker &worker, Connection &connection, const loomcomm::Data &msg)
{
    this->worker = &worker;
    length = msg.length();
    indices = std::make_unique<size_t[]>(length + 1);
    indices_ptr = (char*) &indices[0];
    connection.set_raw_read((length + 1) * sizeof(size_t));
    return false;
}

bool IndexUnpacker::on_message(Connection &connection, const char *data, size_t size)
{
    if (unpacker) {
        if (unpacker->on_message(connection, data, size)) {
            finish_data();
            return true;
        }
        return false;
    }
    loomcomm::Data msg;
    assert(msg.ParseFromArray(data, size));
    unpacker = worker->unpack(msg.type_id());
    if (unpacker->init(*worker, connection, msg)) {
        finish_data();
        return true;
    } else {
       return false;
    }
}

void IndexUnpacker::on_data_chunk(const char *data, size_t size)
{
    if (unpacker) {
        unpacker->on_data_chunk(data, size);
        return;
    }
    memcpy(indices_ptr, data, size);
    indices_ptr += size;
}

bool IndexUnpacker::on_data_finish(Connection &connection)
{
    if (unpacker) {
        if (unpacker->on_data_finish(connection)) {
            finish_data();
            return true;
        }
    }
    return false;
}

void IndexUnpacker::finish_data()
{
    DataPtr &inner_data = unpacker->get_data();
    data = std::make_shared<Index>(*worker, inner_data, length, std::move(indices));
}
*/

IndexUnpacker::IndexUnpacker()
{
   assert(0);
}

IndexUnpacker::~IndexUnpacker()
{

}

DataUnpacker::Result IndexUnpacker::on_message(const char *data, size_t size)
{
   assert(0);
}

DataPtr IndexUnpacker::finish()
{
   assert(0);
}
