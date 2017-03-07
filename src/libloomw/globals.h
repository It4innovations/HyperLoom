#ifndef LIBLOOMW_GLOBALS_H
#define LIBLOOMW_GLOBALS_H

#include <string>
#include <atomic>

namespace loom {
    class Worker;

class Globals {
    friend class loom::Worker;

public:

    Globals();

    const std::string& get_work_dir() const {
        return work_dir;
    }

    std::string create_data_filename();

private:
    void init(const std::string &work_dir);
    std::string work_dir;
    std::atomic<size_t> file_id_counter;
};

}


#endif // LIBLOOMW_GLOBALS_H
