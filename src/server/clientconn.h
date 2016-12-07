#ifndef LOOM_SERVER_CLIENTCONN_H
#define LOOM_SERVER_CLIENTCONN_H

#include "libloomnet/pbutils.h"
#include "libloomnet/socket.h"


namespace loom {
class SendBuffer;
}

class Server;

/** Connection to client */
class ClientConnection {
public:
    ClientConnection(Server &server,
                     std::unique_ptr<loom::net::Socket> socket);

    void on_message(const char *buffer, size_t size);

    void send(std::unique_ptr<loom::net::SendBuffer> buffer) {
        socket->send(std::move(buffer));
    }

    void send_message(google::protobuf::MessageLite &msg) {
       loom::net::send_message(*socket, msg);
    }

protected:
    Server &server;
    std::unique_ptr<loom::net::Socket> socket;
};


#endif // LOOM_SERVER_CLIENTCONN_H
