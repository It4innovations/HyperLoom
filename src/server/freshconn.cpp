#include "freshconn.h"
#include "workerconn.h"
#include "clientconn.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/log.h"
#include "libloom/loomcomm.pb.h"

#include <sstream>

using namespace loom;

FreshConnection::FreshConnection(Server &server) :
    server(server), connection(std::make_unique<Connection>(this, server.get_loop()))
{
}

void FreshConnection::accept(uv_tcp_t *listen_socket) {
    connection->accept(listen_socket);
    llog->debug("New connection to server ({})", connection->get_peername());
}

void FreshConnection::on_message(const char *buffer, size_t size)
{    
    loomcomm::Register msg;
    msg.ParseFromArray(buffer, size);

    if (msg.protocol_version() != PROTOCOL_VERSION) {
        llog->error("Connection from {} registered with invalid protocol version",
                    connection->get_peername());
        connection->close_and_discard_remaining_data();
        return;
    }

    if (msg.type() == loomcomm::Register_Type_REGISTER_WORKER) {
        std::stringstream address;
        address << this->connection->get_peername() << ":" << msg.port();

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
                                                        std::move(connection),
                                                        address.str(),
                                                        task_types,
                                                        data_types,
                                                        msg.cpus());

        server.add_worker_connection(std::move(wconn));
        server.remove_freshconnection(*this);
        return;
    }
    if (msg.type() == loomcomm::Register_Type_REGISTER_CLIENT) {
        bool info_flag = msg.has_info() && msg.info();
        auto cconn = std::make_unique<ClientConnection>(server,
                                                        std::move(connection),
                                                        info_flag);
        server.add_client_connection(std::move(cconn));
        assert(connection.get() == nullptr);
        server.remove_freshconnection(*this);
        return;
    }
    llog->error("Invalid registration from {}", connection->get_peername());
    connection->close_and_discard_remaining_data();
}

void FreshConnection::on_close()
{
    llog->error("Connection from closed without registration");
    server.remove_freshconnection(*this);
}
