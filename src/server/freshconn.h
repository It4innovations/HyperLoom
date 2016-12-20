#ifndef LOOM_SERVER_FRESHCONN
#define LOOM_SERVER_FRESHCONN

#include "libloom/socket.h"
#include "libloom/listener.h"

class Server;

/**
 * A new connection before registration (it is not determined if it is client or worker)
 */
class FreshConnection {

public:
    FreshConnection(Server &server);

    Server& get_server() const {
        return server;
    }

    void accept(loom::base::Listener &listener);

protected:
    void on_message(const char *buffer, size_t size);

    Server &server;
    std::unique_ptr<loom::base::Socket> socket;
};

#endif // LOOM_SERVER_FRESHCONN
