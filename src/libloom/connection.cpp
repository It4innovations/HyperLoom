#include "connection.h"
#include "utils.h"

#include <string.h>

using namespace loom;

Connection::Connection(ConnectionCallback *callback, uv_loop_t *loop)
    : callback(callback),
      state(ConnectionNew),
      data_size(0),
      data_ptr(nullptr),
      remaining_raw_data(0)
{
    uv_tcp_init(loop, &socket);
    uv_tcp_nodelay(&socket, 1);
    socket.data = this;
}

Connection::~Connection()
{
    assert(state == ConnectionClosed);
}

std::string Connection::get_peername()
{
    sockaddr_in addr;
    int len = sizeof(addr);
    UV_CHECK(uv_tcp_getpeername(&socket, (struct sockaddr*) &addr, &len));
    char tmp[60];
    UV_CHECK(uv_ip4_name(&addr, tmp, 60));
    return tmp;
}

void Connection::close()
{
    if (state != ConnectionClosed && state != ConnectionClosing) {
        state = ConnectionClosing;
        uv_close((uv_handle_t*) &socket, _on_close);
    }
}

void Connection::close_and_discard_remaining_data()
{
    received_buffer.reset();
    data_size = 0;
    uv_read_stop((uv_stream_t*) &socket);
    close();
}

void Connection::accept(uv_tcp_t *listen_socket)
{
    UV_CHECK(uv_accept((uv_stream_t*) listen_socket, (uv_stream_t*) &socket));
    uv_read_start((uv_stream_t *)&socket, _buf_alloc, _on_read);
    state = ConnectionOpen;
}

void Connection::start_read()
{
    uv_read_start((uv_stream_t *)&socket, _buf_alloc, _on_read);
}

void Connection::set_raw_read(size_t size)
{
    assert(remaining_raw_data == 0);

    if (size == 0) {
        callback->on_data_finish();
        return;
    }

    remaining_raw_data = size;
    if (data_size == 0) {
        return;
    }

    if (data_size <= size) {
        callback->on_data_chunk(data_ptr, data_size);
        remaining_raw_data -= data_size;
        data_size = 0;
        received_buffer.reset();
        if (remaining_raw_data == 0) {
            callback->on_data_finish();
        }
        return;
    }

    callback->on_data_chunk(data_ptr, size);
    remaining_raw_data = 0;
    data_size -= size;
    data_ptr += size;
    callback->on_data_finish();
}

void Connection::_on_connection(uv_connect_t *connect, int status)
{
    Connection *connection = static_cast<Connection *>(connect->data);
    if (status) {
        connection->callback->on_error(status);
    } else {
        connection->state = ConnectionOpen;
        connection->callback->on_connection();
    }
    delete connect;
}

void Connection::connect(std::string host, int port)
{
    assert(state == ConnectionNew);
    state = ConnectionConnecting;
    struct sockaddr_in dest;
    UV_CHECK(uv_ip4_addr(host.c_str(), port, &dest));
    uv_connect_t *connect = new uv_connect_t;
    connect->data = this;
    UV_CHECK(uv_tcp_connect(connect, &socket, (const struct sockaddr *)&dest, _on_connection));
}

void Connection::_on_write(uv_write_t *write_req, int status)
{
    UV_CHECK(status);
    SendBuffer *buffer = static_cast<SendBuffer *>(write_req->data);
    buffer->on_finish(status);
}

void Connection::send_message(google::protobuf::MessageLite &message)
{
    SendBuffer *buffer = new SendBuffer();
    buffer->add(message);
    send_buffer(buffer);
}

void Connection::send_buffer(SendBuffer *buffer)
{
    uv_buf_t *bufs = buffer->get_uv_bufs();
    size_t count = buffer->get_uv_bufs_count();
    UV_CHECK(uv_write(&buffer->request, (uv_stream_t *) &socket, bufs, count, _on_write));
}

void Connection::_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    Connection *connection = static_cast<Connection *>(stream->data);
    if (nread == UV_EOF) {
        if (buf->base) {
            delete[] buf->base;
        }
        connection->close();
        return;
    }
    if (nread < 0) {
        connection->callback->on_error(nread);
        return;
    }

    if (nread == 0) {
        return;
    }

    /* This needs some better buffer handling,
     * but for now, we just stick with super simple solution */

    auto &data_size = connection->data_size;
    auto &data_ptr = connection->data_ptr;
    auto &received_buffer = connection->received_buffer;
    auto &remaining_raw_data = connection->remaining_raw_data;
    auto size = nread;
    auto data = buf->base;

    if (data_size) {
        char *new_data = new char[size + data_size];
        memcpy(new_data, data_ptr, data_size);
        memcpy(new_data + data_size, data, size);
        received_buffer.reset(new_data);
        data_ptr = new_data;
        delete[] data;
        data_size += size;
    } else {
        data_size = size;
        received_buffer.reset(data);
        data_ptr = data;
    }

    for (;;) {
        if (remaining_raw_data) {
            if (data_size == 0) {
                return;
            }
            if (data_size <= remaining_raw_data) {
                connection->callback->on_data_chunk(data_ptr, data_size);
                remaining_raw_data -= data_size;
                data_size = 0;
                received_buffer.reset();
                if (remaining_raw_data == 0) {
                    connection->callback->on_data_finish();
                }
                return;
            }
            connection->callback->on_data_chunk(data_ptr, remaining_raw_data);
            data_ptr += remaining_raw_data;
            data_size -= remaining_raw_data;
            remaining_raw_data = 0;
            connection->callback->on_data_finish();
            continue;
        }

        if (data_size < (ssize_t) sizeof(uint32_t)) {
            return;
        }

        uint32_t sz = *(reinterpret_cast<uint32_t *>(data_ptr));
        uint32_t sz2 = sz + sizeof(uint32_t);
        if (data_size < sz2) {
            return;
        }

        char *msg_data = data_ptr + sizeof(uint32_t);
        data_ptr += sz2;
        data_size -= sz2;
        connection->callback->on_message(msg_data, sz);
        if (data_size == 0) {
            received_buffer.reset();
            return;
        }
    }
}

void Connection::_buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

void Connection::_on_close(uv_handle_t *handle)
{
    Connection *connection = static_cast<Connection *>(handle->data);
    connection->state = ConnectionClosed;
    connection->callback->on_close();
}

ConnectionCallback::~ConnectionCallback()
{

}

void ConnectionCallback::on_error(int error_code)
{
    assert(error_code != 0);
    UV_CHECK(error_code);
}

SimpleConnectionCallback::~SimpleConnectionCallback()
{

}
