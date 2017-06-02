#ifndef LIBLOOMW_EXTERNFILE_H
#define LIBLOOMW_EXTERNFILE_H


#include "../data.h"


namespace loom {

class ExternFile : public Data {
public:
    std::string get_type_name() const override;
    ExternFile(const std::string &filename);
    ~ExternFile();

    size_t get_size() const override;
    const char *get_raw_data() const override;
    bool has_raw_data() const override;
    std::string get_info() const override;
    std::string map_as_file(Globals &globals) const override;
    size_t serialize(Worker &worker, loom::base::SendBuffer &buffer, const DataPtr &data_ptr) const override;

protected:

    void open();
    void map(int fd, bool write);

    char *data;
    size_t size;
    std::string filename;
};

}

#endif // LIBLOOMW_EXTERNFILE_H
