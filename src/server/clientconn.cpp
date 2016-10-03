#include "clientconn.h"
#include "server.h"

#include "libloom/loomplan.pb.h"
#include "libloom/loomcomm.pb.h"
#include "libloom/log.h"

using namespace loom;

ClientConnection::ClientConnection(Server &server, std::unique_ptr<loom::Connection> connection, bool info_flag)
    : server(server), connection(std::move(connection)), info_flag(info_flag)
{
    this->connection->set_callback(this);
    llog->info("Client {} connected", this->connection->get_peername());

    // Send dictionary
    loomcomm::ClientMessage cmsg;
    cmsg.set_type(loomcomm::ClientMessage_Type_DICTIONARY);

    std::vector<std::string> symbols = server.get_dictionary().get_all_symbols();
    for (std::string &symbol : symbols) {
        std::string *s = cmsg.add_symbols();
        *s = symbol;
    }
    SendBuffer *send_buffer = new SendBuffer();
    send_buffer->add(cmsg);
    this->connection->send_buffer(send_buffer);
    // End of send dictionary
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

    loom::Id id_base = server.new_id(plan.tasks_size());
    task_manager.add_plan(Plan(plan, id_base));
    llog->info("Plan submitted tasks={}", plan.tasks_size());
}

void ClientConnection::on_close()
{
    llog->info("Client disconnected");
    server.remove_client_connection(*this);
}
