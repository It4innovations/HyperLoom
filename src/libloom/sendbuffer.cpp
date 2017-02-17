#include "sendbuffer.h"

#include "compat.h"

using namespace loom::base;

SendBuffer::SendBuffer()
{
    request.data = this;
}

size_t SendBuffer::get_data_size() const
{
    size_t result = 0;
    for (auto& item : items) {
        uv_buf_t buf = item->get_buf();
        result += buf.len;
    }
    return result;
}

std::vector<uv_buf_t> loom::base::SendBuffer::get_bufs()
{
   std::vector<uv_buf_t> bufs;
   bufs.reserve(items.size());
   for (auto &item : items) {
      bufs.push_back(item->get_buf());
   }
   return bufs;
}

loom::base::MemItem::MemItem(size_t size) : size(size)
{
   mem = std::make_unique<char[]>(size);
}

loom::base::MemItem::~MemItem()
{

}

uv_buf_t MemItem::get_buf()
{
   uv_buf_t buf;
   buf.base = &mem[0];
   buf.len = size;
   return buf;
}

SendBufferItem::~SendBufferItem()
{

}

MemItemWithSz::MemItemWithSz(size_t size) : size(size + sizeof(uint64_t))
{
   mem = std::make_unique<char[]>(size + sizeof(uint64_t));
   uint64_t *size_ptr = reinterpret_cast<uint64_t*>(mem.get());
   *size_ptr = size;
}

MemItemWithSz::~MemItemWithSz()
{

}

uv_buf_t MemItemWithSz::get_buf()
{
   uv_buf_t buf;
   buf.base = &mem[0];
   buf.len = size;
   return buf;
}
