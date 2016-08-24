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
                     std::unique_ptr<loom::Connection> connection, bool info_flag);
    ~ClientConnection();
    void on_message(const char *buffer, size_t size);
    void on_close();

    void send_buffer(loom::SendBuffer *buffer) {
        connection->send_buffer(buffer);
    }

    bool has_info_flag() const {
        return info_flag;
    }

protected:
    Server &server;
    std::unique_ptr<loom::Connection> connection;
    bool info_flag;
};


#endif // LOOM_SERVER_CLIENTCONN_H
