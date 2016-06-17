#ifndef LOOM_SERVER_SERVER_H
#define LOOM_SERVER_SERVER_H

#include "workerconn.h"
#include "clientconn.h"
#include "freshconn.h"
#include "taskmanager.h"

#include "libloom/worker.h"

#include <vector>

class Server {

public:
    Server(uv_loop_t *loop, int port);

    uv_loop_t* get_loop() {
        return loop;
    }

    void add_worker_connection(std::unique_ptr<WorkerConnection> conn) {
        connections.push_back(std::move(conn));
    }
    void remove_worker_connection(WorkerConnection &conn);

    void add_client_connection(std::unique_ptr<ClientConnection> conn);
    void remove_client_connection(ClientConnection &conn);

    loom::Worker& get_dummy_worker() const {
        return *dummy_worker;
    }

    std::string get_dummy_worker_address() const;

    void remove_freshconnection(FreshConnection &conn);

    TaskManager& get_task_manager() {
        return task_manager;
    }

    auto& get_connections() {
        return connections;
    }

    ClientConnection& get_client_connection() {
        assert(client_connection.get() != nullptr);
        return *client_connection;
    }

    void add_resend_task(loom::Id id);

private:
    void start_listen();

    uv_loop_t *loop;
    std::unique_ptr<loom::Worker> dummy_worker;

    std::vector<std::unique_ptr<WorkerConnection>> connections;

    std::vector<std::unique_ptr<FreshConnection>> fresh_connections;

    std::unique_ptr<ClientConnection> client_connection;

    uv_tcp_t listen_socket;
    int listen_port;

    TaskManager task_manager;

    static void _on_new_connection(uv_stream_t *stream, int status);
};

#endif // LOOM_SERVER_SERVER_H
