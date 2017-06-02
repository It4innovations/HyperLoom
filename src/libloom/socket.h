#ifndef LIBLOOM_SOCKET_H
#define LIBLOOM_SOCKET_H

#include "sendbuffer.h"

#include <uv.h>
#include <memory>
#include <vector>
#include <functional>

namespace loom {
namespace base {

class Socket {

public:

    enum State {
        New,
        Connecting,
        Open,
        Closing,        
        Closed,
    };

    explicit Socket(uv_loop_t *loop);
    ~Socket();

    void close();
    void close_and_discard_remaining_data();
    void accept(uv_tcp_t *listen_socket);
    void connect(const std::string &host, int port);

    State get_state() const {
        return state;
    }
    std::string get_peername() const;

    bool is_connected() const {
       return state == State::Open;
    }

    void send(std::unique_ptr<SendBuffer> buffer);

    void set_on_message(const std::function<void(const char *buffer, size_t size)> &fn) {
        on_message = fn;
    }

    void set_on_stream_data(const std::function<void(const char *buffer, size_t size, size_t remaining)> &fn) {
        on_stream_data = fn;
    }

    void set_on_close(const std::function<void()> &fn) {
        on_close = fn;
    }

    void set_on_connect(const std::function<void()> &fn) {
        on_connect = fn;
    }

    void set_on_error(const std::function<void(int error_code)> &fn) {
        on_error = fn;
    }

    void set_stream_mode(bool value) {
       stream_mode = value;
    }

protected:

    std::function<void(const char *buffer, size_t size)> on_message;
    std::function<void(const char *buffer, size_t size, size_t remaining)> on_stream_data;
    std::function<void()> on_close;
    std::function<void()> on_connect;
    /** An error has occured, error_code is from libuv. */
    std::function<void(int error_code)> on_error;

    State state;
    uv_tcp_t uv_socket;

    std::vector<char> buffer;
    bool stream_mode;
    size_t stream_remaining;


private:
    static void _on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void _buf_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void _on_resolved(uv_getaddrinfo_t *handle, int status, addrinfo *response);
};

}}

#endif // LIBLOOM_CONNECTION_H
