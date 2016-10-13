#ifndef LOOM_SERVER_CLIENTCONN_H
#define LOOM_SERVER_CLIENTCONN_H

#include "libloom/connection.h"

namespace loom {
class SendBuffer;
}

class Server;

/** Connection to client */
class ClientConnection : public loom::ConnectionCallback {
public:
    ClientConnection(Server &server,
                     std::unique_ptr<loom::Connection> connection);
    ~ClientConnection();
    void on_message(const char *buffer, size_t size);
    void on_close();

    void send_buffer(loom::SendBuffer *buffer) {
        connection->send_buffer(buffer);
    }

protected:
    Server &server;
    std::unique_ptr<loom::Connection> connection;
};


#endif // LOOM_SERVER_CLIENTCONN_H
