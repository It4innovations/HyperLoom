
#include "array.h"
#include "../compat.h"
#include "../worker.h"

loom::Array::Array(size_t length, std::unique_ptr<std::shared_ptr<Data>[]> items)
   : length(length), items(std::move(items))
{

}

loom::Array::~Array()
{

}

size_t loom::Array::get_size()
{
   size_t size = 0;
   for (size_t i = 0; i < length; i++) {
      size += items[i]->get_size();
   }
   return size;
}

std::string loom::Array::get_info()
{
   return "Array";
}

std::shared_ptr<loom::Data>& loom::Array::get_at_index(size_t index)
{
   assert(index < length);
   return items[index];
}

void loom::Array::serialize_data(loom::Worker &worker, loom::SendBuffer &buffer, std::shared_ptr<loom::Data> &data_ptr)
{
   for (size_t i = 0; i < length; i++) {
      items[i]->serialize(worker, buffer, items[i]);
   }
}

void loom::Array::init_message(loom::Worker &worker, loomcomm::Data &msg) const
{
   msg.set_arg0_u64(length);
}

loom::ArrayUnpacker::~ArrayUnpacker()
{

}

bool loom::ArrayUnpacker::init(loom::Worker &worker, loom::Connection &connection, const loomcomm::Data &msg)
{
   assert(msg.has_arg0_u64());
   index = 0;
   length = msg.arg0_u64();
   items = std::make_unique<std::shared_ptr<Data>[]>(length);
   if (length == 0) {
      finish();
      return true;
   } else {
      this->worker = &worker;
      return false;
   }
}

bool loom::ArrayUnpacker::on_message(loom::Connection &connection, const char *data, size_t size)
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

void loom::ArrayUnpacker::on_data_chunk(const char *data, size_t size)
{
   unpacker->on_data_chunk(data, size);;
}

bool loom::ArrayUnpacker::on_data_finish(loom::Connection &connection)
{
   bool r = unpacker->on_data_finish(connection);
   if (r) {
      return finish_data();
   }
   return false;
}

void loom::ArrayUnpacker::finish()
{
   data = std::make_shared<Array>(length, std::move(items));
}

bool loom::ArrayUnpacker::finish_data()
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
