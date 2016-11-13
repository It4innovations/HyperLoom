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

/** Base class for data objects */
class Data
{    
public:
    virtual ~Data();

    virtual std::string get_type_name() const = 0;

    /** Get size of data */
    virtual size_t get_size() = 0;

    /** Get debugging info string */
    virtual std::string get_info() = 0;

    /** Get length of data (when object is not indexable then returns 0) */
    virtual size_t get_length();

    /** Get subobject at given index (0 ... get_length()) */
    virtual std::shared_ptr<Data> get_at_index(size_t index);

    /** Get subobject slice at given indices (0 ... get_length()) */
    virtual std::shared_ptr<Data> get_slice(size_t from, size_t to);

    /** Serialize object into send buffer */
    void serialize(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr);

    /** Get pointer to raw data, returns nullptr when it is not possible */
    virtual const char *get_raw_data() const;

    /** Returns a filename if data obeject is mapped from file, empty string otherwise */
    virtual std::string get_filename() const;

    virtual bool has_raw_data() const;

protected:
    /** Init serialization message */
    virtual void init_message(Worker &worker, loomcomm::Data &msg) const;

    /** Serialize content */
    virtual void serialize_data(Worker &worker, SendBuffer &buffer, std::shared_ptr<Data> &data_ptr) = 0;
};

/** Base class for deserialization */
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

typedef std::vector<std::shared_ptr<Data>> DataVector;

}

#endif // LOOM_DATA_H
