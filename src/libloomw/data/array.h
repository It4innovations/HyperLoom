#ifndef LIBLOOMW_DATA_ARRAY_H
#define LIBLOOMW_DATA_ARRAY_H

#include "../data.h"
#include "../unpacking.h"

namespace loom {

class Array : public Data {
public:    
    Array(size_t length, std::unique_ptr<std::shared_ptr<Data>[]> items);
    Array(const DataVector &items);
    ~Array();

    size_t get_length() const override;
    size_t get_size() const override;
    std::string get_info() const override;
    std::shared_ptr<Data> get_at_index(size_t index) const override;
    std::shared_ptr<Data> get_slice(size_t from, size_t to) const override;
    std::string get_type_name() const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr) const override;

    std::shared_ptr<Data>& get_ref_at_index(size_t index);

protected:

    size_t length;
    std::unique_ptr<std::shared_ptr<Data>[]> items;
};


class ArrayUnpacker : public DataUnpacker
{
public:
   ArrayUnpacker(Worker &worker);
   ~ArrayUnpacker();

   Result on_message(const char *data, size_t size) override;
   Result on_stream_data(const char *data, size_t size, size_t remaining) override;
   std::shared_ptr<Data> finish() override;

   Result unpack_next();

private:
   std::unique_ptr<DataUnpacker> unpacker;
   std::vector<loom::base::Id> types;
   std::unique_ptr<std::shared_ptr<Data>[]> items;
   size_t index;
   Worker &worker;
};


}
#endif // LIBLOOMW_DATA_ARRAY_H
