#ifndef LIBLOOM_DATA_ARRAY_H
#define LIBLOOM_DATA_ARRAY_H

#include "../data.h"
#include "../unpacking.h"

namespace loom {

class Array : public Data {
public:    
    Array(size_t length, std::unique_ptr<std::shared_ptr<Data>[]> items);
    Array(const DataVector &items);
    ~Array();

    size_t get_length() {
        return length;
    }

    size_t get_size();
    std::string get_info();
    std::shared_ptr<Data> get_at_index(size_t index);
    std::shared_ptr<Data> get_slice(size_t from, size_t to);

    std::shared_ptr<Data>& get_ref_at_index(size_t index);

    std::string get_type_name() const;
    size_t serialize(Worker &worker, loom::net::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

protected:

    size_t length;
    std::unique_ptr<std::shared_ptr<Data>[]> items;
};


class ArrayUnpacker : public DataUnpacker
{
public:
   ArrayUnpacker(Worker &worker);
   ~ArrayUnpacker();

   Result on_message(const char *data, size_t size);
   Result on_stream_data(const char *data, size_t size, size_t remaining);
   std::shared_ptr<Data> finish();

   Result unpack_next();

private:
   std::unique_ptr<DataUnpacker> unpacker;
   std::vector<Id> types;
   std::unique_ptr<std::shared_ptr<Data>[]> items;
   size_t index;
   Worker &worker;
};


}
#endif // LIBLOOM_DATA_ARRAY_H
