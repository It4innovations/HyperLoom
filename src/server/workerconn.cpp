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
                                   int resource_cpus)
    : server(server),
      connection(std::move(connection)),
      resource_cpus(resource_cpus),
      address(address),
      task_types(task_types)
{

    llog->info("Worker {} connected (cpus={})", address, resource_cpus);
    if (this->connection) {
        this->connection->set_callback(this);
        loomcomm::WorkerCommand msg;
        msg.set_type(loomcomm::WorkerCommand_Type_DICTIONARY);
        std::vector<std::string> symbols = server.get_dictionary().get_all_symbols();
        for (std::string &symbol : symbols) {
            std::string *s = msg.add_symbols();
            *s = symbol;
        }
        SendBuffer *send_buffer = new SendBuffer();
        send_buffer->add(msg);
        this->connection->send_buffer(send_buffer);
    }

    if (unlikely(task_types.size() == 0)) {
        llog->warn("No task_type has been registered by worker");
    }

    /*auto &manager = server.get_task_manager();
    for (size_t i = 0; i < task_types.size(); i++) {
        task_type_translates[i] = manager.translate_task_type(task_types[i]);
    }*/
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
        task->set_finished();
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
