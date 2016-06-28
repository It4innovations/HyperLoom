/*#ifndef LIBLOOM_DATABUILDER_H
#define LIBLOOM_DATABUILDER_H

#include "data.h"

#include <stdlib.h>
#include <memory>
#include <string.h>

namespace loom {

class Worker;
class Data;

class DataBuilder
{
public:
    DataBuilder(Worker &worker, loom::Id id, size_t size, bool in_file)
        : data(std::make_unique<Data>(id))
    {
        if (in_file) {
            pointer = data->init_empty_file(worker, size);
        } else {
            pointer = data->init_memonly(size);
        }
    }

    std::unique_ptr<Data> release_data() {
        pointer = nullptr;
        return std::move(data);
    }

    void add(const char *new_data, size_t size) {
        memcpy(pointer, new_data, size);
        pointer += size;
    }

public:
    std::unique_ptr<Data> data;
    char *pointer;

};

}

#endif // LIBLOOM_DATABUILDER_H
*/
