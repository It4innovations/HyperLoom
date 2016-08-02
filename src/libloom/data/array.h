#ifndef LIBLOOM_DATA_ARRAY_H
#define LIBLOOM_DATA_ARRAY_H

#include "../data.h"

namespace loom {

class Array : public Data {
public:    
    Array(size_t length, std::unique_ptr<std::shared_ptr<Data>[]> items);
    ~Array();

    size_t get_length() {
        return length;
    }

    size_t get_size();
    std::string get_info();
    std::shared_ptr<Data> get_at_index(size_t index);
    std::shared_ptr<Data> get_slice(size_t from, size_t to);

    std::shared_ptr<Data>& get_ref_at_index(size_t index);

    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);    
    std::string get_type_name() const;

private:
    size_t length;
    std::unique_ptr<std::shared_ptr<Data>[]> items;
};


class ArrayUnpacker : public DataUnpacker
{
public:
    ~ArrayUnpacker();

    bool init(Worker &worker, Connection &connection, const loomcomm::Data &msg);
    bool on_message(Connection &connection, const char *data, size_t size);
    void on_data_chunk(const char *data, size_t size);
    bool on_data_finish(Connection &connection);

    static const char* get_type_name() {
        return "loom/array";
    }

protected:

    void finish();
    bool finish_data();


    std::unique_ptr<DataUnpacker> unpacker;
    Worker *worker;
    size_t index;
    size_t length;
    std::unique_ptr<std::shared_ptr<Data>[]> items;

};

}
#endif // LIBLOOM_DATA_ARRAY_H
