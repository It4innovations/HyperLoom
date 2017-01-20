
#include "index.h"
#include "rawdata.h"

#include "libloom/log.h"
#include "../worker.h"

using namespace loom;
using namespace loom::base;

Index::Index(Worker &worker, const DataPtr &data, size_t length, std::unique_ptr<size_t[]> &&indices)
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

size_t Index::serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const
{
    size_t size = length * sizeof(size_t);
    buffer.add(std::make_unique<base::SizeBufferItem>(size + sizeof(Id)));
    buffer.add(std::make_unique<base::IdBufferItem>(data->get_type_id(worker)));
    buffer.add(std::make_unique<DataBufferItem>(data_ptr, reinterpret_cast<char*>(indices.get()), size));
    return 1 + data->serialize(worker, buffer, data);
}

IndexUnpacker::IndexUnpacker(Worker &worker) : worker(worker)
{

}

DataUnpacker::Result IndexUnpacker::on_message(const char *data, size_t size)
{
   if (unpacker == nullptr) {
       Id id = *(reinterpret_cast<const Id*>(data));
       data += sizeof(Id);
       size -= sizeof(Id);

       length = size / sizeof(size_t);
       indices = std::make_unique<size_t[]>(length);
       memcpy(indices.get(), data, size);
       unpacker = worker.get_unpacker(id);
       return unpacker->get_initial_mode();
   } else {
       return unpacker->on_message(data, size);
   }
}

DataUnpacker::Result IndexUnpacker::on_stream_data(const char *data, size_t size, size_t remaining)
{
    assert(unpacker);
    return unpacker->on_stream_data(data, size, remaining);
}


DataPtr IndexUnpacker::finish()
{
    return std::make_shared<Index>(worker, unpacker->finish(), length, std::move(indices));
}
