#ifndef LOOM_SERVER_SERVER_H
#define LOOM_SERVER_SERVER_H

#include "workerconn.h"
#include "clientconn.h"
#include "freshconn.h"
#include "taskmanager.h"
#include "dummyworker.h"
#include "trace.h"


#include "libloom/dictionary.h"
#include "libloom/listener.h"

#include <vector>
#include <fstream>


/** Main class of the server */
class Server {

public:
    Server(uv_loop_t *loop, int port);

    uv_loop_t* get_loop() {
        return loop;
    }

    void add_worker_connection(std::unique_ptr<WorkerConnection> &&conn);
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

    bool has_client_connection() const {
        return client_connection.get() != nullptr;
    }

    void add_resend_task(loom::base::Id id);

    void on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc);
    void on_data_transferred(loom::base::Id id, WorkerConnection *wc);

    loom::base::Dictionary& get_dictionary() {
        return dictionary;
    }

    void inform_about_task_error(loom::base::Id id, WorkerConnection &wconn, const std::string &error_msg);
    void on_task_failed(loom::base::Id id, WorkerConnection *wc, const std::string &error_msg);

    loom::base::Id new_id(int count = 1) {
        auto id = id_counter;
        id_counter += count;
        return id;
    }

    void send_dictionary(loom::base::Socket &socket);
    int get_worker_ncpus();
    void need_task_distribution();

    const std::vector<std::unique_ptr<WorkerConnection>>& get_workers() const {
        return connections;
    }

    void create_trace(const std::string &trace_path);
    void terminate();

    void create_file_in_trace_dir(const std::string &filename, const char *data, size_t size);

    std::unique_ptr<ServerTrace>& get_trace() {
        return trace;
    }

private:



    loom::base::Dictionary dictionary;
    uv_loop_t *loop;
    loom::base::Listener listener;

    std::vector<std::unique_ptr<WorkerConnection>> connections;

    std::vector<std::unique_ptr<FreshConnection>> fresh_connections;

    std::unique_ptr<ClientConnection> client_connection;


    TaskManager task_manager;
    DummyWorker dummy_worker;

    loom::base::Id id_counter;

    void on_new_connection();

    bool task_distribution_active;
    uv_idle_t distribution_idle;

    std::string trace_dir;
    std::unique_ptr<ServerTrace> trace;

    static void _distribution_callback(uv_idle_t *idle);
};

#endif // LOOM_SERVER_SERVER_H
