#include "clientconn.h"
#include "server.h"



#include "libloom/loomplan.pb.h"
#include "libloom/loomcomm.pb.h"
#include "libloomnet/compat.h"
#include "libloomnet/pbutils.h"
#include "libloom/log.h"

using namespace loom;

ClientConnection::ClientConnection(Server &server,
                                   std::unique_ptr<loom::net::Socket> socket)
    : server(server), socket(std::move(socket))
{
    this->socket->set_on_message([this](const char *buffer, size_t size) {
        on_message(buffer, size);
    });
    this->socket->set_on_close([this](){
        llog->info("Client disconnected");
        this->server.remove_client_connection(*this);
    });

    llog->info("Client {} connected", this->socket->get_peername());

    // Send dictionary
    loomcomm::ClientMessage cmsg;
    cmsg.set_type(loomcomm::ClientMessage_Type_DICTIONARY);

    std::vector<std::string> symbols = server.get_dictionary().get_all_symbols();
    for (std::string &symbol : symbols) {
        std::string *s = cmsg.add_symbols();
        *s = symbol;
    }
    loom::net::send_message(*this->socket, cmsg);
    // End of send dictionary
}

void ClientConnection::on_message(const char *buffer, size_t size)
{
    llog->debug("Plan received");
    loomcomm::ClientSubmit submit;
    submit.ParseFromArray(buffer, size);
    auto& task_manager = server.get_task_manager();

    const loomplan::Plan &plan = submit.plan();
    loom::Id id_base = server.new_id(plan.tasks_size());
    task_manager.add_plan(Plan(plan, id_base, server.get_dictionary()), submit.report());
    llog->info("Plan submitted tasks={} report={}", plan.tasks_size(), submit.report());
}
