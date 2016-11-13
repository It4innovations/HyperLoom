#ifndef LIBLOOM_EXTERNFILE_H
#define LIBLOOM_EXTERNFILE_H


#include "../data.h"


namespace loom {

class ExternFile : public Data {
public:
    std::string get_type_name() const;
    ExternFile(const std::string &filename);
    ~ExternFile();

    size_t get_size() {
        return size;
    }

    const char *get_raw_data() const
    {
        return data;
    }

    bool has_raw_data() const {
        return true;
    }

    std::string get_info();
    std::string get_filename() const;


protected:
    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

    void open();
    void map(int fd, bool write);

    char *data;
    size_t size;
    std::string filename;

    static size_t file_id_counter;

};

}

#endif // LIBLOOM_EXTERNFILE_H
