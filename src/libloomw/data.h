#ifndef LIBLOOMW_DATA_H
#define LIBLOOMW_DATA_H



#include "libloom/loomcomm.pb.h"
#include "libloom/types.h"
#include "libloom/sendbuffer.h"

#include <uv.h>
#include <string>
#include <memory>

namespace loom {

class Worker;

class Connection;
class Data;

using DataPtr = std::shared_ptr<const Data>;
using DataVector = std::vector<DataPtr>;

/** Base class for data objects */
class Data
{    
public:
    virtual ~Data();

    virtual std::string get_type_name() const = 0;

    /** Get size of data */
    virtual size_t get_size() const = 0;

    /** Get length of data (when object is not indexable then returns 0) */
    virtual size_t get_length() const;

    /** Get debugging info string */
    virtual std::string get_info() const = 0;

    /** Get subobject at given index (0 ... get_length()) */
    virtual DataPtr get_at_index(size_t index) const;

    /** Get subobject slice at given indices (0 ... get_length()) */
    virtual DataPtr get_slice(size_t from, size_t to) const;

    /** Serialize object into send buffer */
    virtual size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, DataPtr &data_ptr) const = 0;

    /** Get pointer to raw data, returns nullptr when it is not possible */
    virtual const char *get_raw_data() const;

    /** Returns a filename if data obeject is mapped from file, empty string otherwise */
    virtual std::string get_filename() const;

    virtual bool has_raw_data() const;

    loom::base::Id get_type_id(Worker &worker) const;

protected:
};


class DataBufferItem : public loom::base::SendBufferItem {
public:
   DataBufferItem(DataPtr &data, const char *mem, size_t size);
   uv_buf_t get_buf();

protected:
   const char *mem;
   size_t size;
   DataPtr data;
};

}

#endif // LIBLOOMW_DATA_H