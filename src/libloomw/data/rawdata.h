#ifndef LIBLOOMW_RAWDATA_H
#define LIBLOOMW_RAWDATA_H

#include "../data.h"
#include "../unpacking.h"

#include <mutex>

namespace loom {

class Globals;

class RawData : public Data {

public:
    RawData();
    ~RawData();

    std::string get_type_name() const override;

    size_t get_size() const override;

    const char *get_raw_data() const override;

    bool has_raw_data() const override;
    std::string get_info() const override;
    std::string get_filename() const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const override;

    char* init_empty(loom::Globals &globals, size_t size);
    void init_from_string(loom::Globals &globals, const std::string &str);
    void init_from_mem(loom::Globals &globals, const void *ptr, size_t size);
    void init_from_file(loom::Globals &globals);
    void assign_filename(loom::Globals &globals);


protected:

    void open() const;
    void map(int fd, bool write) const;

    mutable char *data;
    mutable std::mutex mutex;
    size_t size;
    std::string filename;

    static std::atomic<size_t> file_id_counter;
};


class RawDataUnpacker : public DataUnpacker
{
public:
   RawDataUnpacker(Worker &worker);
   ~RawDataUnpacker();

   Result get_initial_mode() override;
   Result on_stream_data(const char *data, size_t size, size_t remaining) override;
   DataPtr finish() override;
private:
   DataPtr result;
   char* ptr;
   Globals& globals;
};

}

#endif // LIBLOOMW_RAWDATA_H
