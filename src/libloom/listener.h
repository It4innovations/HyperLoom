#ifndef LIBLOOMNET_LISTENER_H
#define LIBLOOMNET_LISTENER_H

#include "socket.h"

#include <uv.h>

namespace loom {
namespace base {

class Listener
{
public:
    Listener() {}

    void start(uv_loop_t *loop, int port, const std::function<void()> &callback);
    void close();

    int get_port() const;

    void accept(Socket &socket) {
        socket.accept(&uv_socket);
    }

protected:
    std::function<void()> on_new_connection;

    uv_tcp_t uv_socket;
};

}
}

#endif // LIBLOOMNET_LISTENER_H
