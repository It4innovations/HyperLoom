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

    switch(request.type()) {
    case ClientRequest_Type_RELEASE:
        release(request.id());
        return;
    case ClientRequest_Type_FETCH:
        fetch(request.id());
        return;
    case ClientRequest_Type_PLAN: {
        logger->debug("Plan received");
        const Plan &plan = request.plan();
        loom::base::Id id_base = task_manager.add_plan(plan, request.load_checkpoints());
        logger->info("Plan submitted tasks={}, load_checkpoints={}", plan.tasks_size(), request.load_checkpoints());

        if (server.get_trace()) {
            server.create_file_in_trace_dir(std::to_string(id_base) + ".plan", buffer, size);
            server.get_trace()->entry("SUBMIT", id_base);
        }
        return;
    }
    case ClientRequest_Type_STATS: {
        logger->debug("Stats request");
        ClientResponse cmsg;
        cmsg.set_type(ClientResponse_Type_STATS);
        Stats *stats = cmsg.mutable_stats();
        stats->set_n_workers(server.get_connections().size());
        stats->set_n_data_objects(server.get_task_manager().get_n_of_data_objects());
        send_message(cmsg);
        return;
    }
    case ClientRequest_Type_TRACE:
        server.create_trace(request.trace_path());
        return;
    case ClientRequest_Type_TERMINATE:
        logger->info("Server terminated by client");
        server.terminate();
        return;
    default:
        logger->critical("Invalid request type");
        exit(1);
    }
}

void ClientConnection::send_info_about_finished_result(const TaskNode &task)
{
    using namespace loom::pb::comm;
    ClientResponse msg;
    msg.set_type(ClientResponse_Type_TASK_FINISHED);
    msg.set_id(task.get_id());
    send_message(msg);
}

void ClientConnection::send_error(const std::string &error_msg)
{
    using namespace loom::pb::comm;
    ClientResponse msg;
    msg.set_type(ClientResponse_Type_ERROR);
    Error *error = msg.mutable_error();
    error->set_error_msg(error_msg);

    send_message(msg);
}

void ClientConnection::fetch(Id id)
{
    logger->debug("Client fetch: id={}", id);
    TaskNode *node = get_result_node(id);
    if (!node) {
       return;
    }
    WorkerConnection *owner = node->get_random_owner();
    if (!owner) {
        logger->error("Client asked for nonfinished task; id={}", id);
        send_error("Task is not finished");
        return;
    }
    owner->send_data(id, server.get_dummy_worker().get_address());
}

void ClientConnection::release(Id id)
{
   logger->debug("Client release: id={}", id);
   TaskNode *node = get_result_node(id);
   if (!node) {
      return;
   }
   server.get_task_manager().release_node(node);
}

TaskNode *ClientConnection::get_result_node(Id id)
{
   TaskNode *node = server.get_task_manager().get_node_ptr(id);
   if (!node) {
       logger->error("Client asked invalid for id; id={}", id);
       send_error("Invalid id: " + std::to_string(id));
       return nullptr;
   }
   if (!node->is_result()) {
       logger->error("Client asked for non-result node; id={}", id);
       send_error("Task is not result");
       return nullptr;
   }
   return node;
}

void ClientConnection::send_task_failed(Id id, WorkerConnection &wconn, const std::string &error_msg)
{
    using namespace loom::pb::comm;
    ClientResponse msg;
    msg.set_type(ClientResponse_Type_TASK_FAILED);
    Error *error = msg.mutable_error();

    error->set_id(id);
    error->set_worker(wconn.get_address());
    error->set_error_msg(error_msg);

    send_message(msg);
}
