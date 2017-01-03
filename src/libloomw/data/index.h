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
          DataPtr &data,
          size_t length,
          std::unique_ptr<size_t[]> indices);

    ~Index();

    std::string get_type_name() const override;
    size_t get_length() const override;
    size_t get_size() const override;
    std::string get_info() const override;
    DataPtr get_at_index(size_t index) const override;
    DataPtr get_slice(size_t from, size_t to) const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, DataPtr &data_ptr) const override;

protected:


    Worker &worker;
    DataPtr data;
    size_t length;
    std::unique_ptr<size_t[]> indices;
};


class IndexUnpacker : public DataUnpacker
{
public:
   IndexUnpacker();
   ~IndexUnpacker();

   Result on_message(const char *data, size_t size) override;
   DataPtr finish() override;
};
}

#endif // LIBLOOMW_INDEX_H
