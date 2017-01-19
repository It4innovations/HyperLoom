
#include "array.h"
#include "libloom/compat.h"
#include "libloom/log.h"
#include "../worker.h"

using namespace loom;
using namespace loom::base;

Array::Array(size_t length, std::unique_ptr<DataPtr[]> items)
   : length(length), items(std::move(items))
{

}

Array::~Array()
{
    logger->debug("Disposing array");
}

size_t Array::get_length() const {
    return length;
}

size_t Array::get_size() const
{
    size_t size = 0;
    for (size_t i = 0; i < length; i++) {
      size += items[i]->get_size();
   }
   return size;
}

std::string Array::get_info() const
{
    return "Array";
}

DataPtr Array::get_slice(size_t from, size_t to) const
{
    if (from > length) {
        from = length;
    }

    if (to > length) {
        to = length;
    }

    size_t size;
    if (from >= to) {
        size = 0;
    } else {
        size = to - from;
    }

    auto items = std::make_unique<DataPtr[]>(size);

    size_t j = 0;
    for (size_t i = from; i < to; i++, j++) {
        items[j] = this->items[i];
    }
    return std::make_shared<Array>(size, std::move(items));
}

DataPtr &Array::get_ref_at_index(size_t index)
{
    assert(index < length);
    return items[index];
}

std::string Array::get_type_name() const
{
   return "loom/array";
}

DataPtr Array::get_at_index(size_t index) const
{
   assert(index < length);
   return items[index];
}

size_t Array::serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const
{
    auto types = std::make_unique<base::MemItemWithSz>(sizeof(loom::base::Id) * length);
    loom::base::Id *ts = reinterpret_cast<loom::base::Id*>(types->get_ptr());
    for (size_t i = 0; i < length; i++) {
        ts[i] = items[i]->get_type_id(worker);
    }
    buffer.add(std::move(types));
    size_t messages = 1;
    for (size_t i = 0; i < length; i++) {
        messages += items[i]->serialize(worker, buffer, items[i]);
    }
    return messages;
}

ArrayUnpacker::ArrayUnpacker(Worker &worker) : index(0), worker(worker)
{

}

ArrayUnpacker::~ArrayUnpacker()
{

}

DataUnpacker::Result ArrayUnpacker::on_message(const char *data, size_t size)
{
   if (!unpacker) {
       assert(size % sizeof(Id) == 0);
       size_t sz = size / sizeof(Id);
       types.resize(sz);
       memcpy(&types[0], data, size);

       if (types.size() == 0) {
           return FINISHED;
       }

       items = std::make_unique<DataPtr[]>(sz);
       unpacker = worker.get_unpacker(types[0]);
       return unpacker->get_initial_mode();
   } else {
       Result r = unpacker->on_message(data, size);
       if (r != FINISHED) {
           return r;
       }
       return unpack_next();
   }
}

DataUnpacker::Result ArrayUnpacker::on_stream_data(const char *data, size_t size, size_t remaining)
{
   assert(unpacker);
   Result r = unpacker->on_stream_data(data, size, remaining);
   if (r != FINISHED) {
       return r;
   }
   return unpack_next();
}

DataPtr ArrayUnpacker::finish()
{
   return std::make_shared<Array>(types.size(), std::move(items));
}

DataUnpacker::Result ArrayUnpacker::unpack_next()
{
   items[index++] = unpacker->finish();
   if (index == types.size()) {
      return FINISHED;
   } else {
      unpacker = worker.get_unpacker(types[index]);
      return unpacker->get_initial_mode();
   }
}
