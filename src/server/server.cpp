#include "server.h"

#include "libloom/compat.h"
#include "libloom/utils.h"
#include "libloom/log.h"
#include "libloom/loomcomm.pb.h"

#include <sstream>

using namespace loom;

Server::Server(uv_loop_t *loop, int port)
    : loop(loop),
      listen_port(port),
      task_manager(*this),
      dummy_worker(*this),
      id_counter(1)
{
    /* Since the server do not implement fully resource management, we forces
     * symbol for the only schedulable resouce: loom/resource/cpus */
    dictionary.find_or_create("loom/resource/cpus");

    if (loop != NULL) {
        UV_CHECK(uv_tcp_init(loop, &listen_socket));
        listen_socket.data = this;
        start_listen();

        llog->info("Starting server on {}", port);


        dummy_worker.start_listen();
        loom::llog->debug("Dummy worker started at {}", dummy_worker.get_listen_port());
    }
}

void Server::add_worker_connection(std::unique_ptr<WorkerConnection> conn)
{
    task_manager.register_worker(conn.get());
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
}

loom::Id Server::translate_to_client_id(loom::Id id) const
{
    return task_manager.get_plan().get_node(id).get_client_id();
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

void Server::on_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    task_manager.on_task_finished(id, size, length, wc);
}

void Server::inform_about_error(std::string &error_msg)
{

}

void Server::inform_about_task_error(Id id, WorkerConnection &wconn, const std::string &error_msg)
{
    llog->error("Task id={} failed on worker {}: {}",
                id, wconn.get_address(), error_msg);

    loomcomm::ClientMessage msg;
    msg.set_type(loomcomm::ClientMessage_Type_ERROR);
    loomcomm::Error *error = msg.mutable_error();

    error->set_id(id);
    error->set_worker(wconn.get_address());
    error->set_error_msg(error_msg);

    if (client_connection) {
        auto buffer = std::make_unique<SendBuffer>();
        buffer->add(msg);
        client_connection->send_buffer(std::move(buffer));
    }
    exit(1);
}

void Server::send_dictionary(Connection &connection)
{
    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_DICTIONARY);
    std::vector<std::string> symbols = dictionary.get_all_symbols();
    for (std::string &symbol : symbols) {
        std::string *s = msg.add_symbols();
        *s = symbol;
    }
    auto send_buffer = std::make_unique<SendBuffer>();
    send_buffer->add(msg);
    connection.send_buffer(std::move(send_buffer));
}

int Server::get_worker_ncpus()
{
    int count = 0;
    for (auto &w : connections) {
        count += w->get_resource_cpus();
    }
    return count;
}

void Server::start_listen()
{
    struct sockaddr_in addr;
    UV_CHECK(uv_ip4_addr("0.0.0.0", listen_port, &addr));

    UV_CHECK(uv_tcp_bind(&listen_socket, (const struct sockaddr *) &addr, 0));
    UV_CHECK(uv_listen((uv_stream_t *) &listen_socket, 30, _on_new_connection));
}

void Server::_on_new_connection(uv_stream_t *stream, int status)
{
    UV_CHECK(status);
    Server *server = static_cast<Server*>(stream->data);
    auto connection = std::make_unique<FreshConnection>(*server);
    connection->accept((uv_tcp_t*) stream);
    server->fresh_connections.push_back(std::move(connection));
}

void Server::report_event(std::unique_ptr<loomcomm::Event> event)
{
    if (!client_connection) {
        return;
    }

    loomcomm::ClientMessage cmsg;
    cmsg.set_type(loomcomm::ClientMessage_Type_EVENT);
    cmsg.set_allocated_event(event.release());

    auto buffer = std::make_unique<SendBuffer>();
    buffer->add(cmsg);
    client_connection->send_buffer(std::move(buffer));
}
