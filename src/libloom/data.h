#ifndef LOOM_DATA_H
#define LOOM_DATA_H

#include "types.h"

#include <uv.h>

#include <stdlib.h> 
#include <vector>
#include <string>

namespace loom {

class Worker;

class Data
{    
public:

    Data(Id id);
    ~Data();

    char* init_memonly(size_t size);
    char* init_empty_file(Worker &worker, size_t size);
    void init_from_file(Worker &worker);
    
    int get_id() const {
        return id;
    }

    size_t get_size() const {
        return size;
    }

    char* get_data(Worker &worker) {
        if (data == NULL) {
            open(worker);
        }
        return data;
    }

    uv_buf_t get_uv_buf(Worker &worker) {
        uv_buf_t buf;
        buf.base = get_data(worker);
        buf.len = size;
        return buf;
    }

    std::string get_filename(Worker &worker) const;
    int get_fd(Worker &worker) const;
    void make_symlink(Worker &worker, const std::string &path) const;

    /*void add(const char *new_data, size_t size) {
        data.insert(data.end(), new_data, new_data + size);
    }

    void add(const Data &other) {
        data.insert(data.end(), other.data.begin(), other.data.end());
    }

    void add(const std::string& new_data) {
        add(new_data.data(), new_data.size());
    }



    std::string get_data_as_string() const {
        return std::string(data.begin(), data.end());
    }*/

private:

    void open(Worker &worker);
    void map(int fd, bool write);

    Id id;    
    char *data;
    size_t size;
    bool in_file;
};

}

#endif // LOOM_DATA_H
