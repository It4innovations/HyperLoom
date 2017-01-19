#ifndef LIBLOOMW_INDEX_H
#define LIBLOOMW_INDEX_H

#include "../data.h"
#include "../unpacking.h"


#include <memory>


namespace loom {

class Worker;

class Index : public Data {
public:
    Index(Worker &worker,
          const DataPtr &data,
          size_t length,
          std::unique_ptr<size_t[]> &&indices);
    ~Index();

    std::string get_type_name() const override;
    size_t get_length() const override;
    size_t get_size() const override;
    std::string get_info() const override;
    DataPtr get_at_index(size_t index) const override;
    DataPtr get_slice(size_t from, size_t to) const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const override;

protected:


    Worker &worker;
    DataPtr data;
    size_t length;
    std::unique_ptr<size_t[]> indices;
};


class IndexUnpacker : public DataUnpacker
{
public:
   explicit IndexUnpacker(Worker &worker);
   Result on_message(const char *data, size_t size) override;
   Result on_stream_data(const char *data, size_t size, size_t remaining) override;
   DataPtr finish() override;
private:
    size_t length;
    std::unique_ptr<size_t[]> indices;
    std::unique_ptr<DataUnpacker> unpacker;
    Worker &worker;
};
}

#endif // LIBLOOMW_INDEX_H
