#include "clientconn.h"
#include "server.h"



#include "libloom/loomplan.pb.h"
#include "libloom/loomcomm.pb.h"
#include "libloom/log.h"
#include "libloom/compat.h"
#include "libloom/pbutils.h"


using namespace loom;
using namespace loom::base;

ClientConnection::ClientConnection(Server &server,
                                   std::unique_ptr<loom::base::Socket> socket)
    : server(server), socket(std::move(socket))
{
    this->socket->set_on_message([this](const char *buffer, size_t size) {
        on_message(buffer, size);
    });
    this->socket->set_on_close([this](){
        logger->info("Client disconnected");
        this->server.remove_client_connection(*this);
    });

    logger->info("Client {} connected", this->socket->get_peername());

    // Send dictionary
    loomcomm::ClientResponse cmsg;
    cmsg.set_type(loomcomm::ClientResponse_Type_DICTIONARY);

    std::vector<std::string> symbols = server.get_dictionary().get_all_symbols();
    for (std::string &symbol : symbols) {
        std::string *s = cmsg.add_symbols();
        *s = symbol;
    }
    loom::base::send_message(*this->socket, cmsg);
    // End of send dictionary
}

void ClientConnection::on_message(const char *buffer, size_t size)
{
    auto& task_manager = server.get_task_manager();
    loomcomm::ClientRequest request;
    request.ParseFromArray(buffer, size);

    if (request.type() == loomcomm::ClientRequest_Type_PLAN) {
        logger->debug("Plan received");
        const loomplan::Plan &plan = request.plan();
        loom::base::Id id_base = server.new_id(plan.tasks_size());
        bool report = request.report();
        task_manager.add_plan(Plan(plan, id_base, server.get_dictionary()), report);
        logger->info("Plan submitted tasks={} report={}", plan.tasks_size(), report);
    } else if (request.type() == loomcomm::ClientRequest_Type_STATS) {
        logger->debug("Stats request");
        loomcomm::ClientResponse cmsg;
        cmsg.set_type(loomcomm::ClientResponse_Type_STATS);
        loomcomm::Stats *stats = cmsg.mutable_stats();
        stats->set_n_workers(server.get_connections().size());
        stats->set_n_data_objects(server.get_task_manager().get_n_of_data_objects());
        send_message(cmsg);
    } else {
        logger->critical("Invalid request type");
        exit(1);
    }
}
