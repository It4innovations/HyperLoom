#include "freshconn.h"
#include "workerconn.h"
#include "clientconn.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/log.h"
#include "pb/comm.pb.h"

#include <sstream>

using namespace loom;
using namespace loom::base;

FreshConnection::FreshConnection(Server &server) :
    server(server),
    socket(std::make_unique<base::Socket>(server.get_loop()))
{
    socket->set_on_close([this](){
        logger->error("Connection closed without registration");
        this->server.remove_freshconnection(*this);
    });

    socket->set_on_message([this](const char *buffer, size_t size) {
        on_message(buffer, size);
    });
}

void FreshConnection::accept(loom::base::Listener &listener) {
    listener.accept(*socket);
    logger->debug("New connection to server ({})", socket->get_peername());
}

void FreshConnection::on_message(const char *buffer, size_t size)
{
    using namespace loom::pb::comm;
    Register msg;
    bool r = msg.ParseFromArray(buffer, size);
    if (!r) {
        logger->error("Invalid registration message from {}",
                    socket->get_peername());
        socket->close_and_discard_remaining_data();
        return;
    }

    if (msg.protocol_version() != PROTOCOL_VERSION) {
        logger->error("Connection from {} registered with invalid protocol version",
                    socket->get_peername());
        socket->close_and_discard_remaining_data();
        return;
    }

    if (msg.type() == Register_Type_REGISTER_WORKER) {
        std::stringstream address;
        address << socket->get_peername() << ":" << msg.port();

        std::vector<int> task_types, data_types;
        task_types.reserve(msg.task_types_size());
        Dictionary &dictionary = server.get_dictionary();

        for (int i = 0; i < msg.task_types_size(); i++) {
            task_types.push_back(dictionary.find_or_create(msg.task_types(i)));
        }

        for (int i = 0; i < msg.data_types_size(); i++) {
            data_types.push_back(dictionary.find_or_create(msg.data_types(i)));
        }

        auto wconn = std::make_unique<WorkerConnection>(server,
                                                        std::move(socket),
                                                        address.str(),
                                                        task_types,
                                                        data_types,
                                                        msg.cpus(),
                                                        server.new_id());

        server.add_worker_connection(std::move(wconn));
        server.remove_freshconnection(*this);
        return;
    }
    if (msg.type() == Register_Type_REGISTER_CLIENT) {
        if (server.has_client_connection()) {
            logger->error("New client connection while there still exists a client connection.");
            logger->error("The new connection is rejected.");
            logger->error("The current version now supports only one clien tconnection");
            socket->close_and_discard_remaining_data();
            return;
        }
        auto cconn = std::make_unique<ClientConnection>(server,
                                                        std::move(socket));
        server.add_client_connection(std::move(cconn));
        server.remove_freshconnection(*this);
        return;
    }
    logger->error("Invalid registration from {}", socket->get_peername());
    socket->close_and_discard_remaining_data();
}
