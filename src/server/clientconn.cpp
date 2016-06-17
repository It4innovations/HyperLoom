#include "clientconn.h"
#include "server.h"

#include "libloom/loomplan.pb.h"
#include "libloom/log.h"

using namespace loom;

ClientConnection::ClientConnection(Server &server, std::unique_ptr<loom::Connection> connection)
    : server(server), connection(std::move(connection))
{
    this->connection->set_callback(this);
    llog->info("Client {} connected", this->connection->get_peername());
}

ClientConnection::~ClientConnection()
{

}

void ClientConnection::on_message(const char *buffer, size_t size)
{
    llog->debug("Plan received");
    loomplan::Plan plan;
    plan.ParseFromArray(buffer, size);
    auto& task_manager = server.get_task_manager();
    task_manager.add_plan(plan);
    llog->info("Plan submitted tasks={}", plan.tasks_size());
}

void ClientConnection::on_close()
{
    llog->info("Client disconnected");
    server.remove_client_connection(*this);
}
