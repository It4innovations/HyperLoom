#ifndef LOOM_DATA_H
#define LOOM_DATA_H

#include "types.h"

#include "loomcomm.pb.h"

#include <uv.h>
#include <string>
#include <memory>

namespace loom {

class Worker;
class SendBuffer;
class Connection;

class Data
{    
public:
    virtual ~Data();

    virtual int get_type_id() = 0;

    virtual size_t get_size() = 0;
    virtual std::string get_info() = 0;

    void serialize(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);
    virtual void init_message(Worker &worker, loomcomm::Data &msg) const;
    virtual void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr) = 0;

    virtual char *get_raw_data(Worker &worker);
    virtual std::string get_filename(Worker &worker) const;

};

class DataUnpacker
{
public:
    virtual ~DataUnpacker();
    virtual bool init(Worker &worker, Connection &connection, const loomcomm::Data &msg) = 0;
    virtual bool on_message(Connection &connection, const char *data, size_t size);
    virtual void on_data_chunk(const char *data, size_t size);
    virtual bool on_data_finish(Connection &connection);

    std::shared_ptr<Data>& get_data() {
        return data;
    }

protected:
    std::shared_ptr<Data> data;
};

}

#endif // LOOM_DATA_H
