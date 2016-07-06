#ifndef LIBLOOM_RAWDATA_H
#define LIBLOOM_RAWDATA_H

#include "../data.h"

namespace loom {

class RawData : public Data {
public:
    static const int TYPE_ID = 300;

    RawData();
    ~RawData();

    int get_type_id() {
        return TYPE_ID;
    }

    size_t get_size() {
        return size;
    }

    char *get_raw_data(Worker &worker)
    {
        if (data == nullptr) {
            open(worker);
        }
        return data;
    }

    std::string get_info();    
    void init_message(Worker &worker, loomcomm::Data &msg) const;
    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

    char* init_memonly(size_t size);
    char* init_empty_file(Worker &worker, size_t size);
    void assign_file_id();
    void init_from_file(Worker &worker);

    std::string get_filename(Worker &worker) const;

private:

    void open(Worker &worker);
    void map(int fd, bool write);

    char *data;
    size_t size;
    size_t file_id;

    static size_t file_id_counter;

};


class RawDataUnpacker : public DataUnpacker
{
public:
    ~RawDataUnpacker();
    RawData& get_data() {
        return *(static_cast<RawData*>(data.get()));
    }

    bool init(Worker &worker, Connection &connection, const loomcomm::Data &msg);
    void on_data_chunk(const char *data, size_t size);
    bool on_data_finish(Connection &connection);
protected:
    char *pointer = nullptr;
};

}

#endif // LIBLOOM_RAWDATA_H
