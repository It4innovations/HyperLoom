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

    char *get_raw_data(Worker &worker)
    {
        if (data == nullptr) {
            open();
        }
        return data;
    }

    std::string get_info();
    void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

    std::string get_filename() const;


private:

    void open();
    void map(int fd, bool write);

    char *data;
    size_t size;
    std::string filename;

    static size_t file_id_counter;

};

}

#endif // LIBLOOM_EXTERNFILE_H
