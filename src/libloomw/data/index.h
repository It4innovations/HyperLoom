#ifndef LIBLOOM_INDEX_H
#define LIBLOOM_INDEX_H

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

    std::string get_type_name() const;
    size_t get_length();
    size_t get_size();
    std::string get_info();
    std::shared_ptr<Data> get_at_index(size_t index);
    std::shared_ptr<Data> get_slice(size_t from, size_t to);
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

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

   Result on_message(const char *data, size_t size);
   std::shared_ptr<Data> finish();
};
}

#endif // LIBLOOM_INDEX_H
