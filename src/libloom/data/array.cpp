
#include "array.h"
#include "../compat.h"
#include "../worker.h"
#include "../log.h"

using namespace loom;

Array::Array(size_t length, std::unique_ptr<std::shared_ptr<Data>[]> items)
   : length(length), items(std::move(items))
{

}

Array::~Array()
{
    llog->debug("Disposing array");
}

size_t Array::get_size()
{
   size_t size = 0;
   for (size_t i = 0; i < length; i++) {
      size += items[i]->get_size();
   }
   return size;
}

std::string Array::get_info()
{
    return "Array";
}

std::shared_ptr<Data> Array::get_slice(size_t from, size_t to)
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

    auto items = std::make_unique<std::shared_ptr<Data>[]>(size);

    size_t j = 0;
    for (size_t i = from; i < to; i++, j++) {
        items[j] = this->items[i];
    }
    return std::make_shared<Array>(size, std::move(items));
}

std::shared_ptr<Data> &Array::get_ref_at_index(size_t index)
{
    assert(index < length);
    return items[index];
}

std::shared_ptr<Data> Array::get_at_index(size_t index)
{
   assert(index < length);
   return items[index];
}

void Array::serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr)
{
   for (size_t i = 0; i < length; i++) {
      items[i]->serialize(worker, buffer, items[i]);
   }
}

ArrayUnpacker::~ArrayUnpacker()
{

}

bool ArrayUnpacker::init(Worker &worker, Connection &connection, const loomcomm::Data &msg)
{
   length = msg.length();
   index = 0;
   items = std::make_unique<std::shared_ptr<Data>[]>(length);
   if (length == 0) {
      finish();
      return true;
   } else {
      this->worker = &worker;
      return false;
   }
}

bool ArrayUnpacker::on_message(Connection &connection, const char *data, size_t size)
{
   if (unpacker) {
      bool r = unpacker->on_message(connection, data, size);
      if (r) {
         return finish_data();
      }
      return false;
   }
   loomcomm::Data msg;
   msg.ParseFromArray(data, size);
   unpacker = worker->unpack(msg.type_id());
   if (unpacker->init(*worker, connection, msg)) {
       return finish_data();
   } else {
      return false;
   }
}

void ArrayUnpacker::on_data_chunk(const char *data, size_t size)
{
   unpacker->on_data_chunk(data, size);;
}

bool ArrayUnpacker::on_data_finish(Connection &connection)
{
   bool r = unpacker->on_data_finish(connection);
   if (r) {
      return finish_data();
   }
   return false;
}

void ArrayUnpacker::finish()
{
   data = std::make_shared<Array>(length, std::move(items));
}

bool ArrayUnpacker::finish_data()
{
   items[index] = unpacker->get_data();
   unpacker.reset();
   index++;
   if (index == length) {
      finish();
      return true;
   } else {
      return false;
   }
}
