#ifndef LIBLOOM_RAWDATA_H
#define LIBLOOM_RAWDATA_H

#include "../data.h"

namespace loom {

class RawData : public Data {

public:
    RawData();
    ~RawData();

    std::string get_type_name() const;

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

    char* init_empty_file(Worker &worker, size_t size);
    void assign_filename(Worker &worker);
    void init_from_file(Worker &worker);

    std::string get_filename() const;

protected:

    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);
    void open(Worker &worker);
    void map(int fd, bool write);

    char *data;
    size_t size;
    std::string filename;

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

    static const char* get_type_name() {
        return "loom/data";
    }

protected:
    char *pointer = nullptr;
};

}

#endif // LIBLOOM_RAWDATA_H
