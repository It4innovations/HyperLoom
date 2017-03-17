#ifndef LOOM_SERVER_CLIENTCONN_H
#define LOOM_SERVER_CLIENTCONN_H

#include "libloom/pbutils.h"
#include "libloom/socket.h"
#include "tasknode.h"

namespace loom {
class SendBuffer;
}

class Server;

/** Connection to client */
class ClientConnection {
public:
    ClientConnection(Server &server,
                     std::unique_ptr<loom::base::Socket> socket);

    void on_message(const char *buffer, size_t size);

    void send(std::unique_ptr<loom::base::SendBuffer> buffer) {
        socket->send(std::move(buffer));
    }

    void send_message(google::protobuf::MessageLite &msg) {
       loom::base::send_message(*socket, msg);
    }

    void send_info_about_finished_result(const TaskNode &task);

    void send_task_failed(loom::base::Id id, WorkerConnection &wconn, const std::string &error_msg);
    void send_error(const std::string &error_msg);

protected:

    void fetch(loom::base::Id id);
    void release(loom::base::Id id);

    TaskNode *get_result_node(loom::base::Id id);

    Server &server;
    std::unique_ptr<loom::base::Socket> socket;
};


#endif // LOOM_SERVER_CLIENTCONN_H
