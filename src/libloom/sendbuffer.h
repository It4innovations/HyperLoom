#ifndef LIBLOOM_SENDBUFFER_H
#define LIBLOOM_SENDBUFFER_H

#include "types.h"

#include <uv.h>
#include <memory>
#include <vector>

namespace loom {
namespace base {


class SendBufferItem {
public:
   virtual ~SendBufferItem();
   virtual uv_buf_t get_buf() = 0;
};


class MemItem : public SendBufferItem {

public:
   explicit MemItem(size_t size);
   ~MemItem();
   uv_buf_t get_buf();
   char* get_ptr() {
      return mem.get();
   }

private:
   std::unique_ptr<char[]> mem;
   size_t size;
};


class MemItemWithSz : public SendBufferItem {

public:
   explicit MemItemWithSz(size_t size);
   ~MemItemWithSz();
   uv_buf_t get_buf();
   char* get_ptr() {
      return mem.get() + sizeof(uint64_t);
   }

private:
   std::unique_ptr<char[]> mem;
   size_t size;
};

template<typename T> class PODItem : public SendBufferItem {

public:
   explicit PODItem(const T& value) : value(value) {}

   uv_buf_t get_buf() {
      uv_buf_t buf;
      buf.base = reinterpret_cast<char*>(&value);
      buf.len = sizeof(T);
      return buf;
   }

protected:
   T value;
};

using SizeBufferItem = PODItem<uint64_t>;
using IdBufferItem = PODItem<Id>;


class SendBuffer {

public:

    SendBuffer();

    uv_write_t* get_request() {
        return &request;
    }

    void add(std::unique_ptr<SendBufferItem> &&item) {
        items.push_back(std::move(item));
    }

    void insert(size_t index, std::unique_ptr<SendBufferItem> &&item) {
        items.insert(items.begin() + index, std::move(item));
    }

    size_t get_size() const {
       return items.size();
    }

    size_t get_data_size() const;

    std::vector<uv_buf_t> get_bufs();

protected:
    uv_write_t request;
    std::vector<std::unique_ptr<SendBufferItem>> items;
};

}}

#endif // LIBLOOM_SENDBUFFER_H
