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
          std::shared_ptr<Data> &data,
          size_t length,
          std::unique_ptr<size_t[]> indices);

    ~Index();

    std::string get_type_name() const override;
    size_t get_length() override;
    size_t get_size() override;
    std::string get_info() override;
    std::shared_ptr<Data> get_at_index(size_t index) override;
    std::shared_ptr<Data> get_slice(size_t from, size_t to) override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr) override;

protected:


    Worker &worker;
    std::shared_ptr<Data> data;
    size_t length;
    std::unique_ptr<size_t[]> indices;
};


class IndexUnpacker : public DataUnpacker
{
public:
   IndexUnpacker();
   ~IndexUnpacker();

   Result on_message(const char *data, size_t size) override;
   std::shared_ptr<Data> finish() override;
};
}

#endif // LIBLOOMW_INDEX_H
