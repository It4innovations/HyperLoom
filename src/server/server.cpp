#include "server.h"
#include "resendtask.h"

#include "libloom/utils.h"
#include "libloom/log.h"

#include <sstream>

using namespace loom;

Server::Server(uv_loop_t *loop, int port)
    : loop(loop),
      listen_port(port),
      task_manager(*this)
{
    if (loop != NULL) {
        UV_CHECK(uv_tcp_init(loop, &listen_socket));
        listen_socket.data = this;
        start_listen();

        llog->info("Starting server on {}", port);

        dummy_worker = std::make_unique<Worker>(loop, "", 0, "");
        dummy_worker->add_task_factory(std::make_unique<ResendTaskFactory>(*this));
        loom::llog->debug("Dummy worker started at {}", dummy_worker->get_listen_port());
    }
}

void Server::remove_worker_connection(WorkerConnection &conn)
{
    auto i = std::find_if(
                connections.begin(),
                connections.end(),
                [&](auto& p) { return p.get() == &conn; } );
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

std::string Server::get_dummy_worker_address() const
{
    std::stringstream s;
    s << "!:" << dummy_worker->get_listen_port();
    return s.str();
}

void Server::remove_freshconnection(FreshConnection &conn)
{
    auto i = std::find_if(
                fresh_connections.begin(),
                fresh_connections.end(),
                [&](auto& p) { return p.get() == &conn; } );
    assert(i != fresh_connections.end());
    fresh_connections.erase(i);
}

void Server::add_resend_task(Id id)
{
    std::unique_ptr<Task> task = std::make_unique<Task>(-1, 0, "");
    task->add_input(id);
    dummy_worker->new_task(std::move(task));
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
