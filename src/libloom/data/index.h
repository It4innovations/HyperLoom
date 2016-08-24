#ifndef LIBLOOM_INDEX_H
#define LIBLOOM_INDEX_H

#include "../data.h"


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

protected:
    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

    Worker &worker;
    std::shared_ptr<Data> data;
    size_t length;
    std::unique_ptr<size_t[]> indices;
};


class IndexUnpacker : public DataUnpacker
{
public:
    ~IndexUnpacker();

    bool init(Worker &worker, Connection &connection, const loomcomm::Data &msg);
    bool on_message(Connection &connection, const char *data, size_t size);
    void on_data_chunk(const char *data, size_t size);
    bool on_data_finish(Connection &connection);

    static const char* get_type_name() {
        return "loom/index";
    }

protected:

    void finish_data();


    std::unique_ptr<size_t[]> indices;
    char *indices_ptr;
    Worker *worker = nullptr;
    size_t length;
    std::unique_ptr<DataUnpacker> unpacker;

};

}

#endif // LIBLOOM_INDEX_H
