
#include "socket.h"
#include "log.h"
#include "libloom/compat.h"

#include <assert.h>

using namespace loom::base;

Socket::Socket(uv_loop_t *loop)
   : state(State::New), stream_mode(false), stream_remaining(0)
{
    UV_CHECK(uv_tcp_init(loop, &uv_socket));
    UV_CHECK(uv_tcp_nodelay(&uv_socket, 1));
    uv_socket.data = this;

    on_error = [](int status) {
        assert(status);
        UV_CHECK(status);
    };
}

Socket::~Socket()
{
    assert(state == State::Closed);
}

void Socket::close()
{
    if (state != State::Closed && state != State::Closing) {
        state = State::Closing;
        uv_close((uv_handle_t*) &uv_socket, [](uv_handle_t *handle) {
            auto *socket = static_cast<Socket *>(handle->data);
            socket->state = State::Closed;
            socket->on_close();
        });
    }
}

void Socket::close_and_discard_remaining_data()
{
    buffer.clear();
    uv_read_stop((uv_stream_t*) &uv_socket);
    close();
}


void Socket::accept(uv_tcp_t *listen_socket)
{
    assert(state == State::New);
    UV_CHECK(uv_accept((uv_stream_t*) listen_socket, (uv_stream_t*) &uv_socket));
    uv_read_start((uv_stream_t *)&uv_socket, _buf_alloc, _on_read);
    state = State::Open;
}

void Socket::connect(const std::string &host, int port)
{
    assert(state == State::New);
    state = State::Connecting;

    struct sockaddr_in dest;
    UV_CHECK(uv_ip4_addr(host.c_str(), port, &dest));
    uv_connect_t *connect = new uv_connect_t;
    connect->data = this;
    UV_CHECK(uv_tcp_connect(connect, &uv_socket, (const struct sockaddr *)&dest,
                            [](uv_connect_t *connect, int status){
                 Socket *socket = static_cast<Socket *>(connect->data);
                 delete connect;
                 if (status) {
                     socket->on_error(status);
                 } else {
                     socket->state = State::Open;
                     uv_read_start((uv_stream_t *)&socket->uv_socket, _buf_alloc, _on_read);
                     socket->on_connect();
                 }                
             }));
}

std::string Socket::get_peername() const
{
    sockaddr_in addr;
    int len = sizeof(addr);
    UV_CHECK(uv_tcp_getpeername(&uv_socket, (struct sockaddr*) &addr, &len));
    char tmp[60];
    UV_CHECK(uv_ip4_name(&addr, tmp, 60));
    return tmp;
}

void Socket::send(std::unique_ptr<SendBuffer> buffer)
{    
    auto bufs = buffer->get_bufs();
    SendBuffer *b = buffer.release();
    UV_CHECK(uv_write(b->get_request(), (uv_stream_t *) &uv_socket, &bufs[0], bufs.size(),
                      [](uv_write_t *write_req, int status){
                 UV_CHECK(status);
                 SendBuffer *buffer = static_cast<SendBuffer *>(write_req->data);
                 delete buffer;
             }));
}

void Socket::_buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    size_t size;
    Socket *socket = static_cast<Socket*>(handle->data);
    if (socket->stream_mode) {
        size = 8 << 20; // 8 MB
    } else {
        size = suggested_size;
    }
    buf->base = new char[size];
    buf->len = size;
}


void Socket::_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    Socket *socket = static_cast<Socket *>(stream->data);
    if (nread == UV_EOF) {
        if (buf->base) {
            delete[] buf->base;
        }
        socket->close();
        return;
    }
    if (nread < 0) {
        socket->on_error(nread);
        return;
    }

    if (nread == 0) {
        return;
    }

    auto &buffer = socket->buffer;
    char* data = buf->base;
    size_t size_read = nread;

    if (socket->stream_remaining) {
         if (socket->stream_remaining >= size_read) {
            socket->stream_remaining -= size_read;
            socket->on_stream_data(data, size_read, socket->stream_remaining);
            delete [] data;
            return;
         }
         socket->on_stream_data(data, socket->stream_remaining, 0);
         assert(buffer.size() == 0);
         char *start_data = data + socket->stream_remaining;
         socket->stream_remaining = 0;
         buffer.insert(buffer.begin(), start_data, data + size_read);
    } else {
       buffer.insert(buffer.end(), data, data + size_read);
    }
    delete [] data;

    for (;;) {
        size_t size = buffer.size();
        if (size < sizeof(uint64_t)) {
            return;
        }
        uint64_t sz = *(reinterpret_cast<uint64_t *>(&buffer[0]));
        uint64_t sz2 = sz + sizeof(sz);
        if (size < sz2) {
            if (socket->stream_mode) {
               uint64_t data_size = size - sizeof(sz);
               socket->stream_remaining = sz - data_size;
               socket->on_stream_data(&buffer[sizeof(sz)], data_size, socket->stream_remaining);
               buffer.clear();
            }
            return;
        }

        if (!socket->stream_mode) {
            socket->on_message(&buffer[sizeof(sz)], sz);
        } else {
            socket->on_stream_data(&buffer[sizeof(sz)], sz, 0);
        }
        if (buffer.size()) { // Buffer was not discarded by on_message/on_stream_data
            buffer.erase(buffer.begin(), buffer.begin() + sz2);
        }
    }
}
