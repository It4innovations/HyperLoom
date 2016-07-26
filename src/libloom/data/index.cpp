
#include "index.h"
#include "rawdata.h"

#include "../log.h"
#include "../sendbuffer.h"
#include "../worker.h"

using namespace loom;

Index::Index(Worker &worker, std::shared_ptr<Data> &data, size_t length, std::unique_ptr<size_t[]> indices)
    : worker(worker), data(data), length(length), indices(std::move(indices))
{

}

Index::~Index()
{
    llog->debug("Disposing index");
}

size_t Index::get_length()
{
    return length;
}

size_t Index::get_size()
{
    return data->get_size() + sizeof(size_t) * (length + 1);
}

std::string Index::get_info()
{
    return "Index";
}

std::shared_ptr<Data> Index::get_at_index(size_t index)
{
    assert (index < length);
    size_t addr = indices[index];
    char *p1 = this->data->get_raw_data(worker);
    p1 += addr;
    size_t size = indices[index + 1] - addr;

    auto data = std::make_shared<RawData>();
    data->init_empty_file(worker, size);
    char *p2 = data->get_raw_data(worker);
    memcpy(p2, p1, size);
    return data;
}

std::shared_ptr<Data> Index::get_slice(size_t from, size_t to)
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

    char *p1 = this->data->get_raw_data(worker);
    p1 += from_addr;

    size_t size = to_addr - from_addr;

    auto data = std::make_shared<RawData>();
    data->init_empty_file(worker, size);
    char *p2 = data->get_raw_data(worker);
    memcpy(p2, p1, size);
    return data;
}

void Index::serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
    buffer.add(data_ptr, (char*) &indices[0], sizeof(size_t) * (length + 1));
    data->serialize(worker, buffer, data);
}

IndexUnpacker::~IndexUnpacker()
{

}

bool IndexUnpacker::init(Worker &worker, Connection &connection, const loomcomm::Data &msg)
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
    std::shared_ptr<Data> &inner_data = unpacker->get_data();
    data = std::make_shared<Index>(*worker, inner_data, length, std::move(indices));
}
