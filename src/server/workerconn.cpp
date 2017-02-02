#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"
#include "libloom/loomcomm.pb.h"
#include "taskmanager.h"


using namespace loom;
using namespace loom::base;

WorkerConnection::WorkerConnection(Server &server,
                                   std::unique_ptr<loom::base::Socket> socket,
                                   const std::string& address,
                                   const std::vector<loom::base::Id> &task_types,
                                   const std::vector<loom::base::Id> &data_types,
                                   int resource_cpus, int worker_id)
    : server(server),
      socket(std::move(socket)),
      free_cpus(resource_cpus),
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

    if (msg.type() == loomcomm::WorkerResponse_Type_FINISHED) {
        server.on_task_finished(msg.id(), msg.size(), msg.length(), this);
        return;
    }

    if (msg.type() == loomcomm::WorkerResponse_Type_TRANSFERED) {
        server.on_data_transferred(msg.id(), this);
        return;
    }

    if (msg.type() == loomcomm::WorkerResponse_Type_FAILED) {
        assert(msg.has_error_msg());
        server.on_task_failed(msg.id(), this, msg.error_msg());
        return;
    }
}

void WorkerConnection::send_task(const TaskNode &task)
{
    auto id = task.get_id();
    logger->debug("Assigning task id={} (client_id={}) to address={} cpus={}",
                  id, task.get_client_id(), address, task.get_n_cpus());

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_TASK);
    msg.set_id(id);
    const TaskDef& def = task.get_task_def();
    msg.set_task_type(def.task_type);
    msg.set_task_config(def.config);
    msg.set_n_cpus(def.n_cpus);

    for (TaskNode *input_node : task.get_inputs()) {
        msg.add_task_inputs(input_node->get_id());
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
