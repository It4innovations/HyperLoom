#ifndef LOOM_CONNECTION_H
#define LOOM_CONNECTION_H

#include "loomcomm.pb.h"
#include "utils.h"

#include <uv.h>

#include <string>
#include <functional>
#include <memory>


namespace loom {

class Connection;

class ConnectionCallback
{
    friend class Connection;
public:
    virtual ~ConnectionCallback();

protected:
    virtual void on_connection() { assert(0); }
    virtual void on_error(int error_code);
    virtual void on_close() = 0;
    virtual void on_data_chunk(const char *buffer, size_t size) { assert(0); }
    virtual void on_data_finish() { assert(0); }
    virtual void on_message(const char *buffer, size_t size) = 0;
};


class Connection
{    
public:

    enum State {
        ConnectionNew,
        ConnectionConnecting,
        ConnectionOpen,
        ConnectionClosing,
        ConnectionClosed,
    };

    Connection(ConnectionCallback *callback, uv_loop_t *loop);
    ~Connection();

    State get_state() const {
        return state;
    }

    void set_callback(ConnectionCallback *callback) {
        this->callback = callback;
    }

    void send(uv_write_t *request, uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    {
        UV_CHECK(uv_write(request, (uv_stream_t*) &socket, bufs, nbufs, cb));
    }

    std::string get_peername();
    
    void connect(std::string host, int port);
    void send_message(::google::protobuf::MessageLite &message);

    void close();
    void close_and_discard_remaining_data();
    void accept(uv_tcp_t *listen_socket);
    void start_read();

    void set_raw_read(size_t size);

protected:

    ConnectionCallback* callback;
    State state;
    uv_tcp_t socket;
    size_t data_size;
    char *data_ptr;
    std::unique_ptr<char[]> received_buffer;
    size_t remaining_raw_data;

private:
    static void _on_connection(uv_connect_t *connect, int status);
    static void _on_fbb_write(uv_write_t *write_req, int status);
    static void _on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void _on_close(uv_handle_t *handle);
    static void _buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
};

class SimpleConnectionCallback : public ConnectionCallback
{
public:
    SimpleConnectionCallback(uv_loop_t *loop) : connection(this, loop) {}
    virtual ~SimpleConnectionCallback();

    void connect(const std::string &address, int port) {
        connection.connect(address, port);
    }

    void send_message(::google::protobuf::MessageLite &message) {
        connection.send_message(message);
    }

    void close()
    {
        connection.close();
    }


protected:
    Connection connection;
};

}

#endif // LOOM_CLIENT_CONNECTION_H
