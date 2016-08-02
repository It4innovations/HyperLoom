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
                                   int resource_cpus)
    : server(server),
      connection(std::move(connection)),
      resource_cpus(resource_cpus),
      address(address),
      task_types(task_types),
      data_types(data_types)
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

    const auto it = tasks.find(msg.id());
    assert(it != tasks.end());
    TaskNode *task = it->second;
    tasks.erase(it);

    if (msg.type() == loomcomm::WorkerResponse_Type_FINISH) {
        task->add_owner(this);
        task->set_finished(msg.size(), msg.length());
        server.on_task_finished(*task);
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

void WorkerConnection::send_task(TaskNode *task)
{
    assert(task->is_waiting());
    task->set_state(TaskNode::RUNNING);
    tasks[task->get_id()] = task;

    auto id = task->get_id();
    llog->debug("Assigning task id={} to address={}", id, address);

    loomcomm::WorkerCommand msg;
    msg.set_type(loomcomm::WorkerCommand_Type_TASK);

    msg.set_id(id);
    msg.set_task_type(task->get_task_type());
    msg.set_task_config(task->get_config());

    for (TaskNode *t : task->get_inputs()) {
        msg.add_task_inputs(t->get_id());
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
