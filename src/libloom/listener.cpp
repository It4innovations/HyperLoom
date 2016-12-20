#include "listener.h"

#include "log.h"


using namespace loom::base;

void Listener::start(uv_loop_t *loop, int port, const std::function<void()> &callback)
{
    on_new_connection = callback;
    uv_socket.data = this;
    UV_CHECK(uv_tcp_init(loop, &uv_socket));

    struct sockaddr_in addr;
    UV_CHECK(uv_ip4_addr("0.0.0.0", port, &addr));

    UV_CHECK(uv_tcp_bind(&uv_socket, (const struct sockaddr *) &addr, 0));
    UV_CHECK(uv_listen((uv_stream_t *) &uv_socket, 10,
                       [](uv_stream_t *stream, int status) {
                 UV_CHECK(status);
                 Listener *listener = static_cast<Listener*>(stream->data);
                 listener->on_new_connection();
             }));
}

void Listener::close()
{
   uv_close((uv_handle_t*) &uv_socket, nullptr);
}

int Listener::get_port() const
{
    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(&uv_socket, (sockaddr*) &sockname, &namelen);
    return ntohs(sockname.sin_port);
}

