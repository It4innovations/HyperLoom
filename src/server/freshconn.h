#ifndef LOOM_SERVER_FRESHCONN
#define LOOM_SERVER_FRESHCONN

#include "libloom/connection.h"

class Server;

/**
 * A new connection before registration (it is not determined if it is client or worker)
 */
class FreshConnection : public loom::ConnectionCallback {

public:
    FreshConnection(Server &server);

    Server& get_server() const {
        return server;
    }

    void accept(uv_tcp_t* listen_socket);

protected:
    void on_message(const char *buffer, size_t size);
    void on_close();

    Server &server;
    std::unique_ptr<loom::Connection> connection;
};

#endif // LOOM_SERVER_FRESHCONN
