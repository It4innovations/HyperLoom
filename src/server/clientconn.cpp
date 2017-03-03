#include "clientconn.h"
#include "server.h"

#include "pb/comm.pb.h"
#include "libloom/log.h"
#include "libloom/compat.h"
#include "libloom/pbutils.h"


using namespace loom;
using namespace loom::base;

ClientConnection::ClientConnection(Server &server,
                                   std::unique_ptr<loom::base::Socket> socket)
    : server(server), socket(std::move(socket))
{
    using namespace loom::pb::comm;

    this->socket->set_on_message([this](const char *buffer, size_t size) {
        on_message(buffer, size);
    });
    this->socket->set_on_close([this](){
        logger->info("Client disconnected");
        this->server.remove_client_connection(*this);
    });

    logger->info("Client {} connected", this->socket->get_peername());

    // Send dictionary
    ClientResponse cmsg;
    cmsg.set_type(ClientResponse_Type_DICTIONARY);

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
    using namespace loom::pb::comm;
    auto& task_manager = server.get_task_manager();
    ClientRequest request;
    request.ParseFromArray(buffer, size);

    if (request.type() == ClientRequest_Type_PLAN) {
        logger->debug("Plan received");
        const Plan &plan = request.plan();
        loom::base::Id id_base = task_manager.add_plan(plan);
        logger->info("Plan submitted tasks={}", plan.tasks_size());

        if (server.get_trace()) {
            server.create_file_in_trace_dir("0.plan", buffer, size);
            server.get_trace()->entry("SUBMIT", id_base);
        }

    } else if (request.type() == ClientRequest_Type_STATS) {
        logger->debug("Stats request");
        ClientResponse cmsg;
        cmsg.set_type(ClientResponse_Type_STATS);
        Stats *stats = cmsg.mutable_stats();
        stats->set_n_workers(server.get_connections().size());
        stats->set_n_data_objects(server.get_task_manager().get_n_of_data_objects());
        send_message(cmsg);
    } else if (request.type() == ClientRequest_Type_TRACE) {
        server.create_trace(request.trace_path());
    } else if (request.type() == ClientRequest_Type_TERMINATE) {
        logger->info("Server terminated by client");
        server.terminate();
    } else {
        logger->critical("Invalid request type");
        exit(1);
    }
}
