#include "server.h"

#include "libloom/pbutils.h"
#include "libloom/compat.h"
#include "libloom/log.h"
#include "libloom/loomcomm.pb.h"

#include <sstream>

using namespace loom;
using namespace loom::base;

Server::Server(uv_loop_t *loop, int port)
    : loop(loop),      
      task_manager(*this),
      dummy_worker(*this),
      id_counter(0),
      task_distribution_active(false)
{
    /* Since the server do not implement fully resource management, we forces
     * symbol for the only schedulable resouce: loom/resource/cpus */
    dictionary.find_or_create("loom/resource/cpus");

    if (loop != NULL) {
        logger->info("Starting server on {}", port);
        listener.start(loop, port, [this]() {
            on_new_connection();
        });
        dummy_worker.start_listen();
        logger->debug("Dummy worker started at {}", dummy_worker.get_listen_port());
    }

    if (loop) {
        UV_CHECK(uv_idle_init(loop, &distribution_idle));
    }

    distribution_idle.data = this;
}

void Server::add_worker_connection(std::unique_ptr<WorkerConnection> &&conn)
{
    connections.push_back(std::move(conn));
}

void Server::remove_worker_connection(WorkerConnection &conn)
{
    auto i = std::find_if(
                connections.begin(),
                connections.end(),
                [&](std::unique_ptr<WorkerConnection>& p) { return p.get() == &conn; } );
    assert(i != connections.end());
    connections.erase(i);
}

void Server::add_client_connection(std::unique_ptr<ClientConnection> conn)
{
    // We now allow only one client
    assert(client_connection.get() == nullptr);
    client_connection = std::move(conn);
    assert(client_connection.get() != nullptr);
}

void Server::remove_client_connection(ClientConnection &conn)
{
    assert(&conn == client_connection.get());
    client_connection.reset();
    task_manager.trash_all_tasks();
}

void Server::remove_freshconnection(FreshConnection &conn)
{
    auto i = std::find_if(
                fresh_connections.begin(),
                fresh_connections.end(),
                [&](std::unique_ptr<FreshConnection>& p) { return p.get() == &conn; } );
    assert(i != fresh_connections.end());
    fresh_connections.erase(i);
}

void Server::on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    task_manager.on_task_finished(id, size, length, wc);
}

void Server::on_data_transferred(loom::base::Id id, WorkerConnection *wc)
{
    task_manager.on_data_transferred(id, wc);
}

void Server::on_task_failed(Id id, WorkerConnection *wc, const std::string &error_msg)
{
    task_manager.on_task_failed(id, wc, error_msg);
}


void Server::inform_about_task_error(Id id, WorkerConnection &wconn, const std::string &error_msg)
{
    logger->error("Task id={} failed on worker {}: {}",
                id, wconn.get_address(), error_msg);

    loomcomm::ClientResponse msg;
    msg.set_type(loomcomm::ClientResponse_Type_ERROR);
    loomcomm::Error *error = msg.mutable_error();

    error->set_id(id);
    error->set_worker(wconn.get_address());
    error->set_error_msg(error_msg);

    if (client_connection) {
        client_connection->send_message(msg);
    }
}

void Server::send_dictionary(loom::base::Socket &socket)
{
    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_DICTIONARY);
    std::vector<std::string> symbols = dictionary.get_all_symbols();
    for (std::string &symbol : symbols) {
        std::string *s = msg.add_symbols();
        *s = symbol;
    }
    loom::base::send_message(socket, msg);
}

int Server::get_worker_ncpus()
{
    int count = 0;
    for (auto &w : connections) {
        count += w->get_resource_cpus();
    }
    return count;
}

void Server::on_new_connection()
{
    auto connection = std::make_unique<FreshConnection>(*this);
    connection->accept(listener);
    fresh_connections.push_back(std::move(connection));
}

void Server::report_event(std::unique_ptr<loomcomm::Event> event)
{
    if (!client_connection) {
        return;
    }

    loomcomm::ClientResponse cmsg;
    cmsg.set_type(loomcomm::ClientResponse_Type_EVENT);
    cmsg.set_allocated_event(event.release());

    client_connection->send_message(cmsg);
}

void Server::need_task_distribution()
{
    if (task_distribution_active) {
        return;
    }
    task_distribution_active = true;
    UV_CHECK(uv_idle_start(&distribution_idle, _distribution_callback));
}

void Server::_distribution_callback(uv_idle_t *idle)
{
    UV_CHECK(uv_idle_stop(idle));
    Server *server = static_cast<Server*>(idle->data);
    server->task_distribution_active = false;
    server->task_manager.run_task_distribution();
}
