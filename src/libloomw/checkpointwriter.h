#ifndef LIBLOOMW_CHECKPOINTWRITER_H
#define LIBLOOMW_CHECKPOINTWRITER_H

#include "libloom/types.h"
#include "data.h"

namespace loom {

class CheckPointWriter {

public:
    CheckPointWriter(Worker &worker, base::Id id, const DataPtr &data, const std::string &path);
    void start();

protected:
    uv_work_t work;
    Worker &worker;
    base::Id id;
    DataPtr data;
    std::string path;
    std::string error;

private:
    static void _work_cb(uv_work_t *req);
    static void _after_work_cb(uv_work_t *req, int status);
};

}

#endif
