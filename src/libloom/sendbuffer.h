#ifndef LIBLOOM_SENDBUFFER_H
#define LIBLOOM_SENDBUFFER_H

#include <uv.h>
#include <memory>
#include <vector>

namespace google {
    namespace protobuf {
        class MessageLite;
    }
}

namespace loom {

class Data;

class SendBuffer {

public:
    SendBuffer() {
        request.data = this;
    }
    virtual ~SendBuffer();

    virtual void on_finish(int status);

    uv_buf_t *get_uv_bufs() {
        return &bufs[0];
    }

    size_t get_uv_bufs_count() {
        return bufs.size();
    }

    void add(std::unique_ptr<char[]> data, size_t size) {
        bufs.emplace_back(uv_buf_t {data.get(), size});
        raw_memory.push_back(std::move(data));
    }

    void add(std::shared_ptr<Data> &data, char *data_ptr, size_t size) {
        bufs.emplace_back(uv_buf_t {data_ptr, size});
        data_vector.push_back(data);
    }

    void add(::google::protobuf::MessageLite &message);

    uv_write_t request;

protected:
    std::vector<uv_buf_t> bufs;
    std::vector<std::unique_ptr<char[]>> raw_memory;
    std::vector<std::shared_ptr<Data>> data_vector;

};


}

#endif // LIBLOOM_SENDBUFFER_H
