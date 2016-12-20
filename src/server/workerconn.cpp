#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"
#include "libloomw/loomcomm.pb.h"
#include "taskmanager.h"


using namespace loom;
using namespace loom::base;

WorkerConnection::WorkerConnection(Server &server,
                                   std::unique_ptr<loom::base::Socket> socket,
                                   const std::string& address,
                                   const std::vector<loom::Id> &task_types,
                                   const std::vector<loom::Id> &data_types,
                                   int resource_cpus, int worker_id)
    : server(server),
      socket(std::move(socket)),
      resource_cpus(resource_cpus),
      address(address),
      task_types(task_types),
      data_types(data_types),
      worker_id(worker_id)
{
    logger->info("Worker {} connected (cpus={})", address, resource_cpus);
    if (this->socket) {
        this->socket->set_on_message([this](const char *buffer, size_t size) {
            on_message(buffer, size);
        });
        this->socket->set_on_close([this](){
            logger->info("Worker {} disconnected.", this->address);
            this->server.remove_worker_connection(*this);
        });
        server.send_dictionary(*this->socket);
    }

    if (task_types.size() == 0) {
        logger->warn("No task_type has been registered by worker");
    }
}

void WorkerConnection::on_message(const char *buffer, size_t size)
{
    loomcomm::WorkerResponse msg;
    msg.ParseFromArray(buffer, size);

    if (msg.type() == loomcomm::WorkerResponse_Type_FINISH) {
        server.on_task_finished(msg.id(), msg.size(), msg.length(), this);
        return;
    }

    if (msg.type() == loomcomm::WorkerResponse_Type_FAILED) {
        assert(msg.has_error_msg());
        server.inform_about_task_error(msg.id(), *this, msg.error_msg());
    }
}

void WorkerConnection::send_task(const PlanNode &task)
{
    auto id = task.get_id();
    logger->debug("Assigning task id={} to address={} cpus={}", id, address, task.get_n_cpus());

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_TASK);
    msg.set_id(id);
    msg.set_task_type(task.get_task_type());
    msg.set_task_config(task.get_config());

    for (loom::Id id : task.get_inputs()) {
        msg.add_task_inputs(id);
    }
    send_message(*socket, msg);
}

void WorkerConnection::send_data(Id id, const std::string &address)
{
    logger->debug("Command for {}: SEND id={} address={}", this->address, id, address);

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_SEND);
    msg.set_id(id);
    msg.set_address(address);
    send_message(*socket, msg);
}

void WorkerConnection::remove_data(Id id)
{
    logger->debug("Command for {}: REMOVE id={}", this->address, id);
    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_REMOVE);
    msg.set_id(id);
    send_message(*socket, msg);
}
