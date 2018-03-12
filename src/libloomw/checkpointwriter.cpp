#include "checkpointwriter.h"
#include "worker.h"
#include "libloom/log.h"
#include <stdio.h>

loom::CheckPointWriter::CheckPointWriter(loom::Worker &worker, base::Id id, const loom::DataPtr &data, const std::string &path)
    : worker(worker), id(id), data(data), path(path)
{
    work.data = this;
}


void loom::CheckPointWriter::start()
{
    UV_CHECK(uv_queue_work(worker.get_loop(), &work, _work_cb, _after_work_cb));
}

void loom::CheckPointWriter::_work_cb(uv_work_t *req)
{
    CheckPointWriter *writer = static_cast<CheckPointWriter*>(req->data);

    std::string &path = writer->path;
    std::string tmp_path = path + ".loom.tmp";

    std::ofstream fout(tmp_path.c_str());
    if (!fout.is_open()) {
        writer->error = "Writing checkpoint '" + path + "' failed. Cannot create " + tmp_path + ":" + strerror(errno);
        return;
    }
    fout.write(writer->data->get_raw_data(), writer->data->get_size());
    fout.close();

    if (rename(tmp_path.c_str(), path.c_str())) {
        writer->error = "Writing checkpoint '" + path + "' failed. Cannot move " + tmp_path;
        unlink(tmp_path.c_str());
    }
}

void loom::CheckPointWriter::_after_work_cb(uv_work_t *req, int status)
{
    UV_CHECK(status);
    CheckPointWriter *writer = static_cast<CheckPointWriter*>(req->data);
    if (writer->error.empty()) {
        writer->worker.checkpoint_written(writer->id);
    } else {
        writer->worker.checkpoint_failed(writer->id, writer->error);
    }
    delete writer;
}
