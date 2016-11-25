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

    const char *get_raw_data() const
    {
        if (data == nullptr) {
            open();
        }
        return data;
    }

    std::string get_info();

    char* init_empty(const std::string &work_dir, size_t size);
    void init_from_string(const std::string &work_dir, const std::string &str);
    void init_from_mem(const std::string &work_dir, const void *ptr, size_t size);
    void init_from_file(const std::string &work_dir);
    void assign_filename(const std::string &work_dir);

    std::string get_filename() const;

protected:

    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);
    void open() const;
    void map(int fd, bool write) const;

    mutable char *data;
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
