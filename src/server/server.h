#ifndef LOOM_SERVER_SERVER_H
#define LOOM_SERVER_SERVER_H

#include "workerconn.h"
#include "clientconn.h"
#include "freshconn.h"
#include "taskmanager.h"
#include "dummyworker.h"

#include "libloom/dictionary.h"

#include <vector>

/** Main class of the server */
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

    DummyWorker& get_dummy_worker() {
        return dummy_worker;
    }

    void remove_freshconnection(FreshConnection &conn);

    TaskManager& get_task_manager() {
        return task_manager;
    }

    const std::vector<std::unique_ptr<WorkerConnection>>& get_connections() {
        return connections;
    }

    ClientConnection& get_client_connection() {
        assert(client_connection.get() != nullptr);
        return *client_connection;
    }

    void add_resend_task(loom::Id id);

    void on_task_finished(TaskNode &task);

    loom::Dictionary& get_dictionary() {
        return dictionary;
    }

    void inform_about_error(std::string &error_msg);
    void inform_about_task_error(loom::Id id, WorkerConnection &wconn, const std::string &error_msg);

    loom::Id new_id(int count = 1) {
        auto id = id_counter;
        id_counter += count;
        return id;
    }

    void send_dictionary(loom::Connection &connection);

    int get_worker_ncpus();

private:
    void start_listen();

    loom::Dictionary dictionary;
    uv_loop_t *loop;
    std::vector<std::unique_ptr<WorkerConnection>> connections;

    std::vector<std::unique_ptr<FreshConnection>> fresh_connections;

    std::unique_ptr<ClientConnection> client_connection;

    uv_tcp_t listen_socket;
    int listen_port;

    TaskManager task_manager;
    DummyWorker dummy_worker;

    loom::Id id_counter;
    static void _on_new_connection(uv_stream_t *stream, int status);
};

#endif // LOOM_SERVER_SERVER_H
