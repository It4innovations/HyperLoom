#include "sendbuffer.h"

#include "utils.h"

#include "loomcomm.pb.h"
#include "compat.h"

using namespace loom;


SendBuffer::~SendBuffer()
{

}

void SendBuffer::on_finish(int status)
{
    UV_CHECK(status);
    delete this;
}

void SendBuffer::add(google::protobuf::MessageLite &message)
{
    uint32_t size = message.ByteSize();
    auto data = std::make_unique<char[]>(size + sizeof(size));

    uint32_t *size_ptr = reinterpret_cast<uint32_t *>(data.get());
    *size_ptr = size;
    message.SerializeToArray(data.get() + sizeof(size), size);
    add(std::move(data), size + sizeof(size));
}

void SendBuffer::insert(int index, google::protobuf::MessageLite &message)
{
    uint32_t size = message.ByteSize();
    auto data = std::make_unique<char[]>(size + sizeof(size));

    uint32_t *size_ptr = reinterpret_cast<uint32_t *>(data.get());
    *size_ptr = size;
    message.SerializeToArray(data.get() + sizeof(size), size);
    insert(index, std::move(data), size + sizeof(size));
}

size_t SendBuffer::get_size() const
{
    size_t size = 0;
    for (auto &buf : bufs) {
        size += buf.len;
    }
    return size;
}
