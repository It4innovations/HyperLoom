#ifndef LIBLOOMW_GLOBALS_H
#define LIBLOOMW_GLOBALS_H

#include <string>

namespace loom {
    class Worker;

class Globals {
    friend class loom::Worker;

public:

    const std::string& get_work_dir() const {
        return work_dir;
    }

private:
    void init(const std::string &work_dir);
    std::string work_dir;
};

}


#endif // LIBLOOMW_GLOBALS_H
