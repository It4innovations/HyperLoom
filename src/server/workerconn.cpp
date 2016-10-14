#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"
#include "libloom/loomcomm.pb.h"
#include "taskmanager.h"


using namespace loom;

WorkerConnection::WorkerConnection(Server &server,
                                   std::unique_ptr<Connection> connection,
                                   const std::string& address,
                                   const std::vector<loom::Id> &task_types,
                                   const std::vector<loom::Id> &data_types,
                                   int resource_cpus, int worker_id)
    : server(server),
      connection(std::move(connection)),
      resource_cpus(resource_cpus),
      address(address),
      task_types(task_types),
      data_types(data_types),
      worker_id(worker_id)
{
    llog->info("Worker {} connected (cpus={})", address, resource_cpus);
    if (this->connection) {
        this->connection->set_callback(this);
        server.send_dictionary(*this->connection);
    }

    if (unlikely(task_types.size() == 0)) {
        llog->warn("No task_type has been registered by worker");
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

void WorkerConnection::on_close()
{
    llog->info("Worker {} disconnected.", address);
    server.remove_worker_connection(*this);
}

void WorkerConnection::send_task(const PlanNode &task)
{
    auto id = task.get_id();
    llog->debug("Assigning task id={} to address={} cpu={}", id, address, task.get_n_cpus());

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_TASK);
    msg.set_id(id);
    msg.set_task_type(task.get_task_type());
    msg.set_task_config(task.get_config());

    for (loom::Id id : task.get_inputs()) {
        msg.add_task_inputs(id);
    }
    connection->send_message(msg);
}

void WorkerConnection::send_data(Id id, const std::string &address, bool with_size)
{
    llog->debug("Command for {}: SEND id={} address={}", this->address, id, address);

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_SEND);
    msg.set_id(id);
    msg.set_address(address);
    if (with_size) {
        msg.set_with_size(with_size);
    }
    connection->send_message(msg);

}

void WorkerConnection::remove_data(Id id)
{
    llog->debug("Command for {}: REMOVE id={}", this->address, id);
    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_REMOVE);
    msg.set_id(id);
    connection->send_message(msg);
}
