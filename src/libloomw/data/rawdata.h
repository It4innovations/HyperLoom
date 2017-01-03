#ifndef LIBLOOMW_RAWDATA_H
#define LIBLOOMW_RAWDATA_H

#include "../data.h"
#include "../unpacking.h"

namespace loom {

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
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, std::shared_ptr<Data> &data_ptr) const override;

    char* init_empty(const std::string &work_dir, size_t size);
    void init_from_string(const std::string &work_dir, const std::string &str);
    void init_from_mem(const std::string &work_dir, const void *ptr, size_t size);
    void init_from_file(const std::string &work_dir);
    void assign_filename(const std::string &work_dir);


protected:

    void open() const;
    void map(int fd, bool write) const;

    mutable char *data;
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
   std::shared_ptr<Data> finish() override;
private:
   std::shared_ptr<Data> result;
   char* ptr;
   const std::string& worker_dir;
};

}

#endif // LIBLOOMW_RAWDATA_H
